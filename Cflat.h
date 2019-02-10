
////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.10
//  C++ based Scripting Language
//
//  Copyright (c) 2019 Arturo Cepeda Pérez
//
//  --------------------------------------------------------------------
//
//  This file is part of Cflat. Permission is hereby granted, free 
//  of charge, to any person obtaining a copy of this software and 
//  associated documentation files (the "Software"), to deal in the 
//  Software without restriction, including without limitation the 
//  rights to use, copy, modify, merge, publish, distribute, 
//  sublicense, and/or sell copies of the Software, and to permit 
//  persons to whom the Software is furnished to do so, subject to 
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be 
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
//  KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
//  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
//  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
//  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <functional>

#if !defined (CflatMalloc)
# define CflatMalloc ::malloc
#endif

#if !defined (CflatFree)
# define CflatFree ::free
#endif

#if !defined (CflatSTLString)
# include <string>
# define CflatSTLString std::string
#endif

#if !defined (CflatSTLVector)
# include <vector>
# define CflatSTLVector std::vector
#endif

#if !defined (CflatSTLMap)
# include <map>
# define CflatSTLMap std::map
#endif

#if !defined (CflatAssert)
# include <cassert>
# define CflatAssert assert
#endif

namespace Cflat
{
   enum class TypeCategory : uint8_t
   {
      BuiltIn,
      Struct,
      Class
   };

   enum class Visibility : uint8_t
   {
      Public,
      Protected,
      Private
   };

   enum class TypeRefFlags : uint8_t
   {
      Const      = 1 << 0,
      Pointer    = 1 << 1,
      Reference  = 1 << 2
   };


   struct Symbol
   {
      CflatSTLString mName;

      Symbol(const char* pName)
      {
         mName = CflatSTLString(pName);
      }
   };

   struct Type : Symbol
   {
      TypeCategory mCategory;

      virtual ~Type()
      {
      }

      virtual size_t getSize() const = 0;

   protected:
      Type(const char* pName)
         : Symbol(pName)
      {
      }
   };

   struct TypeRef
   {
      Type* mType;
      uint16_t mArraySize;
      uint8_t mFlags;

      TypeRef()
         : mType(nullptr)
         , mArraySize(1)
         , mFlags(0)
      {
      }

      size_t getSize() const
      {
         if((mFlags & (uint8_t)TypeRefFlags::Pointer) > 0 ||
            (mFlags & (uint8_t)TypeRefFlags::Reference) > 0)
         {
            return sizeof(void*);
         }

         return mType ? mType->getSize() * mArraySize : 0;
      }

      bool operator==(const TypeRef& pOther) const
      {
         return
            mType == pOther.mType &&
            mArraySize == pOther.mArraySize &&
            mFlags == pOther.mFlags;
      }
   };

   struct Member : Symbol
   {
      TypeRef mTypeRef;
      uint16_t mOffset;
      Visibility mVisibility;

      Member(const char* pName)
         : Symbol(pName)
         , mOffset(0)
         , mVisibility(Visibility::Public)
      {
      }
   };

   struct Value
   {
      TypeRef mTypeRef;
      char* mValueBuffer;

      Value(const TypeRef& pTypeRef)
         : mTypeRef(pTypeRef)
      {
         CflatAssert(mTypeRef.mType);
         mValueBuffer = (char*)CflatMalloc(mTypeRef.getSize());
      }
      Value(const TypeRef& pTypeRef, const void* pDataSource)
         : mTypeRef(pTypeRef)
      {
         CflatAssert(mTypeRef.mType);
         mValueBuffer = (char*)CflatMalloc(mTypeRef.getSize());
         set(pDataSource);
      }
      Value(const Value& pOther)
         : mTypeRef(pOther.mTypeRef)
      {
         mTypeRef = pOther.mTypeRef;
         mValueBuffer = (char*)CflatMalloc(mTypeRef.getSize());
         memcpy(mValueBuffer, pOther.mValueBuffer, mTypeRef.getSize());
      }
      ~Value()
      {
         CflatFree(mValueBuffer);
      }

      void set(const void* pDataSource)
      {
         CflatAssert(pDataSource);
         memcpy(mValueBuffer, pDataSource, mTypeRef.getSize());
      }
      template<typename T>
      T getAs()
      {
         return *(reinterpret_cast<T*>(mValueBuffer));
      }

   private:
      Value& operator=(const Value& pOther);
   };

   struct Function : Symbol
   {
      TypeRef mReturnTypeRef;
      CflatSTLVector<TypeRef> mParameters;
      std::function<void(CflatSTLVector<Value>&, Value*)> execute;

      Function(const char* pName)
         : Symbol(pName)
         , execute(nullptr)
      {
      }
   };


   struct BuiltInType : Type
   {
      size_t mSize;

      BuiltInType(const char* pName)
         : Type(pName)
         , mSize(0u)
      {
         mCategory = TypeCategory::BuiltIn;
      }

      virtual size_t getSize() const override
      {
         return mSize;
      }
   };

   struct Struct : Type
   {
      CflatSTLVector<Member> mMembers;
      CflatSTLVector<Function> mMethods;

      Struct(const char* pName)
         : Type(pName)
      {
         mCategory = TypeCategory::Struct;
      }

      virtual size_t getSize() const override
      {
         if(!mMembers.empty())
         {
            const Member& lastMember = mMembers.back();
            return (size_t)lastMember.mOffset + lastMember.mTypeRef.getSize();
         }

         return 0u;
      }
   };

   struct Class : Struct
   {
      Class(const char* pName)
         : Struct(pName)
      {
         mCategory = TypeCategory::Class;
      }
   };


   class Environment
   {
   private:
      typedef CflatSTLMap<uint32_t, Type*> TypesRegistry;
      TypesRegistry mRegisteredTypes;

      typedef CflatSTLMap<uint32_t, CflatSTLVector<Function*>> FunctionsRegistry;
      FunctionsRegistry mRegisteredFunctions;

      static uint32_t hash(const char* pString);

      void registerBuiltInTypes();
      void registerStandardFunctions();

   public:
      Environment();
      ~Environment();

      template<typename T>
      T* registerType(const char* pName)
      {
         const uint32_t nameHash = hash(pName);
         CflatAssert(mRegisteredTypes.find(nameHash) == mRegisteredTypes.end());
         T* type = new T(pName);
         mRegisteredTypes[nameHash] = type;
         return type;
      }
      Type* getType(const char* pName);

      Function* registerFunction(const char* pName);
      Function* getFunction(const char* pName);
      CflatSTLVector<Function*>* getFunctions(const char* pName);
   };
}



//
//  Type definition: Built-in types
//
#define CflatRegisterBuiltInType(pTypeName, pEnvironmentPtr) \
   BuiltInType* type = (pEnvironmentPtr)->registerType<BuiltInType>(#pTypeName); \
   type->mSize = sizeof(pTypeName); \


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pTypeName, pEnvironmentPtr) \
   Cflat::Struct* type = (pEnvironmentPtr)->registerType<Cflat::Struct>(#pTypeName);

#define CflatStructAddMember(pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member = Cflat::Member(#pMemberName); \
      member.mTypeRef.mType = env.getType(#pMemberTypeName); \
      CflatAssert(member.mTypeRef.mType); \
      member.mTypeRef.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pStructTypeName, pMemberName); \
      type->mMembers.push_back(member); \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pTypeName, pEnvironmentPtr) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName);

#define CflatClassAddMember(pClassTypeName, pMemberTypeName, pMemberName) \
   CflatStructAddMember(pClassTypeName, pMemberTypeName, pMemberName)


//
//  Type definition: Functions
//
#define CflatRegisterFunction(pFunctionName, pEnvironmentPtr) \
   Cflat::Function* function = pEnvironmentPtr->registerFunction(#pFunctionName);

#define CflatFunctionDefineReturnType(pFunctionName, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      function->mReturnTypeRef = returnType; \
   }
#define CflatFunctionDefineReturnTypeConstPtr(pFunctionName, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      returnType.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Pointer; \
      function->mReturnTypeRef = returnType; \
   }
#define CflatFunctionDefineReturnTypeConstRef(pFunctionName, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      returnType.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Reference; \
      function->mReturnTypeRef = returnType; \
   }

#define CflatFunctionAddParameter(pFunctionName, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      function->mParameters.push_back(parameter); \
   }
#define CflatFunctionAddParameterConstPtr(pFunctionName, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      parameter.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Pointer; \
      function->mParameters.push_back(parameter); \
   }
#define CflatFunctionAddParameterConstRef(pFunctionName, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      parameter.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Reference; \
      function->mParameters.push_back(parameter); \
   }

#define CflatFunctionDefineExecutionVoid(pFunctionName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatFunctionDefineExecutionVoidParams1(pFunctionName, \
   pParam0TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
         ); \
      }; \
   }
#define CflatFunctionDefineExecutionVoidParams2(pFunctionName, \
   pParam0TypeName, pParam1TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
            pParameters[1].getAs<pParam1TypeName>() \
         ); \
      }; \
   }
#define CflatFunctionDefineExecutionVoidParams3(pFunctionName, \
   pParam0TypeName, pParam1TypeName, pParam2TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
            pParameters[1].getAs<pParam1TypeName>() \
            pParameters[2].getAs<pParam2TypeName>() \
         ); \
      }; \
   }

#define CflatFunctionDefineExecutionReturn(pFunctionName, pReturnTypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatFunctionDefineExecutionReturnParams1(pFunctionName, pReturnTypeName, \
   pParam0TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName result = pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatFunctionDefineExecutionReturnParams2(pFunctionName, pReturnTypeName, \
   pParam0TypeName, pParam1TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName result = pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
            pParameters[1].getAs<pParam1TypeName>() \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatFunctionDefineExecutionReturnParams3(pFunctionName, pReturnTypeName, \
   pParam0TypeName, pParam1TypeName, pParam2TypeName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName result = pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
            pParameters[1].getAs<pParam1TypeName>() \
            pParameters[2].getAs<pParam2TypeName>() \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }

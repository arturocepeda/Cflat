
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
      static const size_t BufferSize = 16u;

      TypeRef mTypeRef;
      char mValueBuffer[BufferSize];

      Value(const TypeRef& pTypeRef)
         : mTypeRef(pTypeRef)
      {
         CflatAssert(mTypeRef.mType);
      }
      Value(const TypeRef& pTypeRef, const void* pDataSource)
         : mTypeRef(pTypeRef)
      {
         CflatAssert(mTypeRef.mType);
         set(pDataSource);
      }
      ~Value()
      {
      }

      void set(const void* pDataSource)
      {
         CflatAssert(pDataSource);
         CflatAssert(mTypeRef.getSize() <= BufferSize);
         memcpy(mValueBuffer, pDataSource, mTypeRef.getSize());
      }
      template<typename T>
      T getAs()
      {
         return *(reinterpret_cast<T*>(mValueBuffer));
      }
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

      typedef CflatSTLMap<uint32_t, Function*> FunctionsRegistry;
      FunctionsRegistry mRegisteredFunctions;

      static uint32_t hash(const char* pString);

      void registerBuiltInTypes();
      void registerStandardFunctions();

   public:
      Environment();
      ~Environment();

      void registerType(Type* pType);
      Type* getType(const char* pName);

      void registerFunction(Function* pFunction);
      Function* getFunction(const char* pName);
   };
}


#define CflatAddMember(pCflatOwnerTypePtr, pOwnerTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member = Cflat::Member(#pMemberName); \
      member.mTypeRef.mType = env.getType(#pMemberTypeName); \
      CflatAssert(member.mTypeRef.mType); \
      member.mTypeRef.mArraySize = (uint16_t)(sizeof(pOwnerTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pOwnerTypeName, pMemberName); \
      pCflatOwnerTypePtr->mMembers.push_back(member); \
   }

#define CflatDefineReturnType(pCflatFunctionPtr, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      pCflatFunctionPtr->mReturnTypeRef = returnType; \
   }
#define CflatDefineReturnTypeConstPtr(pCflatFunctionPtr, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      returnType.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Pointer; \
      pCflatFunctionPtr->mReturnTypeRef = returnType; \
   }
#define CflatDefineReturnTypeConstRef(pCflatFunctionPtr, pReturnTypeName) \
   { \
      Cflat::TypeRef returnType; \
      returnType.mType = getType(#pReturnTypeName); \
      returnType.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Reference; \
      pCflatFunctionPtr->mReturnTypeRef = returnType; \
   }

#define CflatAddParameter(pCflatFunctionPtr, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      pCflatFunctionPtr->mParameters.push_back(parameter); \
   }
#define CflatAddParameterConstPtr(pCflatFunctionPtr, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      parameter.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Pointer; \
      pCflatFunctionPtr->mParameters.push_back(parameter); \
   }
#define CflatAddParameterConstRef(pCflatFunctionPtr, pParameterTypeName) \
   { \
      Cflat::TypeRef parameter; \
      parameter.mType = getType(#pParameterTypeName); \
      parameter.mFlags = (uint8_t)TypeRefFlags::Const | (uint8_t)TypeRefFlags::Reference; \
      pCflatFunctionPtr->mParameters.push_back(parameter); \
   }

#define CflatDefineExecutionVoid(pCflatFunctionPtr, pFunctionName) \
   { \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatDefineExecutionVoidParams1(pCflatFunctionPtr, pFunctionName, \
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

#define CflatDefineExecutionReturn(pCflatFunctionPtr, pFunctionName, pReturnTypeName) \
   { \
      CflatDefineReturnType(pCflatFunctionPtr, pReturnTypeName); \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pReturnTypeName result = pFunctionName(); \
         *pOutReturnValue = Value(function->mReturnTypeRef, &result); \
      }; \
   }
#define CflatDefineExecutionReturnParams1(pCflatFunctionPtr, pFunctionName, pReturnTypeName, \
   pParam0TypeName) \
   { \
      CflatDefineReturnType(pCflatFunctionPtr, pReturnTypeName); \
      function->execute = [function](CflatSTLVector<Value>& pParameters, Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pReturnTypeName result = pFunctionName \
         ( \
            pParameters[0].getAs<pParam0TypeName>() \
         ); \
         *pOutReturnValue = Value(function->mReturnTypeRef, &result); \
      }; \
   }


////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.10
//  C++ compatible Scripting Language
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
#include <type_traits>

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
         , mFlags(0u)
      {
      }

      size_t getSize() const
      {
         if((mFlags & (uint8_t)TypeRefFlags::Pointer) > 0u ||
            (mFlags & (uint8_t)TypeRefFlags::Reference) > 0u)
         {
            return sizeof(void*);
         }

         return mType ? mType->getSize() * mArraySize : 0u;
      }

      bool isPointer() const
      {
         return (mFlags & (uint8_t)TypeRefFlags::Pointer) > 0u;
      }
      bool isReference() const
      {
         return (mFlags & (uint8_t)TypeRefFlags::Reference) > 0u;
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

         if(mTypeRef.isReference())
         {
            memcpy(mValueBuffer, &pDataSource, mTypeRef.getSize());
         }
         else
         {
            memcpy(mValueBuffer, pDataSource, mTypeRef.getSize());
         }
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

   struct Method : Symbol
   {
      TypeRef mReturnTypeRef;
      CflatSTLVector<TypeRef> mParameters;
      std::function<void(Value&, CflatSTLVector<Value>&, Value*)> execute;

      Method(const char* pName)
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
      CflatSTLVector<Method> mMethods;

      Struct(const char* pName)
         : Type(pName)
      {
         mCategory = TypeCategory::Struct;
      }

      virtual size_t getSize() const override
      {
         size_t size = 0u;

         for(size_t i = 0u; i < mMembers.size(); i++)
         {
            const Member& member = mMembers[i];
            const size_t sizeAtMember = (size_t)member.mOffset + member.mTypeRef.getSize();

            if(sizeAtMember > size)
            {
               size = sizeAtMember;
            }
         }

         return size;
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

      TypeRef getTypeRef(const char* pTypeName);

      Function* registerFunction(const char* pName);
      Function* getFunction(const char* pName);
      CflatSTLVector<Function*>* getFunctions(const char* pName);
   };
}



//
//  Value retrieval
//
#define CflatRetrieveValue(pValuePtr, pTypeName,pRef,pPtr) \
   (pPtr *(reinterpret_cast<pTypeName* pPtr>((pValuePtr)->mValueBuffer)))


//
//  Type definition: Built-in types
//
#define CflatRegisterBuiltInType(pEnvironmentPtr, pTypeName) \
   BuiltInType* type = (pEnvironmentPtr)->registerType<BuiltInType>(#pTypeName); \
   type->mSize = sizeof(pTypeName); \


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pEnvironmentPtr, pTypeName) \
   Cflat::Struct* type = (pEnvironmentPtr)->registerType<Cflat::Struct>(#pTypeName);

#define CflatStructAddMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member(#pMemberName); \
      member.mTypeRef = (pEnvironmentPtr)->getTypeRef(#pMemberTypeName); \
      CflatAssert(member.mTypeRef.mType); \
      member.mTypeRef.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pStructTypeName, pMemberName); \
      type->mMembers.push_back(member); \
   }

#define CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      Cflat::Method method(#pMethodName); \
      type->mMethods.push_back(method); \
   }

#define CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
      }; \
   }
#define CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
      pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeRef(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName \
         ( \
            CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }

#define CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeRef = (pEnvironmentPtr)->getTypeRef(#pReturnTypeName #pReturnRef); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == method->mReturnTypeRef); \
         pReturnTypeName pReturnRef result = CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }



//
//  Type definition: Classes
//
#define CflatRegisterClass(pEnvironmentPtr, pTypeName) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName);

#define CflatClassAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   CflatStructAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName)


//
//  Type definition: Functions
//
#define CflatRegisterFunction(pEnvironmentPtr, pFunctionName) \
   Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName);

#define CflatFunctionDefineVoid(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName) \
   { \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatFunctionDefineVoidParams1(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeRef(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName \
         ( \
            CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }

#define CflatFunctionDefineReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName) \
   { \
      function->mReturnTypeRef = (pEnvironmentPtr)->getTypeRef(#pReturnTypeName #pReturnRef); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatFunctionDefineReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      function->mReturnTypeRef = (pEnvironmentPtr)->getTypeRef(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeRef(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeRef == function->mReturnTypeRef); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }

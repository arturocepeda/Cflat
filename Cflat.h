
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

#if !defined (CflatMalloc)
# define CflatMalloc ::malloc
#endif

#if !defined (CflatFree)
# define CflatFree ::free
#endif

#if !defined (CflatAssert)
# include <cassert>
# define CflatAssert assert
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

#define CflatHasFlag(pBitMask, pFlag)  ((pBitMask & (int)pFlag) > 0)
#define CflatSetFlag(pBitMask, pFlag)  (pBitMask |= (int)pFlag)
#define CflatResetFlag(pBitMask, pFlag)  (pBitMask &= ~((int)pFlag))

#define CflatInvokeCtor(pClassName, pPtr)  new (pPtr) pClassName
#define CflatInvokeDtor(pClassName, pPtr)  pPtr->~pClassName();

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

   enum class TypeUsageFlags : uint8_t
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

   struct TypeUsage
   {
      Type* mType;
      uint16_t mArraySize;
      uint8_t mFlags;

      TypeUsage()
         : mType(nullptr)
         , mArraySize(1u)
         , mFlags(0u)
      {
      }

      size_t getSize() const
      {
         if(CflatHasFlag(mFlags, TypeUsageFlags::Pointer) ||
            CflatHasFlag(mFlags, TypeUsageFlags::Reference))
         {
            return sizeof(void*);
         }

         return mType ? mType->getSize() * mArraySize : 0u;
      }

      bool isPointer() const
      {
         return CflatHasFlag(mFlags, TypeUsageFlags::Pointer);
      }
      bool isReference() const
      {
         return CflatHasFlag(mFlags, TypeUsageFlags::Reference);
      }

      bool operator==(const TypeUsage& pOther) const
      {
         return
            mType == pOther.mType &&
            mArraySize == pOther.mArraySize &&
            mFlags == pOther.mFlags;
      }
   };

   struct Member : Symbol
   {
      TypeUsage mTypeUsage;
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
      TypeUsage mTypeUsage;
      char* mValueBuffer;

      Value(const TypeUsage& pTypeUsage)
         : mTypeUsage(pTypeUsage)
      {
         CflatAssert(mTypeUsage.mType);
         const size_t typeUsageSize = mTypeUsage.getSize();
         mValueBuffer = (char*)CflatMalloc(typeUsageSize);
         memset(mValueBuffer, 0, typeUsageSize);
      }
      Value(const TypeUsage& pTypeUsage, const void* pDataSource)
         : mTypeUsage(pTypeUsage)
      {
         CflatAssert(mTypeUsage.mType);
         mValueBuffer = (char*)CflatMalloc(mTypeUsage.getSize());
         set(pDataSource);
      }
      Value(const Value& pOther)
         : mTypeUsage(pOther.mTypeUsage)
      {
         mTypeUsage = pOther.mTypeUsage;
         mValueBuffer = (char*)CflatMalloc(mTypeUsage.getSize());
         memcpy(mValueBuffer, pOther.mValueBuffer, mTypeUsage.getSize());
      }
      ~Value()
      {
         CflatFree(mValueBuffer);
      }

      void set(const void* pDataSource)
      {
         CflatAssert(pDataSource);

         if(mTypeUsage.isReference())
         {
            memcpy(mValueBuffer, &pDataSource, mTypeUsage.getSize());
         }
         else
         {
            memcpy(mValueBuffer, pDataSource, mTypeUsage.getSize());
         }
      }

   private:
      Value& operator=(const Value& pOther);
   };

   struct Function : Symbol
   {
      TypeUsage mReturnTypeUsage;
      CflatSTLVector<TypeUsage> mParameters;
      std::function<void(CflatSTLVector<Value>&, Value*)> execute;

      Function(const char* pName)
         : Symbol(pName)
         , execute(nullptr)
      {
      }
   };

   struct Method : Symbol
   {
      TypeUsage mReturnTypeUsage;
      Visibility mVisibility;
      CflatSTLVector<TypeUsage> mParameters;
      std::function<void(Value&, CflatSTLVector<Value>&, Value*)> execute;

      Method(const char* pName)
         : Symbol(pName)
         , mVisibility(Visibility::Public)
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
            const size_t sizeAtMember = (size_t)member.mOffset + member.mTypeUsage.getSize();

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
   

   enum class TokenType
   {
      Punctuation,
      Number,
      String,
      Keyword,
      Identifier,
      Operator
   };

   struct Token
   {
      TokenType mType;
      char* mStart;
      size_t mLength;
      uint16_t mLine;
   };


   struct Expression;
   struct Statement;
   struct StatementBlock;


   class Environment
   {
   private:
      static const size_t StackSize = 16384u;

      struct ParsingContext
      {
         CflatSTLString mPreprocessedCode;
         CflatSTLVector<Token> mTokens;
         size_t mTokenIndex;
         CflatSTLString mStringBuffer;
         CflatSTLVector<CflatSTLString> mUsingNamespaces;
         CflatSTLString mErrorMessage;
      };

      struct Stack
      {
         char mMemory[StackSize];
         char* mPointer;

         Stack()
            : mPointer(mMemory)
         {
         }
      };

      struct Instance
      {
         TypeUsage mTypeUsage;
         uint32_t mNameHash;
         uint32_t mScopeLevel;
         Value mValue;
      };

      struct ExecutionContext
      {
         Stack mStack;
         uint32_t mScopeLevel;
         CflatSTLVector<Instance> mInstances;
      };

      typedef CflatSTLMap<uint32_t, Type*> TypesRegistry;
      TypesRegistry mRegisteredTypes;

      typedef CflatSTLMap<uint32_t, CflatSTLVector<Function*>> FunctionsRegistry;
      FunctionsRegistry mRegisteredFunctions;

      static uint32_t hash(const char* pString);
      static const char* findClosure(const char* pCode, char pOpeningChar, char pClosureChar);

      void registerBuiltInTypes();
      void registerStandardFunctions();

      Type* getType(uint32_t pNameHash);
      Function* getFunction(uint32_t pNameHash);
      CflatSTLVector<Function*>* getFunctions(uint32_t pNameHash);

      TypeUsage parseTypeUsage(ParsingContext& pContext);
      void throwCompileError(ParsingContext& pContext, const char* pErrorMsg, uint16_t pLineNumber);

      void preprocess(ParsingContext& pContext, const char* pCode);
      void tokenize(ParsingContext& pContext);
      void parse(ParsingContext& pContext, StatementBlock* pOutAST);

      Expression* parseExpression(ParsingContext& pContext);

      Value getValue(Expression* pExpression);
      void execute(Statement* pStatement);

   public:
      Environment();
      ~Environment();

      template<typename T>
      T* registerType(const char* pName)
      {
         const uint32_t nameHash = hash(pName);
         CflatAssert(mRegisteredTypes.find(nameHash) == mRegisteredTypes.end());
         T* type = (T*)CflatMalloc(sizeof(T));
         CflatInvokeCtor(T, type)(pName);
         mRegisteredTypes[nameHash] = type;
         return type;
      }
      Type* getType(const char* pName);

      TypeUsage getTypeUsage(const char* pTypeName);

      Function* registerFunction(const char* pName);
      Function* getFunction(const char* pName);
      CflatSTLVector<Function*>* getFunctions(const char* pName);

      void load(const char* pCode);
   };
}




//
//  Type definition: Built-in types
//
#define CflatRegisterBuiltInType(pEnvironmentPtr, pTypeName) \
   { \
      Cflat::BuiltInType* type = (pEnvironmentPtr)->registerType<Cflat::BuiltInType>(#pTypeName); \
      type->mSize = sizeof(pTypeName); \
   }


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pEnvironmentPtr, pTypeName) \
   Cflat::Struct* type = (pEnvironmentPtr)->registerType<Cflat::Struct>(#pTypeName);

#define CflatStructAddMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member(#pMemberName); \
      member.mTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberTypeName); \
      CflatAssert(member.mTypeUsage.mType); \
      member.mTypeUsage.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pStructTypeName, pMemberName); \
      type->mMembers.push_back(member); \
   }
#define CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefine(pEnvironmentPtr, pStructTypeName); \
   }
#define CflatStructAddConstructorParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr); \
   }
#define CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName) \
   { \
      _CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName); \
   }
#define CflatStructAddMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName); \
   }
#define CflatStructAddMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr); \
   }
#define CflatStructAddMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName); \
   }
#define CflatStructAddMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr); \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pEnvironmentPtr, pTypeName) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName);

#define CflatClassAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   { \
      CflatStructAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   }
#define CflatClassAddConstructor(pEnvironmentPtr, pClassTypeName) \
   { \
      CflatStructAddConstructor(pEnvironmentPtr, pClassTypeName) \
   }
#define CflatClassAddConstructorParams1(pEnvironmentPtr, pClassTypeName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatStructAddConstructorParams1(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }
#define CflatClassAddDestructor(pEnvironmentPtr, pClassTypeName) \
   { \
      CflatStructAddDestructor(pEnvironmentPtr, pClassTypeName) \
   }
#define CflatClassAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   { \
      CflatStructAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   }
#define CflatClassAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatStructAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }
#define CflatClassAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      CflatStructAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   }
#define CflatClassAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatStructAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }


//
//  Type definition: Functions
//
#define CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         pFunctionName \
         ( \
            _CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == function->mReturnTypeUsage); \
         pReturnTypeName result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pParameters.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == function->mReturnTypeUsage); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            _CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }



//
//  Internal macros - helpers for the user macros
//
#define _CflatRetrieveValue(pValuePtr, pTypeName,pRef,pPtr) \
   (pPtr *(reinterpret_cast<pTypeName* pPtr>((pValuePtr)->mValueBuffer)))
#define _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName) \
   { \
      Cflat::Method method(#pStructTypeName); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName) \
   { \
      Cflat::Method method("~" #pStructTypeName); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      Cflat::Method method(#pMethodName); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructConstructorDefine(pEnvironmentPtr, pStructTypeName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         new (_CflatRetrieveValue(&pThis, pStructTypeName*,,)) pStructTypeName(); \
      }; \
   }
#define _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         new (_CflatRetrieveValue(&pThis, pStructTypeName*,,)) pStructTypeName \
         ( \
            _CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         _CflatRetrieveValue(&pThis, pStructTypeName*,,)->~pStructTypeName(); \
      }; \
   }
#define _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         _CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         _CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName \
         ( \
            _CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == method->mReturnTypeUsage); \
         pReturnTypeName pReturnRef result = _CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
      pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pParameters, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == method->mReturnTypeUsage); \
         CflatAssert(method->mParameters.size() == pParameters.size()); \
         pReturnTypeName pReturnRef result = _CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName \
         ( \
            _CflatRetrieveValue(&pParameters[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }

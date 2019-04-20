
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
#define CflatInvokeDtor(pClassName, pPtr)  pPtr->~pClassName()

namespace Cflat
{
   enum class TypeCategory : uint8_t
   {
      BuiltIn,
      Struct,
      Class
   };

   enum class TypeUsageFlags : uint8_t
   {
      Const      = 1 << 0,
      Reference  = 1 << 1
   };
   

   uint32_t hash(const char* pString);


   struct Identifier
   {
      static const size_t kNameLength = 60u;

      char mName[kNameLength];
      uint32_t mHash;

      Identifier(const char* pName)
      {
         CflatAssert(strlen(pName) < kNameLength);
         strcpy(mName, pName);
         mHash = hash(pName);
      }

      bool operator==(const Identifier& pOther) const
      {
         return mHash == pOther.mHash;
      }
   };

   struct Type
   {
      Identifier mIdentifier;
      size_t mSize;
      TypeCategory mCategory;

      virtual ~Type()
      {
      }

   protected:
      Type(const Identifier& pIdentifier)
         : mIdentifier(pIdentifier)
         , mSize(0u)
      {
      }
   };

   struct TypeUsage
   {
      Type* mType;
      uint16_t mArraySize;
      uint8_t mPointerLevel;
      uint8_t mFlags;

      TypeUsage()
         : mType(nullptr)
         , mArraySize(1u)
         , mPointerLevel(0u)
         , mFlags(0u)
      {
      }

      size_t getSize() const
      {
         if(mPointerLevel > 0u)
         {
            return sizeof(void*);
         }

         return mType ? mType->mSize * mArraySize : 0u;
      }

      bool isPointer() const
      {
         return mPointerLevel > 0u;
      }
      bool isConst() const
      {
         return CflatHasFlag(mFlags, TypeUsageFlags::Const);
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

   struct Member
   {
      Identifier mIdentifier;
      TypeUsage mTypeUsage;
      uint16_t mOffset;

      Member(const char* pName)
         : mIdentifier(pName)
         , mOffset(0)
      {
      }
   };


   template<size_t Size>
   struct StackMemoryPool
   {
      char mMemory[Size];
      char* mPointer;

      StackMemoryPool()
         : mPointer(mMemory)
      {
      }

      void reset()
      {
         mPointer = mMemory;
      }

      const char* push(size_t pSize)
      {
         CflatAssert((mPointer - mMemory + pSize) < Size);

         const char* dataPtr = mPointer;
         mPointer += pSize;

         return dataPtr;
      }
      const char* push(const char* pData, size_t pSize)
      {
         CflatAssert((mPointer - mMemory + pSize) < Size);
         memcpy(mPointer, pData, pSize);

         const char* dataPtr = mPointer;
         mPointer += pSize;

         return dataPtr;
      }
      void pop(size_t pSize)
      {
         mPointer -= pSize;
         CflatAssert(mPointer >= mMemory);
      }
   };

   typedef StackMemoryPool<8192u> EnvironmentStack;


   enum class ValueBufferType : uint8_t
   {
      Uninitialized,
      Stack,    // owned, allocated on the stack
      Heap,     // owned, allocated on the heap
      External  // not owned
   };

   struct Value
   {
      TypeUsage mTypeUsage;
      ValueBufferType mValueBufferType;
      char* mValueBuffer;
      EnvironmentStack* mStack;

      Value()
         : mValueBufferType(ValueBufferType::Uninitialized)
         , mValueBuffer(nullptr)
         , mStack(nullptr)
      {
      }
      Value(const Value& pOther)
         : mTypeUsage(pOther.mTypeUsage)
         , mValueBufferType(ValueBufferType::Uninitialized)
         , mValueBuffer(pOther.mValueBuffer)
         , mStack(pOther.mStack)
      {
         if(pOther.mValueBufferType != ValueBufferType::Uninitialized)
         {
            mValueBufferType = ValueBufferType::External;
         }
      }
      ~Value()
      {
         if(mValueBufferType == ValueBufferType::Stack)
         {
            CflatAssert(mStack);
            mStack->pop(mTypeUsage.getSize());
         }
         else if(mValueBufferType == ValueBufferType::Heap)
         {
            CflatAssert(mValueBuffer);
            CflatFree(mValueBuffer);
         }
      }

      void initOnStack(const TypeUsage& pTypeUsage, EnvironmentStack* pStack)
      {
         CflatAssert(mValueBufferType == ValueBufferType::Uninitialized);
         CflatAssert(pStack);

         mTypeUsage = pTypeUsage;
         mValueBufferType = ValueBufferType::Stack;
         mValueBuffer = (char*)pStack->push(pTypeUsage.getSize());
         mStack = pStack;
      }
      void initOnHeap(const TypeUsage& pTypeUsage)
      {
         CflatAssert(mValueBufferType == ValueBufferType::Uninitialized ||
            mValueBufferType == ValueBufferType::Heap);

         const bool allocationRequired =
            mValueBufferType == ValueBufferType::Uninitialized ||
            mTypeUsage.getSize() != pTypeUsage.getSize();

         if(allocationRequired && mValueBuffer)
         {
            CflatFree(mValueBuffer);
            mValueBuffer = nullptr;
         }

         mTypeUsage = pTypeUsage;
         mValueBufferType = ValueBufferType::Heap;

         if(allocationRequired)
         {
            mValueBuffer = (char*)CflatMalloc(pTypeUsage.getSize());
         }
      }
      void set(const void* pDataSource)
      {
         CflatAssert(mValueBufferType != ValueBufferType::Uninitialized);
         CflatAssert(pDataSource);

         if(mValueBufferType == ValueBufferType::External)
         {
            mValueBuffer = (char*)pDataSource;
         }
         else
         {
            memcpy(mValueBuffer, pDataSource, mTypeUsage.getSize());
         }
      }

      Value& operator=(const Value& pOther)
      {
         if(pOther.mValueBufferType == ValueBufferType::Uninitialized)
         {
            this->~Value();
            mValueBufferType = ValueBufferType::Uninitialized;
            mValueBuffer = nullptr;
         }
         else
         {
            switch(mValueBufferType)
            {
            case ValueBufferType::Uninitialized:
            case ValueBufferType::External:
               mTypeUsage = pOther.mTypeUsage;
               mValueBufferType = ValueBufferType::External;
               mValueBuffer = pOther.mValueBuffer;
               break;
            case ValueBufferType::Heap:
               initOnHeap(pOther.mTypeUsage);
               memcpy(mValueBuffer, pOther.mValueBuffer, mTypeUsage.getSize());
               break;
            case ValueBufferType::Stack:
               CflatAssert(false);
               break;
            }
         }

         return *this;
      }
   };

   struct Function
   {
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      CflatSTLVector<TypeUsage> mParameters;
      std::function<void(CflatSTLVector<Value>&, Value*)> execute;

      Function(const Identifier& pIdentifier)
         : mIdentifier(pIdentifier)
         , execute(nullptr)
      {
      }
   };

   struct Method
   {
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      CflatSTLVector<TypeUsage> mParameters;
      std::function<void(const Value&, CflatSTLVector<Value>&, Value*)> execute;

      Method(const Identifier& pIdentifier)
         : mIdentifier(pIdentifier)
         , execute(nullptr)
      {
      }
   };

   struct Instance
   {
      TypeUsage mTypeUsage;
      Identifier mIdentifier;
      uint32_t mScopeLevel;
      Value mValue;

      Instance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
         : mTypeUsage(pTypeUsage)
         , mIdentifier(pIdentifier)
         , mScopeLevel(0u)
      {
      }
   };

   struct BuiltInType : Type
   {
      BuiltInType(const Identifier& pIdentifier)
         : Type(pIdentifier)
      {
         mCategory = TypeCategory::BuiltIn;
      }
   };

   struct Struct : Type
   {
      CflatSTLVector<Member> mMembers;
      CflatSTLVector<Method> mMethods;

      Struct(const Identifier& pIdentifier)
         : Type(pIdentifier)
      {
         mCategory = TypeCategory::Struct;
      }
   };

   struct Class : Struct
   {
      Class(const Identifier& pIdentifier)
         : Struct(pIdentifier)
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
   struct StatementFunctionDeclaration;
   struct StatementAssignment;
   struct StatementIf;
   struct StatementWhile;
   struct StatementFor;
   struct StatementBreak;
   struct StatementContinue;
   struct StatementReturn;

   struct Program
   {
      char mName[64];
      CflatSTLString mCode;
      CflatSTLVector<Statement*> mStatements;

      ~Program();
   };


   class Environment
   {
   private:
      struct Context
      {
      public:
         uint32_t mScopeLevel;
         EnvironmentStack mStack;
         CflatSTLVector<Instance> mInstances;
         CflatSTLString mStringBuffer;
         CflatSTLString mErrorMessage;

      protected:
         Context()
            : mScopeLevel(0u)
         {
         }
      };

      struct ParsingContext : Context
      {
         CflatSTLString mPreprocessedCode;
         CflatSTLVector<CflatSTLString> mUsingNamespaces;
         CflatSTLVector<Token> mTokens;
         size_t mTokenIndex;

         ParsingContext()
            : mTokenIndex(0u)
         {
         }
      };

      enum class JumpStatement : uint8_t
      {
         None,
         Break,
         Continue,
         Return
      };

      struct ExecutionContext : Context
      {
         uint16_t mCurrentLine;
         Value mReturnValue;
         JumpStatement mJumpStatement;

         ExecutionContext()
            : mCurrentLine(0u)
            , mJumpStatement(JumpStatement::None)
         {
         }
      };

      enum class CompileError : uint8_t
      {
         UnexpectedSymbol,
         UndefinedVariable,
         VariableRedefinition,
         NoDefaultConstructor,
         InvalidMemberAccessOperatorPtr,
         InvalidMemberAccessOperatorNonPtr,
         InvalidOperator,
         MissingMember,
         NonIntegerValue,

         Count
      };
      enum class RuntimeError : uint8_t
      {
         NullPointerAccess,
         InvalidArrayIndex,
         DivisionByZero,

         Count
      };

      typedef CflatSTLMap<uint32_t, Type*> TypesRegistry;
      TypesRegistry mRegisteredTypes;

      typedef CflatSTLMap<uint32_t, CflatSTLVector<Function*>> FunctionsRegistry;
      FunctionsRegistry mRegisteredFunctions;

      typedef CflatSTLMap<uint32_t, Program> ProgramsRegistry;
      ProgramsRegistry mPrograms;

      typedef StackMemoryPool<1024u> LiteralStringsPool;

      LiteralStringsPool mLiteralStringsPool;
      ExecutionContext mExecutionContext;
      CflatSTLString mErrorMessage;

      void registerBuiltInTypes();
      void registerStandardFunctions();

      Type* getType(uint32_t pNameHash);
      Function* getFunction(uint32_t pNameHash);
      CflatSTLVector<Function*>* getFunctions(uint32_t pNameHash);

      TypeUsage parseTypeUsage(ParsingContext& pContext);
      void throwCompileError(ParsingContext& pContext, CompileError pError,
         const char* pArg1 = "", const char* pArg2 = "");

      void preprocess(ParsingContext& pContext, const char* pCode);
      void tokenize(ParsingContext& pContext);
      void parse(ParsingContext& pContext, Program& pProgram);

      Expression* parseExpression(ParsingContext& pContext, size_t pTokenLastIndex);

      size_t findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar);
      TypeUsage getTypeUsage(ParsingContext& pContext, Expression* pExpression);

      Statement* parseStatement(ParsingContext& pContext);
      StatementBlock* parseStatementBlock(ParsingContext& pContext);
      StatementFunctionDeclaration* parseStatementFunctionDeclaration(ParsingContext& pContext);
      StatementAssignment* parseStatementAssignment(ParsingContext& pContext, size_t pOperatorTokenIndex);
      StatementIf* parseStatementIf(ParsingContext& pContext);
      StatementWhile* parseStatementWhile(ParsingContext& pContext);
      StatementFor* parseStatementFor(ParsingContext& pContext);
      StatementBreak* parseStatementBreak(ParsingContext& pContext);
      StatementContinue* parseStatementContinue(ParsingContext& pContext);
      StatementReturn* parseStatementReturn(ParsingContext& pContext);

      bool parseFunctionCallArguments(ParsingContext& pContext, CflatSTLVector<Expression*>& pArguments);
      bool parseMemberAccessIdentifiers(ParsingContext& pContext, CflatSTLVector<Identifier>& pIdentifiers);

      Instance* registerInstance(Context& pContext, const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(Context& pContext, const Identifier& pIdentifier);

      void incrementScopeLevel(Context& pContext);
      void decrementScopeLevel(Context& pContext);

      void throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg = "");

      void getValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getAddressOfValue(ExecutionContext& pContext, Value* pInstanceDataValue, Value* pOutValue);
      void getArgumentValues(ExecutionContext& pContext,
         const CflatSTLVector<Expression*>& pExpressions, CflatSTLVector<Value>& pValues);
      void applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
         const char* pOperator, Value* pOutValue);
      void performAssignment(ExecutionContext& pContext, Value* pValue,
         const char* pOperator, Value* pInstanceDataValue);

      static bool isInteger(const Type& pType);
      static bool isDecimal(const Type& pType);
      static int64_t getValueAsInteger(const Value& pValue);
      static double getValueAsDecimal(const Value& pValue);
      static void setValueAsInteger(int64_t pInteger, Value* pOutValue);
      static void setValueAsDecimal(double pDecimal, Value* pOutValue);

      static Method* getDefaultConstructor(Type* pType);
      static Method* findMethod(Type* pType, const Identifier& pIdentifier);

      void execute(ExecutionContext& pContext, const Program& pProgram);
      void execute(ExecutionContext& pContext, Statement* pStatement);

   public:
      Environment();
      ~Environment();

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         CflatAssert(mRegisteredTypes.find(pIdentifier.mHash) == mRegisteredTypes.end());
         T* type = (T*)CflatMalloc(sizeof(T));
         CflatInvokeCtor(T, type)(pIdentifier);
         mRegisteredTypes[pIdentifier.mHash] = type;
         return type;
      }
      Type* getType(const Identifier& pIdentifier);

      TypeUsage getTypeUsage(const char* pTypeName);

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier);
      CflatSTLVector<Function*>* getFunctions(const Identifier& pIdentifier);

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier);

      bool load(const char* pProgramName, const char* pCode);
      const char* getErrorMessage();
   };
}



//
//  Value retrieval
//
#define CflatRetrieveValue(pValuePtr, pTypeName,pRef,pPtr) \
   (pPtr *(reinterpret_cast<pTypeName* pPtr>((pValuePtr)->mValueBuffer)))


//
//  Function definition
//
#define CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == function->mReturnTypeUsage); \
         pReturnTypeName pReturnRef result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pFunctionName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == function->mReturnTypeUsage); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
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
   Cflat::Struct* type = (pEnvironmentPtr)->registerType<Cflat::Struct>(#pTypeName); \
   type->mSize = sizeof(pTypeName);
#define CflatRegisterDerivedStruct(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   Cflat::Struct* type = (pEnvironmentPtr)->registerType<Cflat::Struct>(#pTypeName); \
   type->mSize = sizeof(pTypeName); \
   Cflat::Struct* baseType = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pBaseTypeName)); \
   CflatAssert(baseType); \
   for(size_t i = 0u; i < baseType->mMembers.size(); i++) type->mMembers.push_back(baseType->mMembers[i]); \
   for(size_t i = 0u; i < baseType->mMethods.size(); i++) type->mMethods.push_back(baseType->mMethods[i]);

#define CflatStructAddMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member(#pMemberName); \
      member.mTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberTypeName); \
      CflatAssert(member.mTypeUsage.mType); \
      member.mTypeUsage.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pStructTypeName, pMemberName); \
      type->mMembers.push_back(member); \
   }
#define CflatStructAddStaticMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::TypeUsage typeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberTypeName); \
      CflatAssert(typeUsage.mType); \
      typeUsage.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      Cflat::Value value(typeUsage, &pStructTypeName::pMemberName); \
      (pEnvironmentPtr)->setVariable(typeUsage, #pStructTypeName "::" #pMemberName, value); \
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
#define CflatStructAddStaticMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   { \
      CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pStructTypeName::pMethodName) \
   }
#define CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef,pVoidPtr, pStructTypeName::pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }
#define CflatStructAddStaticMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pStructTypeName::pMethodName) \
   }
#define CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef,pReturnPtr, pStructTypeName::pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pEnvironmentPtr, pTypeName) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName); \
   type->mSize = sizeof(pTypeName);
#define CflatRegisterDerivedClass(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName); \
   type->mSize = sizeof(pTypeName); \
   Cflat::Class* baseType = static_cast<Cflat::Class*>((pEnvironmentPtr)->getType(#pBaseTypeName)); \
   CflatAssert(baseType); \
   for(size_t i = 0u; i < baseType->mMembers.size(); i++) type->mMembers.push_back(baseType->mMembers[i]); \
   for(size_t i = 0u; i < baseType->mMethods.size(); i++) type->mMethods.push_back(baseType->mMethods[i]);

#define CflatClassAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   { \
      CflatStructAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   }
#define CflatClassAddStaticMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   { \
      CflatStructAddStaticMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
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
#define CflatClassAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   { \
      CflatStructAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName) \
   }
#define CflatClassAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef,pVoidPtr, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }
#define CflatClassAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      CflatStructAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   }
#define CflatClassAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
         pParam0TypeName,pParam0Ref,pParam0Ptr) \
   }



//
//  Internal macros - helpers for the user macros
//
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
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         new (CflatRetrieveValue(&pThis, pStructTypeName*,,)) pStructTypeName(); \
      }; \
   }
#define _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatRetrieveValue(&pThis, pStructTypeName*,,)) pStructTypeName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatRetrieveValue(&pThis, pStructTypeName*,,)->~pStructTypeName(); \
      }; \
   }
#define _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == method->mReturnTypeUsage); \
         pReturnTypeName pReturnRef result = CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef,pReturnPtr, pMethodName, \
      pParam0TypeName,pParam0Ref,pParam0Ptr) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage == method->mReturnTypeUsage); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatRetrieveValue(&pThis, pStructTypeName*,,)->pMethodName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName,pParam0Ref,pParam0Ptr) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }

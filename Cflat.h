
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
      CflatSTLString mName;
      uint32_t mHash;

      Identifier(const char* pName)
         : mName(pName)
      {
         mHash = pName[0] != '\0' ? hash(pName) : 0u;
      }

      bool operator==(const Identifier& pOther) const
      {
         return mHash == pOther.mHash;
      }
      bool operator!=(const Identifier& pOther) const
      {
         return mHash != pOther.mHash;
      }
   };

   class Namespace;

   struct Type
   {
      Namespace* mNamespace;
      Identifier mIdentifier;
      size_t mSize;
      TypeCategory mCategory;

      virtual ~Type()
      {
      }

   protected:
      Type(Namespace* pNamespace, const Identifier& pIdentifier)
         : mNamespace(pNamespace)
         , mIdentifier(pIdentifier)
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

      bool compatibleWith(const TypeUsage& pOther) const
      {
         return
            mType == pOther.mType &&
            mArraySize == pOther.mArraySize &&
            mPointerLevel == pOther.mPointerLevel &&
            isReference() == pOther.isReference();
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
         : mValueBufferType(ValueBufferType::Uninitialized)
         , mValueBuffer(nullptr)
         , mStack(nullptr)
      {
         *this = pOther;
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

      void reset()
      {
         CflatInvokeDtor(Value, this);
         CflatInvokeCtor(Value, this);
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
         CflatAssert(mValueBufferType != ValueBufferType::Stack);

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
      void initExternal(const TypeUsage& pTypeUsage)
      {
         CflatAssert(mValueBufferType == ValueBufferType::Uninitialized);
         mTypeUsage = pTypeUsage;
         mValueBufferType = ValueBufferType::External;
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
            reset();
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
            case ValueBufferType::Stack:
               CflatAssert(mTypeUsage.compatibleWith(pOther.mTypeUsage));
               memcpy(mValueBuffer, pOther.mValueBuffer, mTypeUsage.getSize());
               break;
            case ValueBufferType::Heap:
               initOnHeap(pOther.mTypeUsage);
               memcpy(mValueBuffer, pOther.mValueBuffer, mTypeUsage.getSize());
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
      BuiltInType(Namespace* pNamespace, const Identifier& pIdentifier)
         : Type(pNamespace, pIdentifier)
      {
         mCategory = TypeCategory::BuiltIn;
      }
   };

   struct BaseType
   {
      Type* mType;
      uint16_t mOffset;
   };

   struct Struct : Type
   {
      CflatSTLVector<BaseType> mBaseTypes;
      CflatSTLVector<Member> mMembers;
      CflatSTLVector<Method> mMethods;

      Struct(Namespace* pNamespace, const Identifier& pIdentifier)
         : Type(pNamespace, pIdentifier)
      {
         mCategory = TypeCategory::Struct;
      }
   };

   struct Class : Struct
   {
      Class(Namespace* pNamespace, const Identifier& pIdentifier)
         : Struct(pNamespace, pIdentifier)
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


   class Namespace
   {
   private:
      Identifier mName;
      Identifier mFullName;

      Namespace* mParent;

      typedef CflatSTLMap<uint32_t, Type*> TypesRegistry;
      TypesRegistry mTypes;

      typedef CflatSTLMap<uint32_t, CflatSTLVector<Function*>> FunctionsRegistry;
      FunctionsRegistry mFunctions;

      typedef CflatSTLMap<uint32_t, Namespace*> NamespacesRegistry;
      NamespacesRegistry mNamespaces;

      CflatSTLVector<Instance> mInstances;

      Namespace* getChild(uint32_t pNameHash);

      const char* findFirstSeparator(const char* pString);
      const char* findLastSeparator(const char* pString);

   public:
      Namespace(const Identifier& pName, Namespace* pParent);
      ~Namespace();

      const Identifier& getName() const { return mName; }
      const Identifier& getFullName() const { return mFullName; }
      Namespace* getParent() { return mParent; }

      Namespace* getNamespace(const Identifier& pName);
      Namespace* requestNamespace(const Identifier& pName);

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         const char* lastSeparator = findLastSeparator(pIdentifier.mName.c_str());

         if(lastSeparator)
         {
            char buffer[256];
            const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName.c_str();
            strncpy(buffer, pIdentifier.mName.c_str(), nsIdentifierLength);
            buffer[nsIdentifierLength] = '\0';
            const Identifier nsIdentifier(buffer);
            const Identifier typeIdentifier(lastSeparator + 2);
            return requestNamespace(nsIdentifier)->registerType<T>(typeIdentifier);
         }
         else
         {
            CflatAssert(mTypes.find(pIdentifier.mHash) == mTypes.end());
            T* type = (T*)CflatMalloc(sizeof(T));
            CflatInvokeCtor(T, type)(this, pIdentifier);
            mTypes[pIdentifier.mHash] = type;
            return type;
         }
      }
      Type* getType(const Identifier& pIdentifier);

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier);
      CflatSTLVector<Function*>* getFunctions(const Identifier& pIdentifier);

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier);

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier);
      void releaseInstances(uint32_t pScopeLevel);
   };


   struct Context
   {
   public:
      uint32_t mScopeLevel;
      EnvironmentStack mStack;
      Namespace* mCurrentNamespace;
      CflatSTLVector<Namespace*> mUsingNamespaces;
      CflatSTLString mStringBuffer;
      CflatSTLString mErrorMessage;

   protected:
      Context(Namespace* pGlobalNamespace)
         : mScopeLevel(0u)
         , mCurrentNamespace(pGlobalNamespace)
      {
      }
   };

   struct ParsingContext : Context
   {
      CflatSTLString mPreprocessedCode;
      CflatSTLVector<Token> mTokens;
      size_t mTokenIndex;

      ParsingContext(Namespace* pGlobalNamespace)
         : Context(pGlobalNamespace)
         , mTokenIndex(0u)
      {
      }
   };

   enum class JumpStatement : uint16_t
   {
      None,
      Break,
      Continue,
      Return
   };

   struct ExecutionContext : Context
   {
      Value mReturnValue;
      uint16_t mCurrentLine;
      JumpStatement mJumpStatement;

      ExecutionContext(Namespace* pGlobalNamespace)
         : Context(pGlobalNamespace)
         , mCurrentLine(0u)
         , mJumpStatement(JumpStatement::None)
      {
      }
   };


   class Environment
   {
   private:
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
         UnknownNamespace,

         Count
      };
      enum class RuntimeError : uint8_t
      {
         NullPointerAccess,
         InvalidArrayIndex,
         DivisionByZero,

         Count
      };

      Namespace mGlobalNamespace;

      typedef CflatSTLMap<uint32_t, Program> ProgramsRegistry;
      ProgramsRegistry mPrograms;

      typedef StackMemoryPool<1024u> LiteralStringsPool;

      LiteralStringsPool mLiteralStringsPool;
      ExecutionContext mExecutionContext;
      CflatSTLString mErrorMessage;

      void registerBuiltInTypes();

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
      Instance* retrieveInstance(const Context& pContext, const Identifier& pIdentifier);

      void incrementScopeLevel(Context& pContext);
      void decrementScopeLevel(Context& pContext);

      void throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg = "");

      void getValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getAddressOfValue(ExecutionContext& pContext, Value* pInstanceDataValue, Value* pOutValue);
      void getArgumentValues(ExecutionContext& pContext, const CflatSTLVector<TypeUsage>& pParameters,
         const CflatSTLVector<Expression*>& pExpressions, CflatSTLVector<Value>& pValues);
      void applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
         const char* pOperator, Value* pOutValue);
      void performAssignment(ExecutionContext& pContext, Value* pValue,
         const char* pOperator, Value* pInstanceDataValue);

      static void assertValueInitialization(const TypeUsage& pTypeUsage, Value* pOutValue);

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

      Namespace* getGlobalNamespace() { return &mGlobalNamespace; }

      Namespace* getNamespace(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.getNamespace(pIdentifier);
      }
      Namespace* requestNamespace(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.requestNamespace(pIdentifier);
      }

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.registerType<T>(pIdentifier);
      }
      Type* getType(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.getType(pIdentifier);
      }

      Function* registerFunction(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.registerFunction(pIdentifier);
      }
      Function* getFunction(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.getFunction(pIdentifier);
      }
      CflatSTLVector<Function*>* getFunctions(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.getFunctions(pIdentifier);
      }

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue)
      {
         mGlobalNamespace.setVariable(pTypeUsage, pIdentifier, pValue);
      }
      Value* getVariable(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.getVariable(pIdentifier);
      }

      TypeUsage getTypeUsage(const char* pTypeName);

      bool load(const char* pProgramName, const char* pCode);
      const char* getErrorMessage();
   };
}



//
//  Value retrieval
//
#define CflatRetrieveValue(pValuePtr, pTypeName) \
   (*reinterpret_cast<pTypeName*>((pValuePtr)->mValueBuffer))


//
//  Function definition
//
#define CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName) \
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

#define CflatStructAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   { \
      Cflat::BaseType baseType; \
      baseType.mType = (pEnvironmentPtr)->getType(#pBaseTypeName); \
      CflatAssert(baseType.mType); \
      pTypeName* derivedTypePtr = reinterpret_cast<pTypeName*>(0x1); \
      pBaseTypeName* baseTypePtr = static_cast<pBaseTypeName*>(derivedTypePtr); \
      baseType.mOffset = (uint16_t)((char*)baseTypePtr - (char*)derivedTypePtr); \
      type->mBaseTypes.push_back(baseType); \
      Cflat::Struct* castedBaseType = static_cast<Cflat::Struct*>(baseType.mType); \
      for(size_t i = 0u; i < castedBaseType->mMembers.size(); i++) \
      { \
         type->mMembers.push_back(castedBaseType->mMembers[i]); \
         type->mMembers.back().mOffset += baseType.mOffset; \
      } \
      for(size_t i = 0u; i < castedBaseType->mMethods.size(); i++) \
      { \
         type->mMethods.push_back(castedBaseType->mMethods[i]); \
      } \
   }
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
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName) \
   { \
      _CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName); \
   }
#define CflatStructAddMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName); \
   }
#define CflatStructAddMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName); \
   }
#define CflatStructAddMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddStaticMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef, pStructTypeName::pMethodName) \
   }
#define CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef, pStructTypeName::pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatStructAddStaticMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef, pStructTypeName::pMethodName) \
   }
#define CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef, pStructTypeName::pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pEnvironmentPtr, pTypeName) \
   Cflat::Class* type = (pEnvironmentPtr)->registerType<Cflat::Class>(#pTypeName); \
   type->mSize = sizeof(pTypeName);

#define CflatClassAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   { \
      CflatStructAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   }
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
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddConstructorParams1(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddDestructor(pEnvironmentPtr, pClassTypeName) \
   { \
      CflatStructAddDestructor(pEnvironmentPtr, pClassTypeName) \
   }
#define CflatClassAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatStructAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   }
#define CflatClassAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatStructAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   }
#define CflatClassAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatStructAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   }
#define CflatClassAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatStructAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   }
#define CflatClassAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }



//
//  Internal macros - helpers for the user macros
//
#define _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName) \
   { \
      Cflat::Method method(type->mIdentifier); \
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
         new (CflatRetrieveValue(&pThis, pStructTypeName*)) pStructTypeName(); \
      }; \
   }
#define _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatRetrieveValue(&pThis, pStructTypeName*)) pStructTypeName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName) \
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
         CflatRetrieveValue(&pThis, pStructTypeName*)->~pStructTypeName(); \
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
         CflatRetrieveValue(&pThis, pStructTypeName*)->pMethodName(); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatRetrieveValue(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, CflatSTLVector<Cflat::Value>& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = CflatRetrieveValue(&pThis, pStructTypeName*)->pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
      pParam0TypeName,pParam0Ref) \
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
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatRetrieveValue(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatRetrieveValue(&pArguments[0], pParam0TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }

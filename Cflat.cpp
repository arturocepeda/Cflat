
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.30
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2022 Arturo Cepeda Pérez
//
//  ---------------------------------------------------------------------------
//
//  This software is provided 'as-is', without any express or implied
//  warranty. In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//
///////////////////////////////////////////////////////////////////////////////

#include "Cflat.h"


//
//  Internal definitions
//
namespace Cflat
{
   //
   //  Global functions
   //
   template<typename T>
   inline T min(T pA, T pB)
   {
      return pA < pB ? pA : pB;
   }
   template<typename T>
   inline T max(T pA, T pB)
   {
      return pA > pB ? pA : pB;
   }

   Hash hash(const char* pString)
   {
      static const Hash kOffsetBasis = 2166136261u;
      static const Hash kFNVPrime = 16777619u;

      uint32_t charIndex = 0u;
      Hash hash = kOffsetBasis;

      while(pString[charIndex] != '\0')
      {
         hash ^= pString[charIndex++];
         hash *= kFNVPrime;
      }

      return hash;
   }

   template<typename T>
   void toArgsVector(const CflatSTLVector(T) pSTLVector, CflatArgsVector(T)& pArgsVector)
   {
      const size_t elementsCount = pSTLVector.size();
      pArgsVector.resize(elementsCount);

      if(elementsCount > 0u)
      {
         memcpy(&pArgsVector[0], &pSTLVector[0], elementsCount * sizeof(T));
      }
   }


   //
   //  AST Types
   //
   enum class ExpressionType
   {
      Value,
      NullPointer,
      VariableAccess,
      MemberAccess,
      ArrayElementAccess,
      UnaryOperation,
      BinaryOperation,
      Parenthesized,
      AddressOf,
      Indirection,
      SizeOf,
      Cast,
      Conditional,
      Assignment,
      FunctionCall,
      MethodCall,
      ArrayInitialization,
      ObjectConstruction
   };

   struct Expression
   {
   protected:
      ExpressionType mType;

      Expression()
      {
      }

   public:
      virtual ~Expression()
      {
      }

      ExpressionType getType() const
      {
         return mType;
      }
   };

   struct ExpressionValue : Expression
   {
      Value mValue;

      ExpressionValue(const Value& pValue)
      {
         mType = ExpressionType::Value;

         mValue.initOnHeap(pValue.mTypeUsage);
         mValue.set(pValue.mValueBuffer);
      }
   };

   struct ExpressionNullPointer : Expression
   {
      ExpressionNullPointer()
      {
         mType = ExpressionType::NullPointer;
      }
   };

   struct ExpressionVariableAccess : Expression
   {
      Identifier mVariableIdentifier;

      ExpressionVariableAccess(const Identifier& pVariableIdentifier)
         : mVariableIdentifier(pVariableIdentifier)
      {
         mType = ExpressionType::VariableAccess;
      }
   };

   struct ExpressionMemberAccess : Expression
   {
      Expression* mMemberOwner;
      Identifier mMemberIdentifier;
      TypeUsage mMemberTypeUsage;

      ExpressionMemberAccess(Expression* pMemberOwner, const Identifier& pMemberIdentifier,
         const TypeUsage& pMemberTypeUsage)
         : mMemberOwner(pMemberOwner)
         , mMemberIdentifier(pMemberIdentifier)
         , mMemberTypeUsage(pMemberTypeUsage)
      {
         mType = ExpressionType::MemberAccess;
      }

      virtual ~ExpressionMemberAccess()
      {
         if(mMemberOwner)
         {
            CflatInvokeDtor(Expression, mMemberOwner);
            CflatFree(mMemberOwner);
         }
      }
   };

   struct ExpressionArrayElementAccess : Expression
   {
      Expression* mArray;
      Expression* mArrayElementIndex;

      ExpressionArrayElementAccess(Expression* pArray, Expression* pArrayElementIndex)
         : mArray(pArray)
         , mArrayElementIndex(pArrayElementIndex)
      {
         mType = ExpressionType::ArrayElementAccess;
      }

      virtual ~ExpressionArrayElementAccess()
      {
         if(mArray)
         {
            CflatInvokeDtor(Expression, mArray);
            CflatFree(mArray);
         }

         if(mArrayElementIndex)
         {
            CflatInvokeDtor(Expression, mArrayElementIndex);
            CflatFree(mArrayElementIndex);
         }
      }
   };

   struct ExpressionUnaryOperation : Expression
   {
      Expression* mExpression;
      char mOperator[3];
      bool mPostOperator;

      ExpressionUnaryOperation(Expression* pExpression, const char* pOperator, bool pPostOperator)
         : mExpression(pExpression)
         , mPostOperator(pPostOperator)
      {
         mType = ExpressionType::UnaryOperation;
         strcpy(mOperator, pOperator);
      }

      virtual ~ExpressionUnaryOperation()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionBinaryOperation : Expression
   {
      Expression* mLeft;
      Expression* mRight;
      char mOperator[4];
      TypeUsage mOverloadedOperatorTypeUsage;

      ExpressionBinaryOperation(Expression* pLeft, Expression* pRight, const char* pOperator,
         const TypeUsage& pOverloadedOperatorTypeUsage)
         : mLeft(pLeft)
         , mRight(pRight)
         , mOverloadedOperatorTypeUsage(pOverloadedOperatorTypeUsage)
      {
         mType = ExpressionType::BinaryOperation;
         strcpy(mOperator, pOperator);
      }

      virtual ~ExpressionBinaryOperation()
      {
         if(mLeft)
         {
            CflatInvokeDtor(Expression, mLeft);
            CflatFree(mLeft);
         }

         if(mRight)
         {
            CflatInvokeDtor(Expression, mRight);
            CflatFree(mRight);
         }
      }
   };

   struct ExpressionParenthesized : Expression
   {
      Expression* mExpression;

      ExpressionParenthesized(Expression* pExpression)
         : mExpression(pExpression)
      {
         mType = ExpressionType::Parenthesized;
      }

      virtual ~ExpressionParenthesized()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionAddressOf : Expression
   {
      Expression* mExpression;

      ExpressionAddressOf(Expression* pExpression)
         : mExpression(pExpression)
      {
         mType = ExpressionType::AddressOf;
      }

      virtual ~ExpressionAddressOf()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionIndirection : Expression
   {
      Expression* mExpression;

      ExpressionIndirection(Expression* pExpression)
         : mExpression(pExpression)
      {
         mType = ExpressionType::Indirection;
      }

      virtual ~ExpressionIndirection()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionSizeOf : Expression
   {
      TypeUsage mTypeUsage;
      Expression* mExpression;

      ExpressionSizeOf()
         : mExpression(nullptr)
      {
         mType = ExpressionType::SizeOf;
      }

      virtual ~ExpressionSizeOf()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionCast : Expression
   {
      CastType mCastType;
      TypeUsage mTypeUsage;
      Expression* mExpression;

      ExpressionCast(CastType pCastType, const TypeUsage& pTypeUsage, Expression* pExpression)
         : mCastType(pCastType)
         , mTypeUsage(pTypeUsage)
         , mExpression(pExpression)
      {
         mType = ExpressionType::Cast;
      }

      virtual ~ExpressionCast()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct ExpressionConditional : Expression
   {
      Expression* mCondition;
      Expression* mIfExpression;
      Expression* mElseExpression;

      ExpressionConditional(Expression* pCondition, Expression* pIfExpression,
         Expression* pElseExpression)
         : mCondition(pCondition)
         , mIfExpression(pIfExpression)
         , mElseExpression(pElseExpression)
      {
         mType = ExpressionType::Conditional;
      }

      virtual ~ExpressionConditional()
      {
         if(mCondition)
         {
            CflatInvokeDtor(Expression, mCondition);
            CflatFree(mCondition);
         }

         if(mIfExpression)
         {
            CflatInvokeDtor(Expression, mIfExpression);
            CflatFree(mIfExpression);
         }

         if(mElseExpression)
         {
            CflatInvokeDtor(Expression, mElseExpression);
            CflatFree(mElseExpression);
         }
      }
   };

   struct ExpressionAssignment : Expression
   {
      Expression* mLeftValue;
      Expression* mRightValue;
      char mOperator[4];

      ExpressionAssignment(Expression* pLeftValue, Expression* pRightValue, const char* pOperator)
         : mLeftValue(pLeftValue)
         , mRightValue(pRightValue)
      {
         mType = ExpressionType::Assignment;
         strcpy(mOperator, pOperator);
      }

      virtual ~ExpressionAssignment()
      {
         if(mLeftValue)
         {
            CflatInvokeDtor(Expression, mLeftValue);
            CflatFree(mLeftValue);
         }

         if(mRightValue)
         {
            CflatInvokeDtor(Expression, mRightValue);
            CflatFree(mRightValue);
         }
      }
   };

   struct ExpressionFunctionCall : Expression
   {
      Identifier mFunctionIdentifier;
      CflatSTLVector(Expression*) mArguments;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      Function* mFunction;

      ExpressionFunctionCall(const Identifier& pFunctionIdentifier)
         : mFunctionIdentifier(pFunctionIdentifier)
         , mFunction(nullptr)
      {
         mType = ExpressionType::FunctionCall;
      }

      virtual ~ExpressionFunctionCall()
      {
         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };

   struct ExpressionMethodCall : Expression
   {
      Expression* mMemberAccess;
      CflatSTLVector(Expression*) mArguments;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      Method* mMethod;

      ExpressionMethodCall(Expression* pMemberAccess)
         : mMemberAccess(pMemberAccess)
         , mMethod(nullptr)
      {
         mType = ExpressionType::MethodCall;
      }

      virtual ~ExpressionMethodCall()
      {
         if(mMemberAccess)
         {
            CflatInvokeDtor(Expression, mMemberAccess);
            CflatFree(mMemberAccess);
         }

         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };

   struct ExpressionArrayInitialization : Expression
   {
      Type* mElementType;
      CflatSTLVector(Expression*) mValues;

      ExpressionArrayInitialization()
         : mElementType(nullptr)
      {
         mType = ExpressionType::ArrayInitialization;
      }

      virtual ~ExpressionArrayInitialization()
      {
         for(size_t i = 0u; i < mValues.size(); i++)
         {
            CflatInvokeDtor(Expression, mValues[i]);
            CflatFree(mValues[i]);
         }
      }
   };

   struct ExpressionObjectConstruction : Expression
   {
      Type* mObjectType;
      CflatSTLVector(Expression*) mArguments;
      Method* mConstructor;

      ExpressionObjectConstruction(Type* pObjectType)
         : mObjectType(pObjectType)
         , mConstructor(nullptr)
      {
         mType = ExpressionType::ObjectConstruction;
      }

      virtual ~ExpressionObjectConstruction()
      {
         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };


   enum class StatementType
   {
      Expression,
      Block,
      UsingDirective,
      TypeDefinition,
      NamespaceDeclaration,
      VariableDeclaration,
      FunctionDeclaration,
      StructDeclaration,
      If,
      Switch,
      While,
      DoWhile,
      For,
      ForRangeBased,
      Break,
      Continue,
      Return
   };

   struct Statement
   {
   protected:
      StatementType mType;

      Statement()
         : mProgram(nullptr)
         , mLine(0u)
      {
      }

   public:
      Program* mProgram;
      uint16_t mLine;

      virtual ~Statement()
      {
      }

      StatementType getType() const
      {
         return mType;
      }
   };

   struct StatementExpression : Statement
   {
      Expression* mExpression;

      StatementExpression(Expression* pExpression)
         : mExpression(pExpression)
      {
         mType = StatementType::Expression;
      }

      virtual ~StatementExpression()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };

   struct StatementBlock : Statement
   {
      CflatSTLVector(Statement*) mStatements;
      bool mAlterScope;

      StatementBlock(bool pAlterScope)
         : mAlterScope(pAlterScope)
      {
         mType = StatementType::Block;
      }

      virtual ~StatementBlock()
      {
         for(size_t i = 0u; i < mStatements.size(); i++)
         {
            CflatInvokeDtor(Statement, mStatements[i]);
            CflatFree(mStatements[i]);
         }
      }
   };

   struct StatementUsingDirective : Statement
   {
      Namespace* mNamespace;
      Identifier mAliasIdentifier;
      TypeUsage mAliasTypeUsage;

      StatementUsingDirective(Namespace* pNamespace)
         : mNamespace(pNamespace)
      {
         mType = StatementType::UsingDirective;
      }

      StatementUsingDirective(const Identifier& pAliasIdentifier, const TypeUsage& pAliasTypeUsage)
         : mNamespace(nullptr)
         , mAliasIdentifier(pAliasIdentifier)
         , mAliasTypeUsage(pAliasTypeUsage)
      {
         mType = StatementType::UsingDirective;
      }
   };

   struct StatementTypeDefinition : Statement
   {
      Identifier mAlias;
      TypeUsage mReferencedTypeUsage;

      StatementTypeDefinition(const Identifier& pAlias, const TypeUsage& pReferencedTypeUsage)
         : mAlias(pAlias)
         , mReferencedTypeUsage(pReferencedTypeUsage)
      {
         mType = StatementType::TypeDefinition;
      }
   };

   struct StatementVariableDeclaration : Statement
   {
      TypeUsage mTypeUsage;
      Identifier mVariableIdentifier;
      Expression* mInitialValue;
      bool mStatic;

      StatementVariableDeclaration(const TypeUsage& pTypeUsage, const Identifier& pVariableIdentifier,
         Expression* pInitialValue, bool pStatic)
         : mTypeUsage(pTypeUsage)
         , mVariableIdentifier(pVariableIdentifier)
         , mInitialValue(pInitialValue)
         , mStatic(pStatic)
      {
         mType = StatementType::VariableDeclaration;
      }

      virtual ~StatementVariableDeclaration()
      {
         if(mInitialValue)
         {
            CflatInvokeDtor(Expression, mInitialValue);
            CflatFree(mInitialValue);
         }
      }
   };

   struct StatementNamespaceDeclaration : Statement
   {
      Identifier mNamespaceIdentifier;
      StatementBlock* mBody;

      StatementNamespaceDeclaration(const Identifier& pNamespaceIdentifier)
         : mNamespaceIdentifier(pNamespaceIdentifier)
         , mBody(nullptr)
      {
         mType = StatementType::NamespaceDeclaration;
      }

      virtual ~StatementNamespaceDeclaration()
      {
         if(mBody)
         {
            CflatInvokeDtor(StatementBlock, mBody);
            CflatFree(mBody);
         }
      }
   };

   struct StatementFunctionDeclaration : Statement
   {
      TypeUsage mReturnType;
      Identifier mFunctionIdentifier;
      CflatSTLVector(Identifier) mParameterIdentifiers;
      CflatSTLVector(TypeUsage) mParameterTypes;
      StatementBlock* mBody;
      Function* mFunction;

      StatementFunctionDeclaration(const TypeUsage& pReturnType, const Identifier& pFunctionIdentifier)
         : mReturnType(pReturnType)
         , mFunctionIdentifier(pFunctionIdentifier)
         , mBody(nullptr)
         , mFunction(nullptr)
      {
         mType = StatementType::FunctionDeclaration;
      }

      virtual ~StatementFunctionDeclaration()
      {
         if(mBody)
         {
            CflatInvokeDtor(StatementBlock, mBody);
            CflatFree(mBody);
         }

         if(mFunction && mFunction->mProgram == mProgram)
         {
            mFunction->execute = nullptr;
         }
      }
   };

   struct StatementStructDeclaration : Statement
   {
      Struct* mStruct;

      StatementStructDeclaration()
         : mStruct(nullptr)
      {
         mType = StatementType::StructDeclaration;
      }
   };

   struct StatementIf : Statement
   {
      Expression* mCondition;
      Statement* mIfStatement;
      Statement* mElseStatement;

      StatementIf(Expression* pCondition, Statement* pIfStatement, Statement* pElseStatement)
         : mCondition(pCondition)
         , mIfStatement(pIfStatement)
         , mElseStatement(pElseStatement)
      {
         mType = StatementType::If;
      }

      virtual ~StatementIf()
      {
         if(mCondition)
         {
            CflatInvokeDtor(Expression, mCondition);
            CflatFree(mCondition);
         }

         if(mIfStatement)
         {
            CflatInvokeDtor(Statement, mIfStatement);
            CflatFree(mIfStatement);
         }

         if(mElseStatement)
         {
            CflatInvokeDtor(Statement, mElseStatement);
            CflatFree(mElseStatement);
         }
      }
   };

   struct StatementSwitch : Statement
   {
      struct CaseSection
      {
         Expression* mExpression;
         CflatSTLVector(Statement*) mStatements;
      };

      Expression* mCondition;
      CflatSTLVector(CaseSection) mCaseSections;

      StatementSwitch(Expression* pCondition)
         : mCondition(pCondition)
      {
         mType = StatementType::Switch;
      }

      virtual ~StatementSwitch()
      {
         if(mCondition)
         {
            CflatInvokeDtor(Expression, mCondition);
            CflatFree(mCondition);
         }

         for(size_t i = 0u; i < mCaseSections.size(); i++)
         {
            if(mCaseSections[i].mExpression)
            {
               CflatInvokeDtor(Expression, mCaseSections[i].mExpression);
               CflatFree(mCaseSections[i].mExpression);
            }

            for(size_t j = 0u; j < mCaseSections[i].mStatements.size(); j++)
            {
               if(mCaseSections[i].mStatements[j])
               {
                  CflatInvokeDtor(Statement, mCaseSections[i].mStatements[j]);
                  CflatFree(mCaseSections[i].mStatements[j]);
               }
            }
         }
      }
   };

   struct StatementWhile : Statement
   {
      Expression* mCondition;
      Statement* mLoopStatement;

      StatementWhile(Expression* pCondition, Statement* pLoopStatement)
         : mCondition(pCondition)
         , mLoopStatement(pLoopStatement)
      {
         mType = StatementType::While;
      }

      virtual ~StatementWhile()
      {
         if(mCondition)
         {
            CflatInvokeDtor(Expression, mCondition);
            CflatFree(mCondition);
         }

         if(mLoopStatement)
         {
            CflatInvokeDtor(Statement, mLoopStatement);
            CflatFree(mLoopStatement);
         }
      }
   };

   struct StatementDoWhile : StatementWhile
   {
      StatementDoWhile(Expression* pCondition, Statement* pLoopStatement)
         : StatementWhile(pCondition, pLoopStatement)
      {
         mType = StatementType::DoWhile;
      }
   };

   struct StatementFor : Statement
   {
      Statement* mInitialization;
      Expression* mCondition;
      Expression* mIncrement;
      Statement* mLoopStatement;

      StatementFor(Statement* pInitialization, Expression* pCondition, Expression* pIncrement,
         Statement* pLoopStatement)
         : mInitialization(pInitialization)
         , mCondition(pCondition)
         , mIncrement(pIncrement)
         , mLoopStatement(pLoopStatement)
      {
         mType = StatementType::For;
      }

      virtual ~StatementFor()
      {
         if(mInitialization)
         {
            CflatInvokeDtor(Statement, mInitialization);
            CflatFree(mInitialization);
         }

         if(mCondition)
         {
            CflatInvokeDtor(Expression, mCondition);
            CflatFree(mCondition);
         }

         if(mIncrement)
         {
            CflatInvokeDtor(Expression, mIncrement);
            CflatFree(mIncrement);
         }

         if(mLoopStatement)
         {
            CflatInvokeDtor(Statement, mLoopStatement);
            CflatFree(mLoopStatement);
         }
      }
   };

   struct StatementForRangeBased : Statement
   {
      TypeUsage mVariableTypeUsage;
      Identifier mVariableIdentifier;
      Expression* mCollection;
      Statement* mLoopStatement;

      StatementForRangeBased(const TypeUsage& pVariableTypeUsage, const Identifier& pVariableIdentifier,
         Expression* pCollection, Statement* pLoopStatement)
         : mVariableTypeUsage(pVariableTypeUsage)
         , mVariableIdentifier(pVariableIdentifier)
         , mCollection(pCollection)
         , mLoopStatement(pLoopStatement)
      {
         mType = StatementType::ForRangeBased;
      }

      virtual ~StatementForRangeBased()
      {
         if(mCollection)
         {
            CflatInvokeDtor(Expression, mCollection);
            CflatFree(mCollection);
         }

         if(mLoopStatement)
         {
            CflatInvokeDtor(Statement, mLoopStatement);
            CflatFree(mLoopStatement);
         }
      }
   };

   struct StatementBreak : Statement
   {
      StatementBreak()
      {
         mType = StatementType::Break;
      }
   };

   struct StatementContinue : Statement
   {
      StatementContinue()
      {
         mType = StatementType::Continue;
      }
   };

   struct StatementReturn : Statement
   {
      Expression* mExpression;

      StatementReturn(Expression* pExpression)
         : mExpression(pExpression)
      {
         mType = StatementType::Return;
      }

      virtual ~StatementReturn()
      {
         if(mExpression)
         {
            CflatInvokeDtor(Expression, mExpression);
            CflatFree(mExpression);
         }
      }
   };


   //
   //  Error messages
   //
   const char* kPreprocessorErrorStrings[] = 
   {
      "invalid preprocessor directive",
      "invalid number of arguments for the '%s' macro"
   };
   const size_t kPreprocessorErrorStringsCount = sizeof(kPreprocessorErrorStrings) / sizeof(const char*);

   const char* kCompileErrorStrings[] = 
   {
      "unexpected symbol ('%s')",
      "'%s' expected",
      "undefined type ('%s')",
      "undefined variable ('%s')",
      "undefined function ('%s') or invalid arguments in call",
      "variable redefinition ('%s')",
      "uninitialized reference ('%s')",
      "array initialization expected",
      "no default constructor defined for the '%s' type",
      "invalid literal ('%s')",
      "invalid type ('%s')",
      "invalid assignment",
      "invalid member access operator ('%s' is a pointer)",
      "invalid member access operator ('%s' is not a pointer)",
      "invalid operator for the '%s' type",
      "invalid conditional expression",
      "invalid cast",
      "no member named '%s'",
      "no static member named '%s' in the '%s' type",
      "no constructor matches the given list of arguments",
      "no method named '%s'",
      "no static method named '%s' in the '%s' type",
      "'%s' must be an integer value",
      "unknown namespace ('%s')",
      "cannot modify constant expression"
   };
   const size_t kCompileErrorStringsCount = sizeof(kCompileErrorStrings) / sizeof(const char*);

   const char* kRuntimeErrorStrings[] = 
   {
      "null pointer access ('%s')",
      "invalid array index (%s)",
      "division by zero"
   };
   const size_t kRuntimeErrorStringsCount = sizeof(kRuntimeErrorStrings) / sizeof(const char*);
}


//
//  Memory
//
using namespace Cflat;

void* (*Memory::malloc)(size_t pSize) = ::malloc;
void (*Memory::free)(void* pPtr) = ::free;


//
//  Identifier
//
Identifier::NamesRegistry* Identifier::smNames = nullptr;

Identifier::Identifier()
   : mName(getNamesRegistry()->mMemory)
   , mNameLength(0u)
   , mHash(0u)
{
}

Identifier::Identifier(const char* pName)
{
   mHash = pName[0] != '\0' ? hash(pName) : 0u;
   mName = getNamesRegistry()->registerString(mHash, pName);
   mNameLength = (uint32_t)strlen(mName);
}

Identifier::NamesRegistry* Identifier::getNamesRegistry()
{
   if(!smNames)
   {
      smNames = (NamesRegistry*)CflatMalloc(sizeof(NamesRegistry));
      CflatInvokeCtor(NamesRegistry, smNames);
   }

   return smNames;
}

void Identifier::releaseNamesRegistry()
{
   if(smNames)
   {
      CflatInvokeDtor(NamesRegistry, smNames);
      CflatFree(smNames);
      smNames = nullptr;
   }
}

const char* Identifier::findFirstSeparator() const
{
   if(mNameLength > 0u)
   {
      for(uint32_t i = 1u; i < (mNameLength - 1u); i++)
      {
         if(mName[i] == ':' && mName[i + 1u] == ':')
         {
            return (mName + i);
         }
      }
   }

   return nullptr;
}
const char* Identifier::findLastSeparator() const
{
   if(mNameLength > 0u)
   {
      for(uint32_t i = (mNameLength - 1u); i > 1u; i--)
      {
         if(mName[i] == ':' && mName[i - 1u] == ':')
         {
            return (mName + i - 1);
         }
      }
   }

   return nullptr;
}

bool Identifier::operator==(const Identifier& pOther) const
{
   return mHash == pOther.mHash;
}
bool Identifier::operator!=(const Identifier& pOther) const
{
   return mHash != pOther.mHash;
}


//
//  Type
//
Type::~Type()
{
}

Type::Type(Namespace* pNamespace, const Identifier& pIdentifier)
   : mNamespace(pNamespace)
   , mParent(nullptr)
   , mIdentifier(pIdentifier)
   , mSize(0u)
{
}

Hash Type::getHash() const
{
   return mIdentifier.mHash;
}

bool Type::isDecimal() const
{
   return mCategory == TypeCategory::BuiltIn &&
      (strncmp(mIdentifier.mName, "float", 5u) == 0 ||
         strcmp(mIdentifier.mName, "double") == 0);
}

bool Type::isInteger() const
{
   return (mCategory == TypeCategory::BuiltIn && !isDecimal()) || mCategory == TypeCategory::Enum;
}

bool Type::compatibleWith(const Type& pOther) const
{
   return this == &pOther || (isInteger() && pOther.isInteger());
}


//
//  TypeUsage
//
const CflatArgsVector(TypeUsage) TypeUsage::kEmptyList;

TypeUsage::TypeUsage()
   : mType(nullptr)
   , mArraySize(1u)
   , mPointerLevel(0u)
   , mFlags(0u)
{
}

size_t TypeUsage::getSize() const
{
   if(mPointerLevel > 0u)
   {
      return sizeof(void*);
   }

   return mType ? mType->mSize * mArraySize : 0u;
}

bool TypeUsage::isPointer() const
{
   return mPointerLevel > 0u;
}

bool TypeUsage::isConst() const
{
   return CflatHasFlag(mFlags, TypeUsageFlags::Const);
}

bool TypeUsage::isReference() const
{
   return CflatHasFlag(mFlags, TypeUsageFlags::Reference);
}

bool TypeUsage::isArray() const
{
   return CflatHasFlag(mFlags, TypeUsageFlags::Array);
}

bool TypeUsage::compatibleWith(const TypeUsage& pOther) const
{
   return
      mType->compatibleWith(*pOther.mType) &&
      mArraySize == pOther.mArraySize &&
      mPointerLevel == pOther.mPointerLevel;
}

bool TypeUsage::operator==(const TypeUsage& pOther) const
{
   return
      mType == pOther.mType &&
      mArraySize == pOther.mArraySize &&
      mPointerLevel == pOther.mPointerLevel &&
      isReference() == pOther.isReference();
}
bool TypeUsage::operator!=(const TypeUsage& pOther) const
{
   return !operator==(pOther);
}


//
//  TypeAlias
//
TypeAlias::TypeAlias()
  : mScopeLevel(0u)
{
}

TypeAlias::TypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
  : mIdentifier(pIdentifier)
  , mTypeUsage(pTypeUsage)
  , mScopeLevel(0u)
{
}


//
//  Member
//
Member::Member(const Identifier& pIdentifier)
   : mIdentifier(pIdentifier)
   , mOffset(0u)
{
}


//
//  Value
//
const CflatArgsVector(Value) Value::kEmptyList;

Value::Value()
   : mValueBufferType(ValueBufferType::Uninitialized)
   , mValueInitializationHint(ValueInitializationHint::None)
   , mValueBuffer(nullptr)
   , mStack(nullptr)
{
}

Value::Value(const Value& pOther)
   : mValueBufferType(ValueBufferType::Uninitialized)
   , mValueInitializationHint(ValueInitializationHint::None)
   , mValueBuffer(nullptr)
   , mStack(nullptr)
{
   *this = pOther;
}

Value::~Value()
{
   if(mValueBufferType == ValueBufferType::Stack)
   {
      CflatAssert(mStack);
      mStack->pop(mTypeUsage.getSize());
      CflatAssert(mStack->mPointer == mValueBuffer);
   }
   else if(mValueBufferType == ValueBufferType::Heap)
   {
      CflatAssert(mValueBuffer);
      CflatFree(mValueBuffer);
   }
}

void Value::reset()
{
   CflatInvokeDtor(Value, this);
   CflatInvokeCtor(Value, this);
}

void Value::initOnStack(const TypeUsage& pTypeUsage, EnvironmentStack* pStack)
{
   CflatAssert(mValueBufferType == ValueBufferType::Uninitialized);
   CflatAssert(pStack);

   mTypeUsage = pTypeUsage;
   mValueBufferType = ValueBufferType::Stack;
   mValueBuffer = (char*)pStack->push(pTypeUsage.getSize());
   mStack = pStack;
}

void Value::initOnHeap(const TypeUsage& pTypeUsage)
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

void Value::initExternal(const TypeUsage& pTypeUsage)
{
   CflatAssert(mValueBufferType == ValueBufferType::Uninitialized);
   mTypeUsage = pTypeUsage;
   mValueBufferType = ValueBufferType::External;
}

void Value::set(const void* pDataSource)
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

void Value::assign(const void* pDataSource)
{
   CflatAssert(mValueBufferType != ValueBufferType::Uninitialized);
   CflatAssert(pDataSource);

   memcpy(mValueBuffer, pDataSource, mTypeUsage.getSize());
}

Value& Value::operator=(const Value& pOther)
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


//
//  UsingDirective
//
UsingDirective::UsingDirective(Namespace* pNamespace)
   : mNamespace(pNamespace)
   , mBlockLevel(0u)
{
}


//
//  Function
//
Function::Function(const Identifier& pIdentifier)
   : mNamespace(nullptr)
   , mIdentifier(pIdentifier)
   , mProgram(nullptr)
   , mLine(0u)
   , execute(nullptr)
{
}

Function::~Function()
{
   execute = nullptr;
}


//
//  Method
//
Method::Method(const Identifier& pIdentifier)
   : mIdentifier(pIdentifier)
   , execute(nullptr)
{
}

Method::~Method()
{
   execute = nullptr;
}


//
//  Instance
//
Instance::Instance()
   : mScopeLevel(0u)
{
}

Instance::Instance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
   : mTypeUsage(pTypeUsage)
   , mIdentifier(pIdentifier)
   , mScopeLevel(0u)
{
}


//
//  TypesHolder
//
TypesHolder::~TypesHolder()
{
   for(TypesRegistry::iterator it = mTypes.begin(); it != mTypes.end(); it++)
   {
      Type* type = it->second;
      CflatInvokeDtor(Type, type);
      CflatFree(type);
   }
}

Type* TypesHolder::getType(const Identifier& pIdentifier)
{
   TypesRegistry::const_iterator it = mTypes.find(pIdentifier.mHash);
   return it != mTypes.end() ? it->second : nullptr;
}

Type* TypesHolder::getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   Hash hash = pIdentifier.mHash;

   for(size_t i = 0u; i < pTemplateTypes.size(); i++)
   {
      hash += pTemplateTypes[i].mType->getHash();
      hash += (Hash)pTemplateTypes[i].mPointerLevel;
   }

   TypesRegistry::const_iterator it = mTypes.find(hash);
   return it != mTypes.end() ? it->second : nullptr;
}

void TypesHolder::registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   TypeAlias typeAlias(pIdentifier, pTypeUsage);
   mTypeAliases[pIdentifier.mHash] = typeAlias;
}

const TypeAlias* TypesHolder::getTypeAlias(const Identifier& pIdentifier)
{
   TypeAliasesRegistry::const_iterator it = mTypeAliases.find(pIdentifier.mHash);
   return it != mTypeAliases.end() ? &it->second : nullptr;
}


//
//  FunctionsHolder
//
FunctionsHolder::~FunctionsHolder()
{
   for(FunctionsRegistry::iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
   {
      CflatSTLVector(Function*)& functions = it->second;

      for(size_t i = 0u; i < functions.size(); i++)
      {
         Function* function = functions[i];
         CflatInvokeDtor(Function, function);
         CflatFree(function);
      }
   }
}

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier)
{
   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? it->second.at(0) : nullptr;
}

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   Function* function = nullptr;
   CflatSTLVector(Function*)* functions = getFunctions(pIdentifier);

   if(functions)
   {
      // first pass: look for a perfect argument match
      for(size_t i = 0u; i < functions->size(); i++)
      {
         Function* functionOverload = functions->at(i);

         if(functionOverload->mParameters.size() == pParameterTypes.size() &&
            functionOverload->mTemplateTypes == pTemplateTypes)
         {
            bool parametersMatch = true;

            for(size_t j = 0u; j < pParameterTypes.size(); j++)
            {
               const TypeHelper::Compatibility compatibility =
                  TypeHelper::getCompatibility(functionOverload->mParameters[j], pParameterTypes[j]);

               if(compatibility != TypeHelper::Compatibility::PerfectMatch)
               {
                  parametersMatch = false;
                  break;
               }
            }

            if(parametersMatch)
            {
               function = functionOverload;
               break;
            }
         }
      }

      // second pass: look for a compatible argument match
      if(!function)
      {
         for(size_t i = 0u; i < functions->size(); i++)
         {
            Function* functionOverload = functions->at(i);

            if(functionOverload->mParameters.size() == pParameterTypes.size() &&
               functionOverload->mTemplateTypes == pTemplateTypes)
            {
               bool parametersMatch = true;

               for(size_t j = 0u; j < pParameterTypes.size(); j++)
               {
                  const TypeHelper::Compatibility compatibility =
                     TypeHelper::getCompatibility(functionOverload->mParameters[j], pParameterTypes[j]);

                  if(compatibility == TypeHelper::Compatibility::Incompatible)
                  {
                     parametersMatch = false;
                     break;
                  }
               }

               if(parametersMatch)
               {
                  function = functionOverload;
                  break;
               }
            }
         }
      }
   }

   return function;
}

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   CflatArgsVector(TypeUsage) typeUsages;

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return getFunction(pIdentifier, typeUsages, pTemplateTypes);
}

CflatSTLVector(Function*)* FunctionsHolder::getFunctions(const Identifier& pIdentifier)
{
   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? &it->second : nullptr;
}

void FunctionsHolder::getAllFunctions(CflatSTLVector(Function*)* pOutFunctions)
{
   for(FunctionsRegistry::const_iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
   {
      const CflatSTLVector(Function*)& functions = it->second;

      for(size_t i = 0u; i < functions.size(); i++)
      {
         pOutFunctions->push_back(functions[i]);
      }
   }
}

Function* FunctionsHolder::registerFunction(const Identifier& pIdentifier)
{
   Function* function = (Function*)CflatMalloc(sizeof(Function));
   CflatInvokeCtor(Function, function)(pIdentifier);
   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);

   if(it == mFunctions.end())
   {
      CflatSTLVector(Function*) functions;
      functions.push_back(function);
      mFunctions[pIdentifier.mHash] = functions;
   }
   else
   {
      it->second.push_back(function);
   }

   return function;
}


//
//  InstancesHolder
//
InstancesHolder::InstancesHolder(size_t pMaxInstances)
   : mMaxInstances(pMaxInstances)
{
}

InstancesHolder::~InstancesHolder()
{
   releaseInstances(0u, true);
}

void InstancesHolder::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   Instance* instance = retrieveInstance(pIdentifier);

   if(!instance)
   {
      instance = registerInstance(pTypeUsage, pIdentifier);
   }

   instance->mValue = pValue;
}

Value* InstancesHolder::getVariable(const Identifier& pIdentifier)
{
   Instance* instance = retrieveInstance(pIdentifier);
   return instance ? &instance->mValue : nullptr;
}

Instance* InstancesHolder::registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   CflatAssert(mInstances.size() < mMaxInstances);

   if(mInstances.capacity() == 0u)
   {
      mInstances.reserve(mMaxInstances);
   }

   mInstances.emplace_back(pTypeUsage, pIdentifier);
   return &mInstances.back();
}

Instance* InstancesHolder::retrieveInstance(const Identifier& pIdentifier)
{
   Instance* instance = nullptr;

   for(int i = (int)mInstances.size() - 1; i >= 0; i--)
   {
      if(mInstances[i].mIdentifier == pIdentifier)
      {
         instance = &mInstances[i];
         break;
      }
   }

   return instance;
}

void InstancesHolder::releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors)
{
   while(!mInstances.empty() && mInstances.back().mScopeLevel >= pScopeLevel)
   {
      if(pExecuteDestructors)
      {
         Instance& instance = mInstances.back();
         Type* instanceType = instance.mTypeUsage.mType;

         if(instanceType->mCategory == TypeCategory::StructOrClass &&
            !instance.mTypeUsage.isPointer() &&
            !instance.mTypeUsage.isReference())
         {
            const Identifier dtorId("~");
            Struct* structOrClassType = static_cast<Struct*>(instanceType);

            for(size_t i = 0u; i < structOrClassType->mMethods.size(); i++)
            {
               if(structOrClassType->mMethods[i].mIdentifier == dtorId)
               {
                  Method& dtor = structOrClassType->mMethods[i];

                  TypeUsage thisPtrTypeUsage;
                  thisPtrTypeUsage.mType = instanceType;
                  thisPtrTypeUsage.mPointerLevel = 1u;

                  Value thisPtrValue;
                  thisPtrValue.initExternal(thisPtrTypeUsage);
                  thisPtrValue.set(&instance.mValue.mValueBuffer);

                  CflatArgsVector(Value) args;
                  dtor.execute(thisPtrValue, args, nullptr);

                  break;
               }
            }
         }
      }

      mInstances.pop_back();
   }
}

void InstancesHolder::getAllInstances(CflatSTLVector(Instance*)* pOutInstances)
{
   for(size_t i = 0u; i < mInstances.size(); i++)
   {
      pOutInstances->push_back(&mInstances[i]);
   }
}


//
//  BuiltInType
//
BuiltInType::BuiltInType(Namespace* pNamespace, const Identifier& pIdentifier)
   : Type(pNamespace, pIdentifier)
{
   mCategory = TypeCategory::BuiltIn;
}


//
//  Enum
//
Enum::Enum(Namespace* pNamespace, const Identifier& pIdentifier)
   : Type(pNamespace, pIdentifier)
{
   mCategory = TypeCategory::Enum;
}


//
//  EnumClass
//
EnumClass::EnumClass(Namespace* pNamespace, const Identifier& pIdentifier)
   : Type(pNamespace, pIdentifier)
{
   mCategory = TypeCategory::EnumClass;
}


//
//  Struct
//
Struct::Struct(Namespace* pNamespace, const Identifier& pIdentifier)
   : Type(pNamespace, pIdentifier)
   , mInstancesHolder(kMaxStaticMembers)
{
   mCategory = TypeCategory::StructOrClass;
}

Hash Struct::getHash() const
{
   Hash hash = mIdentifier.mHash;

   for(size_t i = 0u; i < mTemplateTypes.size(); i++)
   {
      hash += mTemplateTypes[i].mType->getHash();
      hash += (Hash)mTemplateTypes[i].mPointerLevel;
   }

   return hash;
}

bool Struct::derivedFrom(Type* pBaseType) const
{
   for(size_t i = 0u; i < mBaseTypes.size(); i++)
   {
      if(mBaseTypes[i].mType == pBaseType)
         return true;
   }

   return false;
}

uint16_t Struct::getOffset(Type* pBaseType) const
{
   for(size_t i = 0u; i < mBaseTypes.size(); i++)
   {
      if(mBaseTypes[i].mType == pBaseType)
         return mBaseTypes[i].mOffset;
   }

   return 0u;
}

Type* Struct::getType(const Identifier& pIdentifier)
{
   return mTypesHolder.getType(pIdentifier);
}

Type* Struct::getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   return mTypesHolder.getType(pIdentifier, pTemplateTypes);
}

void Struct::registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   mTypesHolder.registerTypeAlias(pIdentifier, pTypeUsage);
}

const TypeAlias* Struct::getTypeAlias(const Identifier& pIdentifier)
{
   return mTypesHolder.getTypeAlias(pIdentifier);
}

Function* Struct::registerStaticMethod(const Identifier& pIdentifier)
{
   return mFunctionsHolder.registerFunction(pIdentifier);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier)
{
   return mFunctionsHolder.getFunction(pIdentifier);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes,
   const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   return mFunctionsHolder.getFunction(pIdentifier, pParameterTypes, pTemplateTypes);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments,
   const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   return mFunctionsHolder.getFunction(pIdentifier, pArguments, pTemplateTypes);
}

CflatSTLVector(Function*)* Struct::getStaticMethods(const Identifier& pIdentifier)
{
   return mFunctionsHolder.getFunctions(pIdentifier);
}

void Struct::setStaticMember(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   mInstancesHolder.setVariable(pTypeUsage, pIdentifier, pValue);
}

Value* Struct::getStaticMember(const Identifier& pIdentifier)
{
   return mInstancesHolder.getVariable(pIdentifier);
}

Instance* Struct::getStaticMemberInstance(const Identifier& pIdentifier)
{
   return mInstancesHolder.retrieveInstance(pIdentifier);
}


//
//  Class
//
Class::Class(Namespace* pNamespace, const Identifier& pIdentifier)
   : Struct(pNamespace, pIdentifier)
{
}


//
//  TypeHelper
//
TypeHelper::Compatibility TypeHelper::getCompatibility(
   const TypeUsage& pParameter, const TypeUsage& pArgument)
{
   if(pParameter == pArgument)
   {
      return Compatibility::PerfectMatch;
   }

   if(pParameter.mType == pArgument.mType &&
      pParameter.mPointerLevel == pArgument.mPointerLevel &&
      pParameter.getSize() == pArgument.getSize())
   {
      return Compatibility::PerfectMatch;
   }

   if(pArgument.compatibleWith(pParameter))
   {
      return Compatibility::ImplicitCastableInteger;
   }

   if(pArgument.mType->mCategory == TypeCategory::BuiltIn &&
      !pArgument.isPointer() &&
      !pArgument.isReference() &&
      pParameter.mType->mCategory == TypeCategory::BuiltIn &&
      !pParameter.isPointer() &&
      !pParameter.isReference())
   {
      if(pArgument.mType->isDecimal() && pParameter.mType->isDecimal())
      {
         return Compatibility::ImplicitCastableFloat;
      }

      return Compatibility::ImplicitCastableIntegerFloat;
   }

   if(pArgument.mType->mCategory == TypeCategory::StructOrClass &&
      pArgument.isPointer() &&
      pParameter.mType->mCategory == TypeCategory::StructOrClass &&
      pParameter.isPointer())
   {
      Struct* argumentType = static_cast<Struct*>(pArgument.mType);
      Struct* parameterType = static_cast<Struct*>(pParameter.mType);

      if(argumentType->derivedFrom(parameterType))
      {
         return Compatibility::ImplicitCastableInheritance;
      }
   }

   return Compatibility::Incompatible;
}


//
//  Tokenizer
//
const char* kCflatPunctuation[] =
{
   ".", ",", ":", ";", "->", "(", ")", "{", "}", "[", "]", "::"
};
const size_t kCflatPunctuationCount = sizeof(kCflatPunctuation) / sizeof(const char*);

const char* kCflatOperators[] =
{
   "+", "-", "*", "/", "%",
   "++", "--", "!",
   "=", "+=", "-=", "*=", "/=", "&=", "|=",
   "<<", ">>",
   "==", "!=", ">", "<", ">=", "<=",
   "&&", "||", "&", "|", "~", "^"
};
const size_t kCflatOperatorsCount = sizeof(kCflatOperators) / sizeof(const char*);

const char* kCflatAssignmentOperators[] =
{
   "=", "+=", "-=", "*=", "/=", "&=", "|="
};
const size_t kCflatAssignmentOperatorsCount = sizeof(kCflatAssignmentOperators) / sizeof(const char*);

const char* kCflatLogicalOperators[] =
{
   "==", "!=", ">", "<", ">=", "<=", "&&", "||"
};
const size_t kCflatLogicalOperatorsCount = sizeof(kCflatLogicalOperators) / sizeof(const char*);

const char* kCflatConditionalOperator = "?";

const char* kCflatBinaryOperators[] =
{
   "*", "/", "%",
   "+", "-",
   "<<", ">>",
   "<", "<=", ">", ">=",
   "==", "!=",
   "&",
   "^",
   "|",
   "&&",
   "||"
};
const uint8_t kCflatBinaryOperatorsPrecedence[] =
{
   1u, 1u, 1u,
   2u, 2u,
   3u, 3u,
   4u, 4u, 4u, 4u,
   5u, 5u,
   6u,
   7u,
   8u,
   9u,
   10u
};
const size_t kCflatBinaryOperatorsCount = sizeof(kCflatBinaryOperators) / sizeof(const char*);
static_assert
(
   kCflatBinaryOperatorsCount == (sizeof(kCflatBinaryOperatorsPrecedence) / sizeof(uint8_t)),
   "Precedence must be defined for all binary operators"
);

const char* kCflatKeywords[] =
{
   "break", "case", "class", "const", "const_cast", "continue", "default",
   "delete", "do", "dynamic_cast", "else", "enum", "false", "for", "if",
   "namespace", "new", "nullptr", "operator", "private", "protected", "public",
   "reinterpret_cast", "return", "sizeof", "static", "static_cast",
   "struct", "switch", "this", "true", "typedef", "union", "unsigned",
   "using", "virtual", "void", "while"
};
const size_t kCflatKeywordsCount = sizeof(kCflatKeywords) / sizeof(const char*);

void Tokenizer::tokenize(const char* pCode, CflatSTLVector(Token)& pTokens)
{
   char* cursor = const_cast<char*>(pCode);
   uint16_t currentLine = 1u;

   pTokens.clear();

   while(*cursor != '\0')
   {
      while(*cursor == ' ' || *cursor == '\t' || *cursor == '\n')
      {
         if(*cursor == '\n')
         {
            currentLine++;
         }

         cursor++;
      }

      if(*cursor == '\0')
      {
         break;
      }

      Token token;
      token.mStart = cursor;
      token.mLength = 1u;
      token.mLine = currentLine;

      // string
      if(*cursor == '"')
      {
         do
         {
            cursor++;
         }
         while(!(*cursor == '"' && *(cursor - 1) != '\\'));

         cursor++;
         token.mLength = cursor - token.mStart;
         token.mType = TokenType::String;
         pTokens.push_back(token);
         continue;
      }

      // numeric value
      if(isdigit(*cursor) || (*cursor == '.' && isdigit(*(cursor + 1))))
      {
         if(*cursor == '0' && *(cursor + 1) == 'x')
         {
            cursor++;

            do
            {
               cursor++;
            }
            while(isxdigit(*cursor));
         }
         else
         {
            do
            {
               cursor++;
            }
            while(isdigit(*cursor) || *cursor == '.' || *cursor == 'f' || *cursor == 'u');
         }

         token.mLength = cursor - token.mStart;
         token.mType = TokenType::Number;
         pTokens.push_back(token);
         continue;
      }

      // punctuation (2 characters)
      const size_t tokensCount = pTokens.size();

      for(size_t i = 0u; i < kCflatPunctuationCount; i++)
      {
         if(strncmp(token.mStart, kCflatPunctuation[i], 2u) == 0)
         {
            cursor += 2u;
            token.mLength = cursor - token.mStart;
            token.mType = TokenType::Punctuation;
            pTokens.push_back(token);
            break;
         }
      }

      if(pTokens.size() > tokensCount)
      {
         continue;
      }

      // operator (2 characters)
      for(size_t i = 0u; i < kCflatOperatorsCount; i++)
      {
         if(strncmp(token.mStart, kCflatOperators[i], 2u) == 0)
         {
            cursor += 2u;
            token.mLength = cursor - token.mStart;
            token.mType = TokenType::Operator;
            pTokens.push_back(token);
            break;
         }
      }

      if(pTokens.size() > tokensCount)
      {
         continue;
      }

      // punctuation (1 character)
      for(size_t i = 0u; i < kCflatPunctuationCount; i++)
      {
         if(token.mStart[0] == kCflatPunctuation[i][0] && kCflatPunctuation[i][1] == '\0')
         {
            cursor++;
            token.mType = TokenType::Punctuation;
            pTokens.push_back(token);
            break;
         }
      }

      if(pTokens.size() > tokensCount)
      {
         continue;
      }

      // operator (1 character)
      if(token.mStart[0] == kCflatConditionalOperator[0])
      {
         cursor++;
         token.mType = TokenType::Operator;
         pTokens.push_back(token);
      }
      else
      {
         for(size_t i = 0u; i < kCflatOperatorsCount; i++)
         {
            if(token.mStart[0] == kCflatOperators[i][0])
            {
               cursor++;
               token.mType = TokenType::Operator;
               pTokens.push_back(token);
               break;
            }
         }
      }

      if(pTokens.size() > tokensCount)
      {
         continue;
      }

      // keywords
      for(size_t i = 0u; i < kCflatKeywordsCount; i++)
      {
         const size_t keywordLength = strlen(kCflatKeywords[i]);

         if(strncmp(token.mStart, kCflatKeywords[i], keywordLength) == 0 &&
            !isalnum(token.mStart[keywordLength]) &&
            token.mStart[keywordLength] != '_')
         {
            cursor += keywordLength;
            token.mLength = cursor - token.mStart;
            token.mType = TokenType::Keyword;
            pTokens.push_back(token);
            break;
         }
      }

      if(pTokens.size() > tokensCount)
      {
         continue;
      }

      // identifier
      do
      {
         cursor++;
      }
      while(isalnum(*cursor) || *cursor == '_');

      token.mLength = cursor - token.mStart;
      token.mType = TokenType::Identifier;
      pTokens.push_back(token);
   }
}


//
//  Program
//
Program::~Program()
{
   for(size_t i = 0u; i < mStatements.size(); i++)
   {
      CflatInvokeDtor(Statement, mStatements[i]);
      CflatFree(mStatements[i]);
   }
}


//
//  Namespace
//
Namespace::Namespace(const Identifier& pIdentifier, Namespace* pParent, Environment* pEnvironment)
   : mIdentifier(pIdentifier)
   , mFullIdentifier(pIdentifier)
   , mParent(pParent)
   , mEnvironment(pEnvironment)
   , mInstancesHolder(kMaxGlobalInstancesPerNamespace)
{
   if(pParent && pParent->getParent())
   {
      char buffer[kDefaultLocalStringBufferSize];
      sprintf(buffer, "%s::%s", mParent->mFullIdentifier.mName, mIdentifier.mName);
      mFullIdentifier = Identifier(buffer);
   }
}

Namespace::~Namespace()
{
   for(NamespacesRegistry::iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      Namespace* ns = it->second;
      CflatInvokeDtor(Namespace, ns);
      CflatFree(ns);
   }

   mInstancesHolder.releaseInstances(0u, true);
}

const Identifier& Namespace::getIdentifier() const
{
   return mIdentifier;
}

const Identifier& Namespace::getFullIdentifier() const
{
   return mFullIdentifier;
}

Namespace* Namespace::getParent()
{
   return mParent;
}

Namespace* Namespace::getChild(Hash pNameHash)
{
   NamespacesRegistry::const_iterator it = mNamespaces.find(pNameHash);
   return it != mNamespaces.end() ? it->second : nullptr;
}

Namespace* Namespace::getNamespace(const Identifier& pName)
{
   const char* separator = pName.findFirstSeparator();

   if(separator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t childIdentifierLength = separator - pName.mName;
      strncpy(buffer, pName.mName, childIdentifierLength);
      buffer[childIdentifierLength] = '\0';

      const Hash childNameHash = hash(buffer);
      Namespace* child = getChild(childNameHash);

      if(child)
      {
         const Identifier subIdentifier(separator + 2);
         return child->getNamespace(subIdentifier);
      }

      return nullptr;
   }

   return getChild(pName.mHash);
}

Namespace* Namespace::requestNamespace(const Identifier& pName)
{
   const char* separator = pName.findFirstSeparator();

   if(separator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t childIdentifierLength = separator - pName.mName;
      strncpy(buffer, pName.mName, childIdentifierLength);
      buffer[childIdentifierLength] = '\0';

      const Identifier childIdentifier(buffer);
      Namespace* child = getChild(childIdentifier.mHash);

      if(!child)
      {
         child = (Namespace*)CflatMalloc(sizeof(Namespace));
         CflatInvokeCtor(Namespace, child)(childIdentifier, this, mEnvironment);
         mNamespaces[childIdentifier.mHash] = child;
      }

      const Identifier subIdentifier(separator + 2);
      return child->requestNamespace(subIdentifier);
   }

   Namespace* child = getChild(pName.mHash);

   if(!child)
   {
      child = (Namespace*)CflatMalloc(sizeof(Namespace));
      CflatInvokeCtor(Namespace, child)(pName, this, mEnvironment);
      mNamespaces[pName.mHash] = child;
   }

   return child;
}

Type* Namespace::getType(const Identifier& pIdentifier, bool pExtendSearchToParent)
{
   return getType(pIdentifier, TypeUsage::kEmptyList, pExtendSearchToParent);
}

Type* Namespace::getType(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pTemplateTypes, bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier typeIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getType(typeIdentifier, pTemplateTypes);
      }

      Type* type = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         type = mParent->getType(pIdentifier, pTemplateTypes, true);
      }

      if(!type)
      {
         Type* parentType = getType(nsIdentifier);

         if(parentType && parentType->mCategory == TypeCategory::StructOrClass)
         {
            type = static_cast<Struct*>(parentType)->getType(typeIdentifier, pTemplateTypes);
         }
      }

      return type;
   }

   const TypeAlias* typeAlias = getTypeAlias(pIdentifier);

   if(typeAlias && typeAlias->mTypeUsage.mFlags == 0u)
   {
      return typeAlias->mTypeUsage.mType;
   }

   Type* type = mTypesHolder.getType(pIdentifier, pTemplateTypes);

   if(type)
   {
      return type;
   }

   if(pExtendSearchToParent && mParent)
   {
      return mParent->getType(pIdentifier, pTemplateTypes, true);
   }

   return nullptr;
}

void Namespace::registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   mTypesHolder.registerTypeAlias(pIdentifier, pTypeUsage);
}

const TypeAlias* Namespace::getTypeAlias(const Identifier& pIdentifier)
{
   return mTypesHolder.getTypeAlias(pIdentifier);
}

TypeUsage Namespace::getTypeUsage(const char* pTypeName)
{
   return mEnvironment->getTypeUsage(pTypeName, this);
}

Function* Namespace::getFunction(const Identifier& pIdentifier, bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getFunction(functionIdentifier);
      }

      Function* function = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         function = mParent->getFunction(pIdentifier, true);
      }

      return function;
   }

   Function* function = mFunctionsHolder.getFunction(pIdentifier);

   if(!function && pExtendSearchToParent && mParent)
   {
      function = mParent->getFunction(pIdentifier, true);
   }

   return function;
}

Function* Namespace::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes,
   const CflatArgsVector(TypeUsage)& pTemplateTypes,
   bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getFunction(functionIdentifier, pParameterTypes, pTemplateTypes);
      }

      Function* function = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         function = mParent->getFunction(pIdentifier, pParameterTypes, pTemplateTypes, true);
      }

      return function;
   }

   Function* function = mFunctionsHolder.getFunction(pIdentifier, pParameterTypes, pTemplateTypes);

   if(!function && pExtendSearchToParent && mParent)
   {
      function = mParent->getFunction(pIdentifier, pParameterTypes, pTemplateTypes, true);
   }

   return function;
}

Function* Namespace::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments,
   const CflatArgsVector(TypeUsage)& pTemplateTypes,
   bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getFunction(functionIdentifier, pArguments, pTemplateTypes);
      }
      
      Function* function = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         function = mParent->getFunction(pIdentifier, pArguments, pTemplateTypes, true);
      }

      return function;
   }

   Function* function = mFunctionsHolder.getFunction(pIdentifier, pArguments, pTemplateTypes);

   if(!function && pExtendSearchToParent && mParent)
   {
      function = mParent->getFunction(pIdentifier, pArguments, pTemplateTypes, true);
   }

   return function;
}

CflatSTLVector(Function*)* Namespace::getFunctions(const Identifier& pIdentifier,
   bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getFunctions(functionIdentifier);
      }

      CflatSTLVector(Function*)* functions = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         functions = mParent->getFunctions(pIdentifier, true);
      }

      if(!functions)
      {
         Type* parentType = getType(nsIdentifier);

         if(parentType && parentType->mCategory == TypeCategory::StructOrClass)
         {
            functions = static_cast<Struct*>(parentType)->getStaticMethods(pIdentifier);
         }
      }

      return functions;
   }

   CflatSTLVector(Function*)* functions = mFunctionsHolder.getFunctions(pIdentifier);

   if(!functions && pExtendSearchToParent && mParent)
   {
      functions = mParent->getFunctions(pIdentifier, true);
   }

   return functions;
}

Function* Namespace::registerFunction(const Identifier& pIdentifier)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = requestNamespace(nsIdentifier);
      return ns->registerFunction(functionIdentifier);
   }

   Function* function = mFunctionsHolder.registerFunction(pIdentifier);
   function->mNamespace = this;

   return function;
}

void Namespace::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier variableIdentifier(lastSeparator + 2);

      Namespace* ns = requestNamespace(nsIdentifier);
      return ns->setVariable(pTypeUsage, variableIdentifier, pValue);
   }

   Instance* instance = retrieveInstance(pIdentifier);

   if(!instance)
   {
      instance = registerInstance(pTypeUsage, pIdentifier);
   }

   instance->mValue.initOnHeap(pTypeUsage);
   instance->mValue.set(pValue.mValueBuffer);
}

Value* Namespace::getVariable(const Identifier& pIdentifier, bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier variableIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->getVariable(variableIdentifier);
      }

      Value* value = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         value = mParent->getVariable(pIdentifier, true);
      }

      return value;
   }

   Value* value = mInstancesHolder.getVariable(pIdentifier);

   if(!value && pExtendSearchToParent && mParent)
   {
      value = mParent->getVariable(pIdentifier, true);
   }

   return value;
}

Instance* Namespace::registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier instanceIdentifier(lastSeparator + 2);

      Namespace* ns = requestNamespace(nsIdentifier);
      return ns->registerInstance(pTypeUsage, instanceIdentifier);
   }

   return mInstancesHolder.registerInstance(pTypeUsage, pIdentifier);
}

Instance* Namespace::retrieveInstance(const Identifier& pIdentifier, bool pExtendSearchToParent)
{
   const char* lastSeparator = pIdentifier.findLastSeparator();

   if(lastSeparator)
   {
      char buffer[kDefaultLocalStringBufferSize];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier instanceIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);

      if(ns)
      {
         return ns->retrieveInstance(instanceIdentifier);
      }

      Instance* instance = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         instance = mParent->retrieveInstance(pIdentifier, true);
      }

      return instance;
   }

   Instance* instance = mInstancesHolder.retrieveInstance(pIdentifier);

   if(!instance && pExtendSearchToParent && mParent)
   {
      instance = mParent->retrieveInstance(pIdentifier, true);
   }

   return instance;
}

void Namespace::releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors)
{
   mInstancesHolder.releaseInstances(pScopeLevel, pExecuteDestructors);

   for(NamespacesRegistry::iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      Namespace* ns = it->second;
      ns->releaseInstances(pScopeLevel, pExecuteDestructors);
   }
}

void Namespace::getAllNamespaces(CflatSTLVector(Namespace*)* pOutNamespaces)
{
   for(NamespacesRegistry::const_iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      pOutNamespaces->push_back(it->second);
   }
}

void Namespace::getAllInstances(CflatSTLVector(Instance*)* pOutInstances)
{
   mInstancesHolder.getAllInstances(pOutInstances);
}

void Namespace::getAllFunctions(CflatSTLVector(Function*)* pOutFunctions)
{
   mFunctionsHolder.getAllFunctions(pOutFunctions);
}


//
//  Context
//
Context::Context(ContextType pType, Namespace* pGlobalNamespace)
   : mType(pType)
   , mProgram(nullptr)
   , mBlockLevel(0u)
   , mScopeLevel(0u)
   , mLocalInstancesHolder(kMaxLocalInstances)
{
   mNamespaceStack.push_back(pGlobalNamespace);
}


//
//  ParsingContext
//
ParsingContext::ParsingContext(Namespace* pGlobalNamespace)
   : Context(ContextType::Parsing, pGlobalNamespace)
   , mTokenIndex(0u)
{
}


//
//  CallStackEntry
//
CallStackEntry::CallStackEntry(const Program* pProgram, const Function* pFunction)
   : mProgram(pProgram)
   , mFunction(pFunction)
   , mLine(0u)
{
}


//
//  ExecutionContext
//
ExecutionContext::ExecutionContext(Namespace* pGlobalNamespace)
   : Context(ContextType::Execution, pGlobalNamespace)
   , mJumpStatement(JumpStatement::None)
{
}


//
//  Environment
//
Environment::Environment()
   : mTypesParsingContext(&mGlobalNamespace)
   , mExecutionContext(&mGlobalNamespace)
   , mGlobalNamespace("", nullptr, this)
   , mExecutionHook(nullptr)
{
   static_assert(kPreprocessorErrorStringsCount == (size_t)Environment::PreprocessorError::Count,
      "Missing preprocessor error strings");
   static_assert(kCompileErrorStringsCount == (size_t)Environment::CompileError::Count,
      "Missing compile error strings");
   static_assert(kRuntimeErrorStringsCount == (size_t)Environment::RuntimeError::Count,
      "Missing runtime error strings");

   registerBuiltInTypes();

   mTypeAuto = registerType<BuiltInType>("auto");
   mTypeVoid = registerType<BuiltInType>("void");
   mTypeInt32 = getType("int");
   mTypeUInt32 = getType("uint32_t");
   mTypeFloat = getType("float");
   mTypeDouble = getType("double");

   mTypeUsageSizeT = getTypeUsage("size_t");
   mTypeUsageBool = getTypeUsage("bool");
   mTypeUsageCString = getTypeUsage("const char*");
   mTypeUsageVoidPtr = getTypeUsage("void*");
}

Environment::~Environment()
{
   for(ProgramsRegistry::iterator it = mPrograms.begin(); it != mPrograms.end(); it++)
   {
      CflatInvokeDtor(Program, it->second);
      CflatFree(it->second);
   }
}

void Environment::defineMacro(const char* pDefinition, const char* pBody)
{
   Macro macro;

   // process definition
   const size_t definitionLength = strlen(pDefinition);

   CflatSTLVector(CflatSTLString) parameters;
   int8_t currentParameterIndex = -1;

   for(size_t i = 0u; i < definitionLength; i++)
   {
      char currentChar = pDefinition[i];

      if(currentChar == '(' || currentChar == ',')
      {
         currentParameterIndex++;
         parameters.emplace_back();
         continue;
      }
      else if(currentChar == ')')
      {
         break;
      }

      if(currentParameterIndex < 0)
      {
         macro.mName.push_back(currentChar);
      }
      else
      {
         parameters[currentParameterIndex].push_back(currentChar);
      }
   }

   macro.mParametersCount = (uint8_t)(currentParameterIndex + 1);

   // process body
   const size_t bodyLength = strlen(pBody);

   if(bodyLength > 0u)
   {
      int bodyChunkIndex = -1;

      for(size_t i = 0u; i < bodyLength; i++)
      {
         char currentChar = pBody[i];
         bool anyParameterProcessed = false;

         for(uint8_t j = 0u; j < macro.mParametersCount; j++)
         {
            if(strncmp(pBody + i, parameters[j].c_str(), parameters[j].length()) == 0)
            {
               MacroArgumentType argumentType = MacroArgumentType::Default;

               if(i >= 2u && pBody[i - 1u] == '#' && pBody[i - 2u] == '#')
               {
                  argumentType = MacroArgumentType::TokenPaste;
               }
               else if(i >= 1u && pBody[i - 1u] == '#')
               {
                  argumentType = MacroArgumentType::Stringize;
               }

               macro.mBody.emplace_back();
               bodyChunkIndex++;

               // argument char ('$')
               macro.mBody[bodyChunkIndex].push_back('$');
               // parameter index (1, 2, 3, etc.)
               macro.mBody[bodyChunkIndex].push_back((char)('1' + (char)j));
               // argument type (0: Default, 1: Stringize, 2: TokenPaste)
               macro.mBody[bodyChunkIndex].push_back((char)('0' + (char)argumentType));

               i += parameters[j].length() - 1u;

               anyParameterProcessed = true;
               break;
            }
         }

         if(!anyParameterProcessed && currentChar != '#')
         {
            if(macro.mBody.empty() || macro.mBody[macro.mBody.size() - 1u][0] == '$')
            {
               macro.mBody.emplace_back();
               bodyChunkIndex++;
            }

            macro.mBody[bodyChunkIndex].push_back(currentChar);
         }
      }
   }

   // register macro in the environment
   bool newEntryRequired = true;

   for(size_t i = 0u; i < mMacros.size(); i++)
   {
      if(mMacros[i].mName == macro.mName)
      {
         mMacros[i].mBody = macro.mBody;
         mMacros[i].mParametersCount = macro.mParametersCount;
         newEntryRequired = false;
         break;
      }
   }

   if(newEntryRequired)
   {
      mMacros.push_back(macro);
   }
}

void Environment::registerBuiltInTypes()
{
   CflatRegisterBuiltInType(this, int);
   CflatRegisterBuiltInType(this, int32_t);
   CflatRegisterBuiltInType(this, uint32_t);
   CflatRegisterBuiltInType(this, size_t);
   CflatRegisterBuiltInType(this, char);
   CflatRegisterBuiltInType(this, bool);
   CflatRegisterBuiltInType(this, int8_t);
   CflatRegisterBuiltInType(this, uint8_t);
   CflatRegisterBuiltInType(this, short);
   CflatRegisterBuiltInType(this, int16_t);
   CflatRegisterBuiltInType(this, uint16_t);
   CflatRegisterBuiltInType(this, int64_t);
   CflatRegisterBuiltInType(this, uint64_t);
   CflatRegisterBuiltInType(this, float);
   CflatRegisterBuiltInType(this, double);
}

TypeUsage Environment::parseTypeUsage(ParsingContext& pContext, size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   size_t cachedTokenIndex = tokenIndex;

   TypeUsage typeUsage;
   char baseTypeName[kDefaultLocalStringBufferSize];

   if(tokens[tokenIndex].mType == TokenType::Keyword &&
      strncmp(tokens[tokenIndex].mStart, "const", 5u) == 0)
   {
      CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Const);
      tokenIndex++;
   }

   pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);

   while(tokenIndex < (tokens.size() - 1u) &&
      tokens[tokenIndex + 1u].mLength == 2u &&
      strncmp(tokens[tokenIndex + 1u].mStart, "::", 2u) == 0)
   {
      tokenIndex += 2u;
      pContext.mStringBuffer.append("::");
      pContext.mStringBuffer.append(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
   }

   strcpy(baseTypeName, pContext.mStringBuffer.c_str());
   const Identifier baseTypeIdentifier(baseTypeName);
   CflatArgsVector(TypeUsage) templateTypes;

   if(tokenIndex < (tokens.size() - 1u) && tokens[tokenIndex + 1u].mStart[0] == '<')
   {
      tokenIndex += 2u;
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, '<', '>');

      if((pTokenLastIndex == 0u || closureTokenIndex <= pTokenLastIndex) &&
         isTemplate(pContext, tokenIndex - 1u, closureTokenIndex))
      {
         while(tokenIndex < closureTokenIndex)
         {
            templateTypes.push_back(parseTypeUsage(pContext, closureTokenIndex - 1u));
            tokenIndex++;
         }

         tokenIndex = closureTokenIndex;
      }
   }
   
   for(size_t i = 0u; i < pContext.mTypeAliases.size(); i++)
   {
      if(pContext.mTypeAliases[i].mIdentifier == baseTypeIdentifier)
      {
         typeUsage = pContext.mTypeAliases[i].mTypeUsage;
         break;
      }
   }

   if(!typeUsage.mType)
   {
      for(int i = (int)pContext.mNamespaceStack.size() - 1; i >= 0; i--)
      {
         Namespace* ns = pContext.mNamespaceStack[i];
         const TypeAlias* typeAlias = ns->getTypeAlias(baseTypeIdentifier);

         if(typeAlias)
         {
            typeUsage = typeAlias->mTypeUsage;
            break;
         }
      }

      if(!typeUsage.mType)
      {
         typeUsage.mType = findType(pContext, baseTypeIdentifier, templateTypes);
      }
   }

   if(typeUsage.mType)
   {
      if(tokenIndex < (tokens.size() - 1u) && *tokens[tokenIndex + 1u].mStart == '&')
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
         tokenIndex++;
      }

      while(tokenIndex < (tokens.size() - 1u) && *tokens[tokenIndex + 1u].mStart == '*')
      {
         typeUsage.mPointerLevel++;
         tokenIndex++;
      }

      tokenIndex++;
   }
   else
   {
      tokenIndex = cachedTokenIndex;
   }

   return typeUsage;
}

void Environment::throwPreprocessorError(ParsingContext& pContext, PreprocessorError pError,
   size_t pCursor, const char* pArg)
{
   char errorMsg[kDefaultLocalStringBufferSize];
   sprintf(errorMsg, kPreprocessorErrorStrings[(int)pError], pArg);

   const char* code = pContext.mProgram->mCode.c_str();
   uint16_t line = 1u;

   for(size_t i = 0u; i < pCursor; i++)
   {
      if(code[i] == '\n')
      {
         line++;
      }
   }

   char lineAsString[16];
   sprintf(lineAsString, "%d", line);

   mErrorMessage.assign("[Preprocessor Error] '");
   mErrorMessage.append(pContext.mProgram->mIdentifier.mName);
   mErrorMessage.append("' -- Line ");
   mErrorMessage.append(lineAsString);
   mErrorMessage.append(": ");
   mErrorMessage.append(errorMsg);
}

void Environment::throwCompileError(ParsingContext& pContext, CompileError pError,
   const char* pArg1, const char* pArg2)
{
   if(!mErrorMessage.empty())
      return;

   const Token& token = pContext.mTokenIndex < pContext.mTokens.size()
      ? pContext.mTokens[pContext.mTokenIndex]
      : pContext.mTokens[pContext.mTokens.size() - 1u];

   char errorMsg[kDefaultLocalStringBufferSize];
   sprintf(errorMsg, kCompileErrorStrings[(int)pError], pArg1, pArg2);

   char lineAsString[16];
   sprintf(lineAsString, "%d", token.mLine);

   mErrorMessage.assign("[Compile Error] '");
   mErrorMessage.append(pContext.mProgram->mIdentifier.mName);
   mErrorMessage.append("' -- Line ");
   mErrorMessage.append(lineAsString);
   mErrorMessage.append(": ");
   mErrorMessage.append(errorMsg);
}

void Environment::throwCompileErrorUnexpectedSymbol(ParsingContext& pContext)
{
   const Token& token = pContext.mTokens[pContext.mTokenIndex];
   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
}

void Environment::preprocess(ParsingContext& pContext, const char* pCode)
{
   CflatSTLString& preprocessedCode = pContext.mPreprocessedCode;

   const size_t codeLength = strlen(pCode);
   preprocessedCode.clear();

   size_t cursor = 0u;

   while(cursor < codeLength)
   {
      // line comment
      if(strncmp(pCode + cursor, "//", 2u) == 0)
      {
         while(pCode[cursor] != '\n' && pCode[cursor] != '\0')
         {
            cursor++;
         }
      }
      // block comment
      else if(strncmp(pCode + cursor, "/*", 2u) == 0)
      {
         while(strncmp(pCode + cursor, "*/", 2u) != 0)
         {
            cursor++;

            if(pCode[cursor] == '\n')
            {
               preprocessedCode.push_back('\n');
            }
         }

         cursor += 2u;
         continue;
      }
      // preprocessor directive
      else if(pCode[cursor] == '#')
      {
         cursor++;

         while(pCode[cursor] == ' ' || pCode[cursor] == '\t')
         {
            cursor++;
         }

         // #include (valid, but ignored)
         if(strncmp(pCode + cursor, "include", 7u) == 0)
         {
            cursor += 7u;
         }
         // #ifdef (valid, but ignored)
         else if(strncmp(pCode + cursor, "ifdef", 5u) == 0)
         {
            cursor += 5u;
         }
         // #if (valid, but ignored)
         else if(strncmp(pCode + cursor, "if", 2u) == 0)
         {
            cursor += 2u;
         }
         // #pragma (valid, but ignored)
         else if(strncmp(pCode + cursor, "pragma", 6u) == 0)
         {
            cursor += 6u;
         }
         // #define
         else if(strncmp(pCode + cursor, "define", 6u) == 0)
         {
            cursor += 6u;

            if(pCode[cursor] != ' ' && pCode[cursor] != '\t')
            {
               throwPreprocessorError(pContext, PreprocessorError::InvalidPreprocessorDirective,
                  cursor);
               return;
            }

            while(pCode[cursor] == ' ' || pCode[cursor] == '\t')
            {
               cursor++;
            }

            if(pCode[cursor] == '\n' || pCode[cursor] == '\0')
            {
               throwPreprocessorError(pContext, PreprocessorError::InvalidPreprocessorDirective,
                  cursor);
               return;
            }

            char macroDefinition[kDefaultLocalStringBufferSize];
            size_t macroCursor = 0u;

            while(pCode[cursor] != ' ' &&
               pCode[cursor] != '\t' &&
               pCode[cursor] != '\n' &&
               pCode[cursor] != '\0')
            {
               macroDefinition[macroCursor++] = pCode[cursor++];
            }

            macroDefinition[macroCursor] = '\0';

            while(pCode[cursor] == ' ' || pCode[cursor] == '\t')
            {
               cursor++;
            }

            char macroBody[kDefaultLocalStringBufferSize];
            macroCursor = 0u;

            while(pCode[cursor] != '\n' && pCode[cursor] != '\0')
            {
               macroBody[macroCursor++] = pCode[cursor++];
            }

            macroBody[macroCursor] = '\0';

            defineMacro(macroDefinition, macroBody);
         }
         else
         {
            throwPreprocessorError(pContext, PreprocessorError::InvalidPreprocessorDirective,
               cursor);
            return;
         }

         while(pCode[cursor] != '\n' && pCode[cursor] != '\0')
         {
            cursor++;
         }
      }

      // skip carriage return characters
      while(pCode[cursor] == '\r')
      {
         cursor++;
      }

      // perform macro replacement
      for(size_t i = 0u; i < mMacros.size(); i++)
      {
         Macro& macro = mMacros[i];

         if(strncmp(pCode + cursor, macro.mName.c_str(), macro.mName.length()) == 0)
         {
            cursor += macro.mName.length();

            // parse arguments
            CflatSTLVector(CflatSTLString) arguments;

            if(pCode[cursor] == '(')
            {
               arguments.emplace_back();
               cursor++;

               while(pCode[cursor] != ')')
               {
                  if(pCode[cursor] == ',')
                  {
                     arguments.emplace_back();
                     cursor++;

                     while(pCode[cursor++] == ' ');
                  }
                  else
                  {
                     arguments[arguments.size() - 1u].push_back(pCode[cursor]);
                     cursor++;
                  }
               }
            }

            // append the replaced strings
            for(size_t j = 0u; j < macro.mBody.size(); j++)
            {
               const CflatSTLString& bodyChunk = macro.mBody[j];

               if(bodyChunk[0] == '$')
               {
                  const size_t parameterIndex = (size_t)(bodyChunk[1] - '1');

                  if(parameterIndex >= arguments.size())
                  {
                     throwPreprocessorError(pContext, PreprocessorError::InvalidMacroArgumentCount,
                        cursor, macro.mName.c_str());
                     return;
                  }

                  const MacroArgumentType argumentType = (MacroArgumentType)(bodyChunk[2] - '0');

                  if(argumentType == MacroArgumentType::Stringize)
                  {
                     preprocessedCode.push_back('\"');
                     preprocessedCode.append(arguments[parameterIndex]);
                     preprocessedCode.push_back('\"');
                  }
                  else
                  {
                     preprocessedCode.append(arguments[parameterIndex]);
                  }
               }
               else
               {
                  preprocessedCode.append(bodyChunk);
               }
            }

            if(!arguments.empty())
            {
               cursor++;
            }

            break;
         }
      }

      // add the current character to the preprocessed code
      preprocessedCode.push_back(pCode[cursor]);

      cursor++;
   }

   if(preprocessedCode.back() != '\n')
   {
      preprocessedCode.push_back('\n');
   }

   preprocessedCode.shrink_to_fit();
}

void Environment::tokenize(ParsingContext& pContext)
{
   Tokenizer::tokenize(pContext.mPreprocessedCode.c_str(), pContext.mTokens);
}

void Environment::parse(ParsingContext& pContext)
{
   size_t& tokenIndex = pContext.mTokenIndex;

   for(tokenIndex = 0u; tokenIndex < pContext.mTokens.size(); tokenIndex++)
   {
      Statement* statement = parseStatement(pContext);

      if(!mErrorMessage.empty())
      {
         break;
      }

      if(statement)
      {
         pContext.mProgram->mStatements.push_back(statement);
      }
   }
}

Expression* Environment::parseExpression(ParsingContext& pContext, size_t pTokenLastIndex,
   bool pNullAllowed)
{
   Expression* expression = nullptr;

   const size_t tokensCount = pTokenLastIndex - pContext.mTokenIndex + 1u;

   if(tokensCount == 1u)
   {
      expression = parseExpressionSingleToken(pContext);
   }
   else
   {
      expression = parseExpressionMultipleTokens(pContext, pTokenLastIndex);
   }

   if(!pNullAllowed && !expression)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
   }

   return expression;
}

Expression* Environment::parseExpressionSingleToken(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Expression* expression = nullptr;

   if(token.mType == TokenType::Number)
   {
      TypeUsage typeUsage;
      Value value;

      pContext.mStringBuffer.assign(token.mStart, token.mLength);
      const char* numberStr = pContext.mStringBuffer.c_str();
      const size_t numberStrLength = strlen(numberStr);

      // decimal value
      if(strchr(numberStr, '.'))
      {
         // float
         if(numberStr[numberStrLength - 1u] == 'f')
         {
            typeUsage.mType = mTypeFloat;
            const float number = (float)strtod(numberStr, nullptr);
            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
         // double
         else
         {
            typeUsage.mType = mTypeDouble;
            const double number = strtod(numberStr, nullptr);
            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
      }
      // integer value
      else
      {
         // unsigned
         if(numberStr[numberStrLength - 1u] == 'u')
         {
            typeUsage.mType = mTypeUInt32;
            const uint32_t number = (uint32_t)atoi(numberStr);
            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
         // hex
         else if(numberStr[0] == '0' && numberStr[1] == 'x')
         {
            typeUsage.mType = mTypeUInt32;
            const uint32_t number = (uint32_t)strtoul(numberStr, nullptr, 16);
            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
         // invalid float
         else if(numberStr[numberStrLength - 1u] == 'f')
         {
            throwCompileError(pContext, CompileError::InvalidLiteral, numberStr);
            return nullptr;
         }
         // signed
         else
         {
            typeUsage.mType = mTypeInt32;
            const int number = atoi(numberStr);
            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
      }

      expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
      CflatInvokeCtor(ExpressionValue, expression)(value);
   }
   else if(token.mType == TokenType::Identifier)
   {
      // variable access
      pContext.mStringBuffer.assign(token.mStart, token.mLength);
      const Identifier identifier(pContext.mStringBuffer.c_str());

      Instance* instance = retrieveInstance(pContext, identifier);

      if(instance)
      {
         expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
         CflatInvokeCtor(ExpressionVariableAccess, expression)(identifier);
      }
      else
      {
         throwCompileError(pContext, CompileError::UndefinedVariable, identifier.mName);
      }
   }
   else if(token.mType == TokenType::Keyword)
   {
      if(strncmp(token.mStart, "nullptr", 7u) == 0)
      {
         expression = (ExpressionNullPointer*)CflatMalloc(sizeof(ExpressionNullPointer));
         CflatInvokeCtor(ExpressionNullPointer, expression)();
      }
      else if(strncmp(token.mStart, "true", 4u) == 0)
      {
         Value value;
         value.initOnStack(mTypeUsageBool, &mExecutionContext.mStack);

         const bool boolValue = true;
         value.set(&boolValue);

         expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
         CflatInvokeCtor(ExpressionValue, expression)(value);
      }
      else if(strncmp(token.mStart, "false", 5u) == 0)
      {
         Value value;
         value.initOnStack(mTypeUsageBool, &mExecutionContext.mStack);

         const bool boolValue = false;
         value.set(&boolValue);

         expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
         CflatInvokeCtor(ExpressionValue, expression)(value);
      }
   }
   else if(token.mType == TokenType::String)
   {
      expression = parseExpressionLiteralString(pContext);
   }

   return expression;
}

Expression* Environment::parseExpressionMultipleTokens(ParsingContext& pContext, size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Expression* expression = nullptr;

   size_t assignmentOperatorTokenIndex = 0u;
   size_t binaryOperatorTokenIndex = 0u;
   uint8_t binaryOperatorPrecedence = 0u;
   size_t memberAccessTokenIndex = 0u;
   size_t conditionalTokenIndex = 0u;

   uint32_t parenthesisLevel = tokens[pTokenLastIndex].mStart[0] == ')' ? 1u : 0u;
   uint32_t squareBracketLevel = tokens[pTokenLastIndex].mStart[0] == ']' ? 1u : 0u;
   uint32_t templateLevel = tokens[pTokenLastIndex].mStart[0] == '>' ? 1u : 0u;

   for(size_t i = pTokenLastIndex - 1u; i > tokenIndex; i--)
   {
      if(tokens[i].mLength == 1u)
      {
         if(tokens[i].mStart[0] == ')')
         {
            parenthesisLevel++;
            continue;
         }
         else if(tokens[i].mStart[0] == '(')
         {
            parenthesisLevel--;
            continue;
         }
         else if(tokens[i].mStart[0] == ']')
         {
            squareBracketLevel++;
            continue;
         }
         else if(tokens[i].mStart[0] == '[')
         {
            squareBracketLevel--;
            continue;
         }
         else if(tokens[i].mStart[0] == '>')
         {
            const size_t templateOpeningIndex = findOpeningTokenIndex(pContext, '<', '>', i);
            const bool isTemplateClosure =
               templateOpeningIndex < i && isTemplate(pContext, templateOpeningIndex, i);

            if(isTemplateClosure)
            {
               templateLevel++;
               continue;
            }
         }
         else if(tokens[i].mStart[0] == '<')
         {
            const size_t cachedTokenIndex = tokenIndex;
            tokenIndex = i;
            const size_t templateClosureIndex =
               findClosureTokenIndex(pContext, '<', '>', pTokenLastIndex - 1u);
            tokenIndex = cachedTokenIndex;

            const bool isTemplateOpening =
               templateClosureIndex > i && isTemplate(pContext, i, templateClosureIndex);

            if(isTemplateOpening)
            {
               templateLevel--;
               continue;
            }
         }
      }

      if(parenthesisLevel == 0u && squareBracketLevel == 0u && templateLevel == 0u)
      {
         if(i > tokenIndex &&
            tokens[i].mType == TokenType::Operator &&
            tokens[i - 1u].mType != TokenType::Operator)
         {
            if(tokens[i].mLength == 1u && tokens[i].mStart[0] == '?')
            {
               conditionalTokenIndex = i;
            }
            else
            {
               for(size_t j = 0u; j < kCflatAssignmentOperatorsCount; j++)
               {
                  const size_t operatorLength = strlen(kCflatAssignmentOperators[j]);

                  if(tokens[i].mLength == operatorLength &&
                     strncmp(tokens[i].mStart, kCflatAssignmentOperators[j], operatorLength) == 0)
                  {
                     assignmentOperatorTokenIndex = i;
                     break;
                  }
               }

               if(assignmentOperatorTokenIndex == 0u)
               {
                  const uint8_t precedence = getBinaryOperatorPrecedence(pContext, i);

                  if(precedence > binaryOperatorPrecedence)
                  {
                     binaryOperatorTokenIndex = i;
                     binaryOperatorPrecedence = precedence;
                  }
               }
            }
         }
         else if(tokens[i].mType == TokenType::Punctuation && memberAccessTokenIndex == 0u)
         {
            if(tokens[i].mStart[0] == '.' || strncmp(tokens[i].mStart, "->", 2u) == 0)
            {
               memberAccessTokenIndex = i;
            }
         }
      }
   }

   bool isCStyleCast = false;

   if(tokens[tokenIndex].mStart[0] == '(')
   {
      const size_t cachedTokenIndex = tokenIndex;
      tokenIndex++;

      const TypeUsage typeUsage = parseTypeUsage(pContext, pTokenLastIndex - 1u);
      isCStyleCast = typeUsage.mType != nullptr;
      tokenIndex = cachedTokenIndex;
   }

   // assignment
   if(assignmentOperatorTokenIndex > 0u)
   {
      Expression* left = parseExpression(pContext, assignmentOperatorTokenIndex - 1u);

      if(left)
      {
         const TypeUsage leftTypeUsage = getTypeUsage(pContext, left);

         if(!leftTypeUsage.isConst())
         {
            const Token& operatorToken = pContext.mTokens[assignmentOperatorTokenIndex];
            CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

            tokenIndex = assignmentOperatorTokenIndex + 1u;
            Expression* right = parseExpression(pContext, pTokenLastIndex);

            if(right)
            {
               expression = (ExpressionAssignment*)CflatMalloc(sizeof(ExpressionAssignment));
               CflatInvokeCtor(ExpressionAssignment, expression)(left, right, operatorStr.c_str());
            }
            else
            {
               CflatInvokeDtor(Expression, left);
               CflatFree(left);
            }

            tokenIndex = pTokenLastIndex + 1u;
         }
         else
         {
            throwCompileError(pContext, Environment::CompileError::CannotModifyConstExpression);
         }
      }
   }
   // conditional expression
   else if(conditionalTokenIndex > 0u)
   {
      const size_t elseTokenIndex = findClosureTokenIndex(pContext, 0, ':', pTokenLastIndex - 1u);

      if(elseTokenIndex > 0u)
      {
         Expression* condition = parseExpression(pContext, conditionalTokenIndex - 1u);
         tokenIndex = conditionalTokenIndex + 1u;

         Expression* ifExpression = parseExpression(pContext, elseTokenIndex - 1u);
         tokenIndex = elseTokenIndex + 1u;

         Expression* elseExpression = parseExpression(pContext, pTokenLastIndex);
         tokenIndex = pTokenLastIndex + 1u;

         expression = (ExpressionConditional*)CflatMalloc(sizeof(ExpressionConditional));
         CflatInvokeCtor(ExpressionConditional, expression)
            (condition, ifExpression, elseExpression);
      }
      else
      {
         throwCompileError(pContext, CompileError::InvalidConditionalExpression);
      }
   }
   // binary operator
   else if(binaryOperatorTokenIndex > 0u)
   {
      Expression* left = parseExpression(pContext, binaryOperatorTokenIndex - 1u);

      if(left)
      {
         const TypeUsage leftTypeUsage = getTypeUsage(pContext, left);

         const Token& operatorToken = pContext.mTokens[binaryOperatorTokenIndex];
         CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

         tokenIndex = binaryOperatorTokenIndex + 1u;
         Expression* right = parseExpression(pContext, pTokenLastIndex);

         if(right)
         {
            bool operatorIsValid = true;
            TypeUsage overloadedOperatorTypeUsage;

            if(!leftTypeUsage.isPointer() &&
               leftTypeUsage.mType &&
               leftTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
            {
               const TypeUsage rightTypeUsage = getTypeUsage(pContext, right);

               if(rightTypeUsage.mType)
               {
                  CflatArgsVector(TypeUsage) args;
                  args.push_back(rightTypeUsage);

                  pContext.mStringBuffer.assign("operator");
                  pContext.mStringBuffer.append(operatorStr);
                  const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());

                  Method* operatorMethod =
                     findMethod(leftTypeUsage.mType, operatorIdentifier, args);

                  if(operatorMethod)
                  {
                     overloadedOperatorTypeUsage = operatorMethod->mReturnTypeUsage;
                  }
                  else
                  {
                     args.insert(args.begin(), leftTypeUsage);

                     Function* operatorFunction =
                        leftTypeUsage.mType->mNamespace->getFunction(operatorIdentifier, args);

                     if(!operatorFunction)
                     {
                        operatorFunction = findFunction(pContext, operatorIdentifier, args);

                        if(!operatorFunction)
                        {
                           throwCompileError(pContext, CompileError::InvalidOperator,
                              leftTypeUsage.mType->mIdentifier.mName, operatorStr.c_str());
                           operatorIsValid = false;
                        }
                     }

                     if(operatorFunction)
                     {
                        overloadedOperatorTypeUsage = operatorFunction->mReturnTypeUsage;
                     }
                  }
               }
            }

            if(operatorIsValid)
            {
               expression =
                  (ExpressionBinaryOperation*)CflatMalloc(sizeof(ExpressionBinaryOperation));
               CflatInvokeCtor(ExpressionBinaryOperation, expression)
                  (left, right, operatorStr.c_str(), overloadedOperatorTypeUsage);
            }
         }
         else
         {
            CflatInvokeDtor(Expression, left);
            CflatFree(left);
         }
      }

      tokenIndex = pTokenLastIndex + 1u;
   }
   // unary operator
   else if(token.mType == TokenType::Operator)
   {
      // address of
      if(token.mStart[0] == '&')
      {
         tokenIndex++;
         Expression* addressOfExpression = parseExpression(pContext, pTokenLastIndex);

         expression = (ExpressionAddressOf*)CflatMalloc(sizeof(ExpressionAddressOf));
         CflatInvokeCtor(ExpressionAddressOf, expression)(addressOfExpression);
      }
      // indirection
      else if(token.mStart[0] == '*')
      {
         tokenIndex++;
         Expression* indirectionExpression = parseExpression(pContext, pTokenLastIndex);
         const TypeUsage indirectionExpressionTypeUsage =
            getTypeUsage(pContext, indirectionExpression);
         Type* indirectionExpressionType = indirectionExpressionTypeUsage.mType;

         const Identifier operatorMethodID("operator*");
         Method* operatorMethod = nullptr;

         if(indirectionExpressionType &&
            indirectionExpressionType->mCategory == TypeCategory::StructOrClass)
         {
            operatorMethod =
               findMethod(indirectionExpressionType, operatorMethodID, TypeUsage::kEmptyList);
         }

         if(operatorMethod)
         {
            ExpressionMemberAccess* memberAccess =
               (ExpressionMemberAccess*)CflatMalloc(sizeof(ExpressionMemberAccess));
            CflatInvokeCtor(ExpressionMemberAccess, memberAccess)
               (indirectionExpression, operatorMethodID, operatorMethod->mReturnTypeUsage);

            ExpressionMethodCall* methodCall =
               (ExpressionMethodCall*)CflatMalloc(sizeof(ExpressionMethodCall));
            CflatInvokeCtor(ExpressionMethodCall, methodCall)(memberAccess);
            expression = methodCall;

            methodCall->mMethod = operatorMethod;
         }
         else
         {
            expression = (ExpressionIndirection*)CflatMalloc(sizeof(ExpressionIndirection));
            CflatInvokeCtor(ExpressionIndirection, expression)(indirectionExpression);
         }
      }
      // unary operator (pre)
      else
      {
         const Token& operatorToken = token;
         CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);
         tokenIndex++;
         Expression* operandExpression = parseExpression(pContext, pTokenLastIndex);

         if(operandExpression)
         {
            expression =
               parseExpressionUnaryOperator(pContext, operandExpression, operatorStr.c_str(), false);
         }
      }
   }
   // C-style cast
   else if(isCStyleCast)
   {
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, '(', ')', pTokenLastIndex);
      tokenIndex++;

      const TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex);

      if(tokenIndex == closureTokenIndex)
      {
         tokenIndex++;
         Expression* expressionToCast = parseExpression(pContext, pTokenLastIndex);

         expression = (ExpressionCast*)CflatMalloc(sizeof(ExpressionCast));
         CflatInvokeCtor(ExpressionCast, expression)(CastType::CStyle, typeUsage, expressionToCast);

         const TypeUsage sourceTypeUsage = getTypeUsage(pContext, expressionToCast);

         if(!isCastAllowed(CastType::CStyle, sourceTypeUsage, typeUsage))
         {
            throwCompileError(pContext, CompileError::InvalidCast);
         }
      }
      else
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   // member access
   else if(memberAccessTokenIndex > 0u)
   {
      if(tokens[memberAccessTokenIndex + 1u].mType == TokenType::Identifier)
      {
         Expression* memberOwner = parseExpression(pContext, memberAccessTokenIndex - 1u);
         tokenIndex = memberAccessTokenIndex + 1u;

         if(memberOwner)
         {
            pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
            const Identifier memberIdentifier(pContext.mStringBuffer.c_str());

            const bool isMemberAccess = tokens[memberAccessTokenIndex].mStart[0] == '.';
            const bool isPtrMemberAccess =
               !isMemberAccess && strncmp(tokens[memberAccessTokenIndex].mStart, "->", 2u) == 0;

            bool memberAccessIsValid = true;

            const TypeUsage ownerTypeUsage = getTypeUsage(pContext, memberOwner);
            TypeUsage memberTypeUsage;

            bool isMethodCall = false;

            if((tokenIndex + 1u) < tokens.size())
            {
               isMethodCall = tokens[tokenIndex + 1u].mStart[0] == '(';

               if(!isMethodCall)
               {
                  tokenIndex++;
                  isMethodCall = isTemplate(pContext, pTokenLastIndex);
                  tokenIndex--;
               }

               if(!isMethodCall)
               {
                  Struct* type = static_cast<Struct*>(ownerTypeUsage.mType);
                  Member* member = nullptr;

                  for(size_t i = 0u; i < type->mMembers.size(); i++)
                  {
                     if(type->mMembers[i].mIdentifier == memberIdentifier)
                     {
                        member = &type->mMembers[i];
                        break;
                     }
                  }

                  if(member)
                  {
                     memberTypeUsage = member->mTypeUsage;
                  }
                  else
                  {
                     throwCompileError(pContext, CompileError::MissingMember,
                        memberIdentifier.mName);
                     memberAccessIsValid = false;
                  }
               }
            }

            if(memberAccessIsValid)
            {
               if(ownerTypeUsage.isPointer())
               {
                  if(!isPtrMemberAccess)
                  {
                     throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorPtr,
                        memberIdentifier.mName);
                     memberAccessIsValid = false;
                  }
               }
               else
               {
                  if(isPtrMemberAccess)
                  {
                     throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorNonPtr,
                        memberIdentifier.mName);
                     memberAccessIsValid = false;
                  }
               }
            }

            if(memberAccessIsValid)
            {
               ExpressionMemberAccess* memberAccess =
                  (ExpressionMemberAccess*)CflatMalloc(sizeof(ExpressionMemberAccess));
               CflatInvokeCtor(ExpressionMemberAccess, memberAccess)
                  (memberOwner, memberIdentifier, memberTypeUsage);
               expression = memberAccess;

               // method call
               if(isMethodCall)
               {
                  expression = parseExpressionMethodCall(pContext, memberAccess);

                  if(expression)
                  {
                     Method* method = static_cast<ExpressionMethodCall*>(expression)->mMethod;

                     if(method)
                     {
                        memberAccess->mMemberTypeUsage = method->mReturnTypeUsage;
                     }
                  }
               }
            }
         }
      }
      else
      {
         tokenIndex = memberAccessTokenIndex;
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   // parenthesized expression
   else if(tokens[tokenIndex].mStart[0] == '(')
   {
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, '(', ')', pTokenLastIndex);
      tokenIndex++;

      if(closureTokenIndex > tokenIndex)
      {
         Expression* innerExpression = parseExpression(pContext, closureTokenIndex - 1u);

         if(innerExpression)
         {
            expression = (ExpressionParenthesized*)CflatMalloc(sizeof(ExpressionParenthesized));
            CflatInvokeCtor(ExpressionParenthesized, expression)(innerExpression);
         }

         tokenIndex = closureTokenIndex + 1u;
      }
      else
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   // array initialization
   else if(tokens[tokenIndex].mStart[0] == '{')
   {
      tokenIndex++;

      ExpressionArrayInitialization* concreteExpression =
         (ExpressionArrayInitialization*)CflatMalloc(sizeof(ExpressionArrayInitialization));
      CflatInvokeCtor(ExpressionArrayInitialization, concreteExpression)();
      expression = concreteExpression;

      const size_t closureIndex = findClosureTokenIndex(pContext, '{', '}', pTokenLastIndex);

      while(tokenIndex < closureIndex)
      {
         const size_t separatorIndex = findSeparationTokenIndex(pContext, ',', closureIndex);
         const size_t lastArrayValueIndex = separatorIndex > 0u
            ? separatorIndex - 1u
            : closureIndex - 1u;

         Expression* arrayValueExpression = parseExpression(pContext, lastArrayValueIndex);
         concreteExpression->mValues.push_back(arrayValueExpression);

         tokenIndex = lastArrayValueIndex + 2u;
      }
   }
   // unary operator (post)
   else if(tokens[pTokenLastIndex].mType == TokenType::Operator)
   {
      const Token& operatorToken = tokens[pTokenLastIndex];

      if(operatorToken.mLength == 2u)
      {
         if(strncmp(operatorToken.mStart, "++", 2u) == 0 ||
            strncmp(operatorToken.mStart, "--", 2u) == 0)
         {
            CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);
            Expression* operandExpression = parseExpression(pContext, pTokenLastIndex - 1u);

            expression =
               parseExpressionUnaryOperator(pContext, operandExpression, operatorStr.c_str(), true);
         }
      }

      if(!expression)
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   // array element access / operator[]
   else if(tokens[pTokenLastIndex].mStart[0] == ']')
   {
      const size_t openingIndex = findOpeningTokenIndex(pContext, '[', ']', pTokenLastIndex);

      if(pTokenLastIndex > (openingIndex + 1u))
      {
         Expression* arrayAccess = parseExpression(pContext, openingIndex - 1u);

         tokenIndex = openingIndex + 1u;
         Expression* arrayElementIndex = parseExpression(pContext, pTokenLastIndex - 1u);

         const TypeUsage typeUsage = getTypeUsage(pContext, arrayAccess);

         if(typeUsage.isArray() || typeUsage.isPointer())
         {
            expression =
               (ExpressionArrayElementAccess*)CflatMalloc(sizeof(ExpressionArrayElementAccess));
            CflatInvokeCtor(ExpressionArrayElementAccess, expression)
               (arrayAccess, arrayElementIndex);
         }
         else if(typeUsage.mType->mCategory == TypeCategory::StructOrClass)
         {
            const Identifier operatorMethodID("operator[]");
            Method* operatorMethod = findMethod(typeUsage.mType, operatorMethodID);

            if(operatorMethod)
            {
               ExpressionMemberAccess* memberAccess =
                  (ExpressionMemberAccess*)CflatMalloc(sizeof(ExpressionMemberAccess));
               CflatInvokeCtor(ExpressionMemberAccess, memberAccess)
                  (arrayAccess, operatorMethodID, operatorMethod->mReturnTypeUsage);

               ExpressionMethodCall* methodCall =
                  (ExpressionMethodCall*)CflatMalloc(sizeof(ExpressionMethodCall));
               CflatInvokeCtor(ExpressionMethodCall, methodCall)(memberAccess);
               expression = methodCall;

               methodCall->mArguments.push_back(arrayElementIndex);
               methodCall->mMethod = operatorMethod;
            }
            else
            {
               throwCompileErrorUnexpectedSymbol(pContext);
            }
         }
         else
         {
            throwCompileErrorUnexpectedSymbol(pContext);
         }

         tokenIndex = pTokenLastIndex + 1u;
      }
      else
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   else if(token.mType == TokenType::Identifier)
   {
      pContext.mStringBuffer.assign(token.mStart, token.mLength);

      while(tokenIndex < (tokens.size() - 1u) && strncmp(tokens[++tokenIndex].mStart, "::", 2u) == 0)
      {
         tokenIndex++;
         pContext.mStringBuffer.append("::");
         pContext.mStringBuffer.append(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      }

      const Identifier fullIdentifier(pContext.mStringBuffer.c_str());
      bool isFunctionCall = tokens[tokenIndex].mStart[0] == '(';
         
      if(!isFunctionCall && isTemplate(pContext, pTokenLastIndex))
      {
         const size_t templateClosureIndex =
            findClosureTokenIndex(pContext, '<', '>', pTokenLastIndex);
         isFunctionCall =
            templateClosureIndex > 0u && tokens[templateClosureIndex + 1u].mStart[0] == '(';
      }

      // function call / object construction
      if(isFunctionCall)
      {
         Type* type = findType(pContext, fullIdentifier);
         expression = type
            ? parseExpressionObjectConstruction(pContext, type)
            : parseExpressionFunctionCall(pContext, fullIdentifier);
      }
      // variable access with namespace / static member access
      else
      {
         Instance* instance = retrieveInstance(pContext, fullIdentifier);

         if(!instance)
         {
            const char* lastSeparator = fullIdentifier.findLastSeparator();

            if(lastSeparator)
            {
               char buffer[kDefaultLocalStringBufferSize];
               const size_t containerIdentifierLength = lastSeparator - fullIdentifier.mName;
               strncpy(buffer, fullIdentifier.mName, containerIdentifierLength);
               buffer[containerIdentifierLength] = '\0';
               const Identifier containerIdentifier(buffer);
               const Identifier memberIdentifier(lastSeparator + 2);

               Type* type = findType(pContext, containerIdentifier);

               if(type && type->mCategory == TypeCategory::StructOrClass)
               {
                  instance =
                     static_cast<Struct*>(type)->mInstancesHolder.retrieveInstance(memberIdentifier);

                  if(!instance)
                  {
                     throwCompileError(pContext, Environment::CompileError::MissingStaticMember,
                        memberIdentifier.mName, containerIdentifier.mName);
                  }
               }
            }
         }

         if(instance)
         {
            expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
            CflatInvokeCtor(ExpressionVariableAccess, expression)(fullIdentifier);
         }
         else
         {
            throwCompileError(pContext, Environment::CompileError::UndefinedType,
               fullIdentifier.mName);
         }
      }
   }
   else if(token.mType == TokenType::Keyword)
   {
      if(strncmp(token.mStart, "sizeof", 6u) == 0)
      {
         if(tokens[tokenIndex + 1u].mStart[0] == '(')
         {
            tokenIndex++;
            const size_t closureTokenIndex =
               findClosureTokenIndex(pContext, '(', ')', pTokenLastIndex);
            tokenIndex++;

            ExpressionSizeOf* concreteExpression =
               (ExpressionSizeOf*)CflatMalloc(sizeof(ExpressionSizeOf));
            CflatInvokeCtor(ExpressionSizeOf, concreteExpression)();
            expression = concreteExpression;

            concreteExpression->mTypeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

            if(!concreteExpression->mTypeUsage.mType)
            {
               concreteExpression->mExpression = parseExpression(pContext, closureTokenIndex - 1u);
            }

            tokenIndex = closureTokenIndex;
         }
         else
         {
            throwCompileErrorUnexpectedSymbol(pContext);
         }
      }
      else if(strncmp(token.mStart, "static_cast", 11u) == 0)
      {
         tokenIndex++;
         expression = parseExpressionCast(pContext, CastType::Static, pTokenLastIndex);
      }
      else if(strncmp(token.mStart, "dynamic_cast", 12u) == 0)
      {
         tokenIndex++;
         expression = parseExpressionCast(pContext, CastType::Dynamic, pTokenLastIndex);
      }
      else if(strncmp(token.mStart, "reinterpret_cast", 16u) == 0)
      {
         tokenIndex++;
         expression = parseExpressionCast(pContext, CastType::Reinterpret, pTokenLastIndex);
      }
   }
   else if(token.mType == TokenType::String)
   {
      expression = parseExpressionLiteralString(pContext);
   }

   return expression;
}

Expression* Environment::parseExpressionLiteralString(ParsingContext& pContext)
{
   pContext.mStringBuffer.clear();

   do
   {
      const Token& token = pContext.mTokens[pContext.mTokenIndex];

      for(size_t i = 1u; i < (token.mLength - 1u); i++)
      {
         const char currentChar = *(token.mStart + i);

         if(currentChar == '\\')
         {
            const char escapeChar = *(token.mStart + i + 1u);

            if(escapeChar == 'n')
            {
               pContext.mStringBuffer.push_back('\n');
            }
            else
            {
               pContext.mStringBuffer.push_back('\\');
            }

            i++;
         }
         else
         {
            pContext.mStringBuffer.push_back(currentChar);
         }
      }

      pContext.mTokenIndex++;
   }
   while(pContext.mTokenIndex < pContext.mTokens.size() &&
      pContext.mTokens[pContext.mTokenIndex].mType == TokenType::String);

   pContext.mStringBuffer.push_back('\0');

   const Hash stringHash = hash(pContext.mStringBuffer.c_str());
   const char* string =
      mLiteralStringsPool.registerString(stringHash, pContext.mStringBuffer.c_str());

   Value value;
   value.initOnStack(mTypeUsageCString, &mExecutionContext.mStack);
   value.set(&string);

   ExpressionValue* expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
   CflatInvokeCtor(ExpressionValue, expression)(value);

   return expression;
}

Expression* Environment::parseExpressionUnaryOperator(ParsingContext& pContext, Expression* pOperand,
   const char* pOperator, bool pPostOperator)
{
   Expression* expression = nullptr;
   bool validOperation = true;

   const bool isIncrementOrDecrement =
      strncmp(pOperator, "++", 2u) == 0 ||
      strncmp(pOperator, "--", 2u) == 0;

   if(isIncrementOrDecrement)
   {
      const TypeUsage operandTypeUsage = getTypeUsage(pContext, pOperand);

      if(operandTypeUsage.isConst())
      {
         throwCompileError(pContext, Environment::CompileError::CannotModifyConstExpression);
         validOperation = false;
      }
   }

   if(validOperation)
   {
      expression = (ExpressionUnaryOperation*)CflatMalloc(sizeof(ExpressionUnaryOperation));
      CflatInvokeCtor(ExpressionUnaryOperation, expression)(pOperand, pOperator, pPostOperator);
   }

   return expression;
}

Expression* Environment::parseExpressionCast(ParsingContext& pContext, CastType pCastType,
   size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   Expression* expression = nullptr;

   if(tokens[tokenIndex].mStart[0] == '<')
   {
      tokenIndex++;
      const TypeUsage targetTypeUsage = parseTypeUsage(pContext, pTokenLastIndex - 1u);

      if(targetTypeUsage.mType)
      {
         if(tokens[tokenIndex].mStart[0] == '>')
         {
            tokenIndex++;

            if(tokens[tokenIndex].mStart[0] == '(')
            {
               tokenIndex++;
               const size_t closureTokenIndex =
                  findClosureTokenIndex(pContext, '(', ')', pTokenLastIndex);

               if(closureTokenIndex > 0u)
               {
                  Expression* expressionToCast = parseExpression(pContext, closureTokenIndex - 1u);
                  const TypeUsage sourceTypeUsage = getTypeUsage(pContext, expressionToCast);

                  if(isCastAllowed(pCastType, sourceTypeUsage, targetTypeUsage))
                  {
                     expression = (ExpressionCast*)CflatMalloc(sizeof(ExpressionCast));
                     CflatInvokeCtor(ExpressionCast, expression)
                        (pCastType, targetTypeUsage, expressionToCast);
                  }
                  else
                  {
                     throwCompileError(pContext, CompileError::InvalidCast);
                  }
               }
               else
               {
                  tokenIndex = pTokenLastIndex - 1u;
                  throwCompileErrorUnexpectedSymbol(pContext);
               }
            }
            else
            {
               throwCompileErrorUnexpectedSymbol(pContext);
            }
         }
      }
      else
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   else
   {
      throwCompileErrorUnexpectedSymbol(pContext);
   }

   return expression;
}

Expression* Environment::parseExpressionFunctionCall(ParsingContext& pContext,
   const Identifier& pFunctionIdentifier)
{
   ExpressionFunctionCall* expression =
      (ExpressionFunctionCall*)CflatMalloc(sizeof(ExpressionFunctionCall));
   CflatInvokeCtor(ExpressionFunctionCall, expression)(pFunctionIdentifier);

   parseFunctionCallArguments(pContext, &expression->mArguments, &expression->mTemplateTypes);

   if(!mErrorMessage.empty())
   {
      return nullptr;
   }

   CflatArgsVector(TypeUsage) argumentTypes;

   for(size_t i = 0u; i < expression->mArguments.size(); i++)
   {
      const TypeUsage typeUsage = getTypeUsage(pContext, expression->mArguments[i]);
      argumentTypes.push_back(typeUsage);
   }

   CflatArgsVector(TypeUsage) templateTypes;
   toArgsVector(expression->mTemplateTypes, templateTypes);

   expression->mFunction = findFunction(pContext, pFunctionIdentifier, argumentTypes, templateTypes);

   if(!expression->mFunction)
   {
      const char* lastSeparator = pFunctionIdentifier.findLastSeparator();

      if(lastSeparator)
      {
         char buffer[kDefaultLocalStringBufferSize];
         const size_t typeIdentifierLength = lastSeparator - pFunctionIdentifier.mName;
         strncpy(buffer, pFunctionIdentifier.mName, typeIdentifierLength);
         buffer[typeIdentifierLength] = '\0';
         const Identifier typeIdentifier(buffer);
         const Identifier staticMethodIdentifier(lastSeparator + 2);

         Type* type = findType(pContext, typeIdentifier);

         if(type && type->mCategory == TypeCategory::StructOrClass)
         {
            Struct* castedType = static_cast<Struct*>(type);
            expression->mFunction =
               castedType->getStaticMethod(staticMethodIdentifier, argumentTypes, templateTypes);

            if(!expression->mFunction)
            {
               throwCompileError(pContext, CompileError::MissingStaticMethod,
                  staticMethodIdentifier.mName, typeIdentifier.mName);
            }
         }
      }
   }

   if(!expression->mFunction)
   {
      CflatInvokeDtor(ExpressionFunctionCall, expression);
      CflatFree(expression);
      expression = nullptr;

      throwCompileError(pContext, CompileError::UndefinedFunction, pFunctionIdentifier.mName);
   }

   return expression;
}

Expression* Environment::parseExpressionMethodCall(ParsingContext& pContext, Expression* pMemberAccess)
{
   ExpressionMethodCall* expression = 
      (ExpressionMethodCall*)CflatMalloc(sizeof(ExpressionMethodCall));
   CflatInvokeCtor(ExpressionMethodCall, expression)(pMemberAccess);

   pContext.mTokenIndex++;
   parseFunctionCallArguments(pContext, &expression->mArguments, &expression->mTemplateTypes);

   if(!mErrorMessage.empty())
   {
      return nullptr;
   }

   ExpressionMemberAccess* memberAccess = static_cast<ExpressionMemberAccess*>(pMemberAccess);
   const TypeUsage methodOwnerTypeUsage = getTypeUsage(pContext, memberAccess->mMemberOwner);

   Type* methodOwnerType = methodOwnerTypeUsage.mType;
   CflatAssert(methodOwnerType);
   CflatAssert(methodOwnerType->mCategory == TypeCategory::StructOrClass);

   CflatArgsVector(TypeUsage) argumentTypes;

   for(size_t i = 0u; i < expression->mArguments.size(); i++)
   {
      const TypeUsage typeUsage = getTypeUsage(pContext, expression->mArguments[i]);
      argumentTypes.push_back(typeUsage);
   }

   CflatArgsVector(TypeUsage) templateTypes;
   toArgsVector(expression->mTemplateTypes, templateTypes);

   const Identifier& methodId = memberAccess->mMemberIdentifier;
   expression->mMethod = findMethod(methodOwnerType, methodId, argumentTypes, templateTypes);

   if(!expression->mMethod)
   {
      throwCompileError(pContext, CompileError::MissingMethod, methodId.mName);
   }

   return expression;
}

Expression* Environment::parseExpressionObjectConstruction(ParsingContext& pContext, Type* pType)
{
   ExpressionObjectConstruction* expression =
      (ExpressionObjectConstruction*)CflatMalloc(sizeof(ExpressionObjectConstruction));
   CflatInvokeCtor(ExpressionObjectConstruction, expression)(pType);

   parseFunctionCallArguments(pContext, &expression->mArguments);

   if(!mErrorMessage.empty())
   {
      return nullptr;
   }

   CflatArgsVector(TypeUsage) argumentTypes;

   for(size_t i = 0u; i < expression->mArguments.size(); i++)
   {
      const TypeUsage typeUsage = getTypeUsage(pContext, expression->mArguments[i]);
      argumentTypes.push_back(typeUsage);
   }

   expression->mConstructor = findConstructor(pType, argumentTypes);

   if(!expression->mConstructor)
   {
      throwCompileError(pContext, CompileError::MissingConstructor);
   }

   return expression;
}

size_t Environment::findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
   size_t pTokenIndexLimit)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t closureTokenIndex = 0u;

   if(pTokenIndexLimit == 0u)
   {
      pTokenIndexLimit = pContext.mTokens.size() - 1u;
   }

   if(tokens[pContext.mTokenIndex].mStart[0] == pClosureChar)
   {
      closureTokenIndex = pContext.mTokenIndex;
   }
   else
   {
      uint32_t scopeLevel = 0u;

      for(size_t i = (pContext.mTokenIndex + 1u); i <= pTokenIndexLimit; i++)
      {
         if(tokens[i].mLength > 1u)
         {
            continue;
         }

         if(tokens[i].mStart[0] == pClosureChar)
         {
            if(scopeLevel == 0u)
            {
               closureTokenIndex = i;
               break;
            }

            scopeLevel--;
         }
         else if(tokens[i].mStart[0] == pOpeningChar)
         {
            scopeLevel++;
         }
      }
   }

   return closureTokenIndex;
}

size_t Environment::findOpeningTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
   size_t pClosureIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t openingTokenIndex = pClosureIndex;

   if(openingTokenIndex > 0u)
   {
      uint32_t scopeLevel = 0u;

      for(int i = (int)pClosureIndex - 1; i >= (int)pContext.mTokenIndex; i--)
      {
         if(tokens[i].mLength > 1u)
         {
            continue;
         }

         if(tokens[i].mStart[0] == pOpeningChar)
         {
            if(scopeLevel == 0u)
            {
               openingTokenIndex = i;
               break;
            }

            scopeLevel--;
         }
         else if(tokens[i].mStart[0] == pClosureChar)
         {
            scopeLevel++;
         }
      }
   }

   return openingTokenIndex;
}

size_t Environment::findSeparationTokenIndex(ParsingContext& pContext, char pSeparationChar,
   size_t pClosureIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t separationTokenIndex = 0u;

   uint32_t scopeLevel = 0u;

   for(size_t i = pContext.mTokenIndex; i < pClosureIndex; i++)
   {
      if(tokens[i].mLength > 1u)
      {
         continue;
      }

      if(i > pContext.mTokenIndex && tokens[i].mStart[0] == pSeparationChar && scopeLevel == 0u)
      {
         separationTokenIndex = i;
         break;
      }

      if(tokens[i].mStart[0] == '(')
      {
         scopeLevel++;
      }
      else if(tokens[i].mStart[0] == ')')
      {
         scopeLevel--;
      }
   }

   return separationTokenIndex;
}

uint8_t Environment::getBinaryOperatorPrecedence(ParsingContext& pContext, size_t pTokenIndex)
{
   const Token& token = pContext.mTokens[pTokenIndex];
   CflatAssert(token.mType == TokenType::Operator);

   uint8_t precedence = 0u;

   for(size_t i = 0u; i < kCflatBinaryOperatorsCount; i++)
   {
      const size_t operatorLength = strlen(kCflatBinaryOperators[i]);

      if(token.mLength == operatorLength &&
         strncmp(token.mStart, kCflatBinaryOperators[i], operatorLength) == 0)
      {
         precedence = kCflatBinaryOperatorsPrecedence[i];
         break;
      }
   }

   return precedence;
}

bool Environment::isTemplate(ParsingContext& pContext, size_t pOpeningTokenIndex, size_t pClosureTokenIndex)
{
   if(pClosureTokenIndex <= pOpeningTokenIndex)
      return false;

   CflatSTLVector(Token)& tokens = pContext.mTokens;

   const Token& openingToken = tokens[pOpeningTokenIndex];

   if(openingToken.mLength != 1u || openingToken.mStart[0] != '<')
      return false;

   const Token& closureToken = tokens[pClosureTokenIndex];

   if(closureToken.mLength != 1u || closureToken.mStart[0] != '>')
      return false;

   for(size_t i = pOpeningTokenIndex + 1u; i < pClosureTokenIndex; i++)
   {
      if(tokens[i].mType == TokenType::Operator)
      {
         const bool isPointerOperator =
            tokens[i].mLength == 1u &&
            tokens[i].mStart[0] == '*' &&
            tokens[i - 1u].mType == TokenType::Identifier;

         if(!isPointerOperator)
            return false;
      }
   }

   return true;
}

bool Environment::isTemplate(ParsingContext& pContext, size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mLength != 1u || tokens[tokenIndex].mStart[0] != '<')
      return false;

   const size_t templateClosureTokenIndex =
      findClosureTokenIndex(pContext, '<', '>', pTokenLastIndex);

   return isTemplate(pContext, tokenIndex, templateClosureTokenIndex);
}

bool Environment::isCastAllowed(CastType pCastType, const TypeUsage& pFrom, const TypeUsage& pTo)
{
   bool castAllowed = false;

   switch(pCastType)
   {
   case CastType::CStyle:
   case CastType::Static:
      if(pFrom.mType->mCategory == TypeCategory::BuiltIn &&
         pTo.mType->mCategory == TypeCategory::BuiltIn)
      {
         castAllowed = true;
      }
      else if(pFrom.mType->mCategory == TypeCategory::StructOrClass &&
         pFrom.isPointer() &&
         pTo.mType->mCategory == TypeCategory::StructOrClass &&
         pTo.isPointer())
      {
         Struct* sourceType = static_cast<Struct*>(pFrom.mType);
         Struct* targetType = static_cast<Struct*>(pTo.mType);
         castAllowed =
            sourceType->derivedFrom(targetType) || targetType->derivedFrom(sourceType);
      }
      break;
   case CastType::Dynamic:
      castAllowed =
         pFrom.isPointer() &&
         pFrom.mType->mCategory == TypeCategory::StructOrClass &&
         pTo.isPointer() &&
         pTo.mType->mCategory == TypeCategory::StructOrClass;
      break;
   case CastType::Reinterpret:
      castAllowed =
         pFrom.isPointer() &&
         pTo.isPointer();
      break;
   default:
      break;
   }

   return castAllowed;
}

Statement* Environment::parseStatement(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   Statement* statement = nullptr;
   const uint16_t statementLine = token.mLine;

   if(token.mType == TokenType::Punctuation && (token.mStart[0] == '{' || token.mStart[0] == '}'))
   {
      // block
      if(token.mStart[0] == '{')
      {
         statement = parseStatementBlock(pContext, true, false);
      }
   }
   else if(token.mType == TokenType::Keyword &&
      strncmp(token.mStart, "const", 5u) != 0 &&
      strncmp(token.mStart, "static", 6u) != 0 &&
      strncmp(token.mStart, "void", 4u) != 0)
   {
      // if
      if(strncmp(token.mStart, "if", 2u) == 0)
      {
         tokenIndex++;
         statement = parseStatementIf(pContext);
      }
      // switch
      else if(strncmp(token.mStart, "switch", 6u) == 0)
      {
         tokenIndex++;
         statement = parseStatementSwitch(pContext);
      }
      // while
      else if(strncmp(token.mStart, "while", 5u) == 0)
      {
         tokenIndex++;
         statement = parseStatementWhile(pContext);
      }
      // do
      else if(strncmp(token.mStart, "do", 2u) == 0)
      {
         tokenIndex++;
         statement = parseStatementDoWhile(pContext);
      }
      // for
      else if(strncmp(token.mStart, "for", 3u) == 0)
      {
         tokenIndex++;
         statement = parseStatementFor(pContext);
      }
      // break
      else if(strncmp(token.mStart, "break", 5u) == 0)
      {
         tokenIndex++;
         statement = parseStatementBreak(pContext);
      }
      // continue
      else if(strncmp(token.mStart, "continue", 8u) == 0)
      {
         tokenIndex++;
         statement = parseStatementContinue(pContext);
      }
      // return
      else if(strncmp(token.mStart, "return", 6u) == 0)
      {
         tokenIndex++;
         statement = parseStatementReturn(pContext);
      }
      // using
      else if(strncmp(token.mStart, "using", 5u) == 0)
      {
         tokenIndex++;
         statement = parseStatementUsingDirective(pContext);
      }
      // struct
      else if(strncmp(token.mStart, "struct", 6u) == 0)
      {
         tokenIndex++;
         statement = parseStatementStructDeclaration(pContext);
      }
      // namespace
      else if(strncmp(token.mStart, "namespace", 9u) == 0)
      {
         tokenIndex++;
         statement = parseStatementNamespaceDeclaration(pContext);
      }
      // typedef
      else if(strncmp(token.mStart, "typedef", 7u) == 0)
      {
         tokenIndex++;
         statement = parseStatementTypeDefinition(pContext);
      }
      // (unexpected)
      else
      {
         throwCompileErrorUnexpectedSymbol(pContext);
      }
   }
   else
   {
      // static?
      bool staticDeclaration = false;

      if(strncmp(token.mStart, "static", 6u) == 0)
      {
         staticDeclaration = true;
         tokenIndex++;
      }

      // type
      TypeUsage typeUsage = parseTypeUsage(pContext, 0u);

      if(typeUsage.mType)
      {
         const Token& identifierToken = tokens[tokenIndex];
         pContext.mStringBuffer.assign(identifierToken.mStart, identifierToken.mLength);
         const Identifier identifier(pContext.mStringBuffer.c_str());
         tokenIndex++;

         if(tokens[tokenIndex].mType != TokenType::Operator &&
            tokens[tokenIndex].mType != TokenType::Punctuation)
         {
            pContext.mStringBuffer.assign(token.mStart, token.mLength);
            throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
            return nullptr;
         }

         bool isFunctionDeclaration = tokens[tokenIndex].mStart[0] == '(';

         if(isFunctionDeclaration)
         {
            const size_t nextSemicolonIndex = findClosureTokenIndex(pContext, 0, ';');
            const size_t nextBracketIndex = findClosureTokenIndex(pContext, 0, '{');
               
            if(nextBracketIndex == 0u ||
               (nextSemicolonIndex > 0u && nextSemicolonIndex < nextBracketIndex))
            {
               // object construction
               isFunctionDeclaration = false;
            }
         }

         // function declaration
         if(isFunctionDeclaration)
         {
            tokenIndex--;
            statement = parseStatementFunctionDeclaration(pContext, typeUsage);
         }
         // variable / const declaration
         else
         {
            if(typeUsage.mType != mTypeVoid || typeUsage.mPointerLevel > 0u)
            {
               statement =
                  parseStatementVariableDeclaration(pContext, typeUsage, identifier, staticDeclaration);
            }
            else
            {
               throwCompileError(pContext, Environment::CompileError::InvalidType, "void");
            }
         }
      }
      // expression
      else
      {
         const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

         if(closureTokenIndex == 0u)
         {
            throwCompileError(pContext, CompileError::Expected, ";");
            return nullptr;
         }

         Expression* expression = parseExpression(pContext, closureTokenIndex - 1u);
         tokenIndex = closureTokenIndex;

         statement = (StatementExpression*)CflatMalloc(sizeof(StatementExpression));
         CflatInvokeCtor(StatementExpression, statement)(expression);
      }
   }

   if(statement)
   {
      statement->mProgram = pContext.mProgram;
      statement->mLine = statementLine;
   }

   return statement;
}

StatementBlock* Environment::parseStatementBlock(ParsingContext& pContext,
   bool pAlterScope, bool pAllowInGlobalScope)
{
   if(!pAllowInGlobalScope && pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   if(token.mStart[0] != '{')
   {
      throwCompileError(pContext, CompileError::Expected, "{");
      return nullptr;
   }

   StatementBlock* block = (StatementBlock*)CflatMalloc(sizeof(StatementBlock));
   CflatInvokeCtor(StatementBlock, block)(pAlterScope);

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, '{', '}');

   if(closureTokenIndex > 0u)
   {
      incrementBlockLevel(pContext);

      if(pAlterScope)
      {
         incrementScopeLevel(pContext);
         incrementScopeLevel(mExecutionContext);
      }

      while(tokenIndex < closureTokenIndex)
      {
         tokenIndex++;
         Statement* statement = parseStatement(pContext);

         if(!mErrorMessage.empty())
         {
            break;
         }

         if(statement)
         {
            block->mStatements.push_back(statement);
         }
      }

      if(pAlterScope)
      {
         decrementScopeLevel(mExecutionContext);
         decrementScopeLevel(pContext);
      }

      decrementBlockLevel(pContext);
   }
   else
   {
      throwCompileError(pContext, CompileError::Expected, "}");
   }

   if(mErrorMessage.empty())
   {
      block->mProgram = pContext.mProgram;
      block->mLine = token.mLine;
   }
   else
   {
      CflatInvokeDtor(StatementBlock, block);
      CflatFree(block);
      block = nullptr;
   }

   return block;
}

StatementUsingDirective* Environment::parseStatementUsingDirective(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   StatementUsingDirective* statement = nullptr;
   const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

   if(closureTokenIndex > 0u)
   {
      if(strncmp(token.mStart, "namespace", 9u) == 0)
      {
         tokenIndex++;
         Token& namespaceToken = const_cast<Token&>(pContext.mTokens[tokenIndex]);
         pContext.mStringBuffer.clear();

         do
         {
            pContext.mStringBuffer.append(namespaceToken.mStart, namespaceToken.mLength);
            tokenIndex++;
            namespaceToken = tokens[tokenIndex];
         }
         while(*namespaceToken.mStart != ';');

         const Identifier identifier(pContext.mStringBuffer.c_str());
         Namespace* ns = nullptr;

         for(int i = (int)pContext.mNamespaceStack.size() - 1; i >= 0; i--)
         {
            ns = pContext.mNamespaceStack[i]->getNamespace(identifier);

            if(ns)
            {
               break;
            }
         }

         if(ns)
         {
            UsingDirective usingDirective(ns);
            usingDirective.mBlockLevel = pContext.mBlockLevel;
            pContext.mUsingDirectives.push_back(usingDirective);

            statement = (StatementUsingDirective*)CflatMalloc(sizeof(StatementUsingDirective));
            CflatInvokeCtor(StatementUsingDirective, statement)(ns);
         }
         else
         {
            throwCompileError(pContext, CompileError::UnknownNamespace, identifier.mName);
         }
      }
      else if(tokenIndex < closureTokenIndex && tokens[tokenIndex].mType == TokenType::Identifier)
      {
         const size_t equalTokenIndex = findClosureTokenIndex(pContext, 0, '=', closureTokenIndex);

         if(equalTokenIndex > 0u)
         {
            pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
            const Identifier alias(pContext.mStringBuffer.c_str());
            tokenIndex++;

            if(tokenIndex == equalTokenIndex)
            {
               tokenIndex++;

               const TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

               if(typeUsage.mType && tokenIndex == closureTokenIndex)
               {
                  registerTypeAlias(pContext, alias, typeUsage);

                  statement = (StatementUsingDirective*)CflatMalloc(sizeof(StatementUsingDirective));
                  CflatInvokeCtor(StatementUsingDirective, statement)(alias, typeUsage);
               }
               else
               {
                  throwCompileErrorUnexpectedSymbol(pContext);
               }
            }
            else
            {
               throwCompileErrorUnexpectedSymbol(pContext);
            }
         }
         else
         {
            const TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

            if(typeUsage.mType && tokenIndex == closureTokenIndex && typeUsage.mFlags == 0u)
            {
               const Identifier& alias = typeUsage.mType->mIdentifier;
               registerTypeAlias(pContext, alias, typeUsage);

               statement = (StatementUsingDirective*)CflatMalloc(sizeof(StatementUsingDirective));
               CflatInvokeCtor(StatementUsingDirective, statement)(alias, typeUsage);
            }
            else
            {
               throwCompileErrorUnexpectedSymbol(pContext);
            }
         }
      }
      else
      {
         throwCompileError(pContext, CompileError::UnexpectedSymbol, "using");
      }

      tokenIndex = closureTokenIndex;
   }
   else
   {
      throwCompileError(pContext, CompileError::Expected, ";");
   }

   return statement;
}

StatementTypeDefinition* Environment::parseStatementTypeDefinition(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   StatementTypeDefinition* statement = nullptr;
   const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

   if(closureTokenIndex > 0u)
   {
      const TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

      if(typeUsage.mType)
      {
         if(tokenIndex == (closureTokenIndex - 1u) && tokens[tokenIndex].mType == TokenType::Identifier)
         {
            pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
            const Identifier alias(pContext.mStringBuffer.c_str());
            registerTypeAlias(pContext, alias, typeUsage);

            statement = (StatementTypeDefinition*)CflatMalloc(sizeof(StatementTypeDefinition));
            CflatInvokeCtor(StatementTypeDefinition, statement)(alias, typeUsage);
         }
         else
         {
            throwCompileErrorUnexpectedSymbol(pContext);
         }
      }
      else
      {
         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         throwCompileError(pContext, CompileError::UndefinedType, pContext.mStringBuffer.c_str());
      }

      tokenIndex = closureTokenIndex;
   }
   else
   {
      throwCompileError(pContext, CompileError::Expected, ";");
   }

   return statement;
}

StatementNamespaceDeclaration* Environment::parseStatementNamespaceDeclaration(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   StatementNamespaceDeclaration* statement = nullptr;

   if(token.mType == TokenType::Identifier)
   {
      pContext.mStringBuffer.assign(token.mStart, token.mLength);
      const Identifier nsIdentifier(pContext.mStringBuffer.c_str());

      Namespace* ns = pContext.mNamespaceStack.back()->requestNamespace(nsIdentifier);
      pContext.mNamespaceStack.push_back(ns);
      mExecutionContext.mNamespaceStack.push_back(ns);

      statement = (StatementNamespaceDeclaration*)CflatMalloc(sizeof(StatementNamespaceDeclaration));
      CflatInvokeCtor(StatementNamespaceDeclaration, statement)(nsIdentifier);

      tokenIndex++;
      statement->mBody = parseStatementBlock(pContext, false, true);

      mExecutionContext.mNamespaceStack.pop_back();
      pContext.mNamespaceStack.pop_back();
   }
   else
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "namespace");
   }

   return statement;
}

StatementVariableDeclaration* Environment::parseStatementVariableDeclaration(ParsingContext& pContext,
   TypeUsage& pTypeUsage, const Identifier& pIdentifier, bool pStatic)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   StatementVariableDeclaration* statement = nullptr;

   bool instanceAlreadyRegistered = false;

   for(size_t i = 0u; i < pContext.mRegisteredInstances.size(); i++)
   {
      if(pContext.mRegisteredInstances[i].mIdentifier == pIdentifier &&
         pContext.mRegisteredInstances[i].mNamespace == pContext.mNamespaceStack.back() &&
         pContext.mRegisteredInstances[i].mScopeLevel == pContext.mScopeLevel)
      {
         instanceAlreadyRegistered = true;
         break;
      }
   }

   if(!instanceAlreadyRegistered)
   {
      Expression* initialValueExpression = nullptr;

      // array
      if(token.mStart[0] == '[')
      {
         uint16_t arraySize = 0u;

         const size_t arrayClosure = findClosureTokenIndex(pContext, '[', ']');
         const bool arraySizeSpecified = arrayClosure > (tokenIndex + 1u);

         if(arraySizeSpecified)
         {
            tokenIndex++;
            Expression* arraySizeExpression = parseExpression(pContext, arrayClosure - 1u);
            CflatAssert(arraySizeExpression);

            Value arraySizeValue;
            arraySizeValue.initOnStack(mTypeUsageSizeT, &mExecutionContext.mStack);
            evaluateExpression(mExecutionContext, arraySizeExpression, &arraySizeValue);

            arraySize = (uint16_t)CflatValueAs(&arraySizeValue, size_t);

            CflatInvokeDtor(Expression, arraySizeExpression);
            CflatFree(arraySizeExpression);
         }

         tokenIndex = arrayClosure + 1u;

         if(tokens[tokenIndex].mStart[0] == '=')
         {
            tokenIndex++;

            const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

            if(closureTokenIndex > 0u)
            {
               initialValueExpression = parseExpression(pContext, closureTokenIndex - 1u);

               if(!initialValueExpression ||
                  initialValueExpression->getType() != ExpressionType::ArrayInitialization)
               {
                  throwCompileError(pContext, CompileError::ArrayInitializationExpected);
                  return nullptr;
               }

               ExpressionArrayInitialization* arrayInitialization =
                  static_cast<ExpressionArrayInitialization*>(initialValueExpression);
               arrayInitialization->mElementType = pTypeUsage.mType;

               if(!arraySizeSpecified)
               {
                  arraySize = (uint16_t)arrayInitialization->mValues.size();
               }

               tokenIndex = closureTokenIndex;
            }
            else
            {
               throwCompileError(pContext, CompileError::Expected, ";");
               return nullptr;
            }
         }
         else if(!arraySizeSpecified)
         {
            throwCompileError(pContext, CompileError::ArrayInitializationExpected);
            return nullptr;
         }

         CflatAssert(arraySize > 0u);
         CflatSetFlag(pTypeUsage.mFlags, TypeUsageFlags::Array);
         pTypeUsage.mArraySize = arraySize;
      }
      // variable/object
      else if(token.mStart[0] == '=')
      {
         tokenIndex++;

         const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

         if(closureTokenIndex > 0u)
         {
            initialValueExpression = parseExpression(pContext, closureTokenIndex - 1u);

            if(pTypeUsage.mType == mTypeAuto)
            {
               const TypeUsage resolvedTypeUsage = getTypeUsage(pContext, initialValueExpression);
               pTypeUsage.mType = resolvedTypeUsage.mType;
            }

            tokenIndex = closureTokenIndex;
         }
         else
         {
            throwCompileError(pContext, CompileError::Expected, ";");
            return nullptr;
         }
      }
      // object with construction
      else if(pTypeUsage.mType &&
         pTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
         !pTypeUsage.isPointer())
      {
         Type* type = pTypeUsage.mType;

         if(token.mStart[0] == '(')
         {
            initialValueExpression = parseExpressionObjectConstruction(pContext, type);
         }
         else
         {
            const Identifier emptyId;
            Method* anyCtor = findMethod(type, emptyId);

            if(anyCtor)
            {
               Method* defaultCtor = getDefaultConstructor(type);

               if(!defaultCtor)
               {
                  throwCompileError(pContext, CompileError::NoDefaultConstructor,
                     type->mIdentifier.mName);
                  return nullptr;
               }
            }
         }
      }

      if(pTypeUsage.isReference() && !initialValueExpression)
      {
         throwCompileError(pContext, CompileError::UninitializedReference, pIdentifier.mName);
         return nullptr;
      }

      registerInstance(pContext, pTypeUsage, pIdentifier);

      ParsingContext::RegisteredInstance registeredInstance;
      registeredInstance.mIdentifier = pIdentifier;
      registeredInstance.mNamespace = pContext.mNamespaceStack.back();
      registeredInstance.mScopeLevel = pContext.mScopeLevel;
      pContext.mRegisteredInstances.push_back(registeredInstance);

      statement = (StatementVariableDeclaration*)CflatMalloc(sizeof(StatementVariableDeclaration));
      CflatInvokeCtor(StatementVariableDeclaration, statement)
         (pTypeUsage, pIdentifier, initialValueExpression, pStatic);

      if(initialValueExpression)
      {
         bool validAssignment = false;

         if(pTypeUsage.isPointer() && initialValueExpression->getType() == ExpressionType::NullPointer)
         {
            validAssignment = true;
         }
         else
         {
            const TypeUsage initialValueTypeUsage = getTypeUsage(pContext, initialValueExpression);

            if(initialValueTypeUsage.mType)
            {
               if(initialValueTypeUsage.mType != mTypeVoid || initialValueTypeUsage.isPointer())
               {
                  if(pTypeUsage.isPointer() && !pTypeUsage.isArray() &&
                     !initialValueTypeUsage.isPointer() && initialValueTypeUsage.isArray() &&
                     pTypeUsage.mType == initialValueTypeUsage.mType)
                  {
                     validAssignment = true;
                  }
                  else
                  {
                     const TypeHelper::Compatibility compatibility =
                        TypeHelper::getCompatibility(pTypeUsage, initialValueTypeUsage);
                     validAssignment = compatibility != TypeHelper::Compatibility::Incompatible;
                  }
               }
            }
         }

         if(validAssignment)
         {
            if(pStatic && pTypeUsage.isConst())
            {
               Instance* execInstance = registerInstance(mExecutionContext, pTypeUsage, pIdentifier);

               Value initialValue;
               initialValue.mValueInitializationHint = ValueInitializationHint::Stack;
               evaluateExpression(mExecutionContext, initialValueExpression, &initialValue);

               assignValue(mExecutionContext, initialValue, &execInstance->mValue, true);
            }
         }
         else
         {
            throwCompileError(pContext, CompileError::InvalidAssignment);
         }
      }
   }
   else
   {
      throwCompileError(pContext, CompileError::VariableRedefinition, pIdentifier.mName);
   }

   return statement;
}

StatementFunctionDeclaration* Environment::parseStatementFunctionDeclaration(ParsingContext& pContext,
   const TypeUsage& pReturnType)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   const Identifier functionIdentifier(pContext.mStringBuffer.c_str());

   StatementFunctionDeclaration* statement =
      (StatementFunctionDeclaration*)CflatMalloc(sizeof(StatementFunctionDeclaration));
   CflatInvokeCtor(StatementFunctionDeclaration, statement)(pReturnType, functionIdentifier);

   tokenIndex++;

   while(tokens[tokenIndex++].mStart[0] != ')')
   {
      // no arguments
      if(tokens[tokenIndex].mStart[0] == ')')
      {
         tokenIndex++;
         break;
      }

      TypeUsage parameterType = parseTypeUsage(pContext, 0u);

      if(!parameterType.mType)
      {
         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         throwCompileError(pContext, Environment::CompileError::UndefinedType,
            pContext.mStringBuffer.c_str());
         return statement;
      }

      statement->mParameterTypes.push_back(parameterType);

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      Identifier parameterIdentifier(pContext.mStringBuffer.c_str());
      statement->mParameterIdentifiers.push_back(parameterIdentifier);
      tokenIndex++;

      pContext.mScopeLevel++;
      registerInstance(pContext, parameterType, parameterIdentifier);
      pContext.mScopeLevel--;
   }

   CflatArgsVector(TypeUsage) parameterTypes;
   toArgsVector(statement->mParameterTypes, parameterTypes);

   Function* function =
      pContext.mNamespaceStack.back()->getFunction(statement->mFunctionIdentifier, parameterTypes);

   if(!function)
   {
      function = pContext.mNamespaceStack.back()->registerFunction(statement->mFunctionIdentifier);
      function->mReturnTypeUsage = statement->mReturnType;
      function->mProgram = pContext.mProgram;
      function->mLine = token.mLine;

      for(size_t i = 0u; i < statement->mParameterTypes.size(); i++)
      {
         function->mParameters.push_back(statement->mParameterTypes[i]);
         function->mParameterIdentifiers.push_back(statement->mParameterIdentifiers[i]);
      }
   }

   statement->mBody = parseStatementBlock(pContext, true, true);

   return statement;
}

StatementStructDeclaration* Environment::parseStatementStructDeclaration(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   const Identifier structIdentifier(pContext.mStringBuffer.c_str());
   tokenIndex++;

   if(tokens[tokenIndex].mStart[0] != '{')
   {
      throwCompileError(pContext, CompileError::Expected, "{");
      return nullptr;
   }

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, '{', '}');

   if(closureTokenIndex == 0u)
   {
      tokenIndex = tokens.size() - 1u;
      throwCompileError(pContext, CompileError::Expected, "}");
      return nullptr;
   }

   tokenIndex++;

   StatementStructDeclaration* statement =
      (StatementStructDeclaration*)CflatMalloc(sizeof(StatementStructDeclaration));
   CflatInvokeCtor(StatementStructDeclaration, statement)();

   Namespace* ns = pContext.mNamespaceStack.back();
   statement->mStruct = ns->registerType<Struct>(structIdentifier);

   size_t structSize = 0u;

   do
   {
      TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

      if(!typeUsage.mType)
      {
         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         throwCompileError(pContext, CompileError::UndefinedType, pContext.mStringBuffer.c_str());
         break;
      }

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      tokenIndex++;

      if(tokens[tokenIndex].mStart[0] != ';')
      {
         throwCompileError(pContext, CompileError::Expected, ";");
         break;
      }

      const Identifier memberIdentifier(pContext.mStringBuffer.c_str());
      Member member(memberIdentifier);
      member.mTypeUsage = typeUsage;
      member.mOffset = (uint16_t)structSize;

      statement->mStruct->mMembers.push_back(member);
      structSize += typeUsage.getSize();
   }
   while(++tokenIndex < closureTokenIndex);

   statement->mStruct->mSize = structSize;

   if(tokens[++tokenIndex].mStart[0] != ';')
   {
      throwCompileError(pContext, CompileError::Expected, ";");
   }

   return statement;
}

StatementIf* Environment::parseStatementIf(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "if");
      return nullptr;
   }

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(conditionClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   tokenIndex++;
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* ifStatement = parseStatement(pContext);

   if(!ifStatement)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   const size_t tokenIndexForElseCheck = tokenIndex + 1u;
   Statement* elseStatement = nullptr;

   if(tokens[tokenIndexForElseCheck].mType == TokenType::Keyword &&
      strncmp(tokens[tokenIndexForElseCheck].mStart, "else", 4u) == 0)
   {
      tokenIndex = tokenIndexForElseCheck + 1u;
      elseStatement = parseStatement(pContext);
   }

   StatementIf* statement = (StatementIf*)CflatMalloc(sizeof(StatementIf));
   CflatInvokeCtor(StatementIf, statement)(condition, ifStatement, elseStatement);

   return statement;
}

StatementSwitch* Environment::parseStatementSwitch(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "switch");
      return nullptr;
   }

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(conditionClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   if(tokens[conditionClosureTokenIndex + 1u].mStart[0] != '{')
   {
      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
      return nullptr;
   }

   tokenIndex++;
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   tokenIndex++;
   const size_t lastSwitchTokenIndex = findClosureTokenIndex(pContext, '{', '}');

   if(lastSwitchTokenIndex == 0u)
   {
      if(condition)
      {
         CflatInvokeDtor(Expression, condition);
         CflatFree(condition);
      }

      throwCompileError(pContext, CompileError::Expected, "}");
      return nullptr;
   }

   StatementSwitch* statement = (StatementSwitch*)CflatMalloc(sizeof(StatementSwitch));
   CflatInvokeCtor(StatementSwitch, statement)(condition);

   StatementSwitch::CaseSection* currentCaseSection = nullptr;

   for(; tokenIndex < lastSwitchTokenIndex; tokenIndex++)
   {
      if(tokens[tokenIndex].mType == TokenType::Keyword)
      {
         if(strncmp(tokens[tokenIndex].mStart, "case", 4u) == 0)
         {
            tokenIndex++;
            const size_t lastCaseTokenIndex =
               findClosureTokenIndex(pContext, 0, ':', lastSwitchTokenIndex);

            StatementSwitch::CaseSection caseSection;
            caseSection.mExpression = parseExpression(pContext, lastCaseTokenIndex - 1u);
            tokenIndex = lastCaseTokenIndex + 1u;

            statement->mCaseSections.push_back(caseSection);
            currentCaseSection = &statement->mCaseSections.back();
         }
         else if(strncmp(tokens[tokenIndex].mStart, "default", 7u) == 0)
         {
            tokenIndex += 2u;

            StatementSwitch::CaseSection caseSection;
            caseSection.mExpression = nullptr;

            statement->mCaseSections.push_back(caseSection);
            currentCaseSection = &statement->mCaseSections.back();
         }
      }

      if(!currentCaseSection)
      {
         if(condition)
         {
            CflatInvokeDtor(Expression, condition);
            CflatFree(condition);
         }

         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
         return nullptr;
      }

      Statement* caseSectionStatement = parseStatement(pContext);

      if(caseSectionStatement)
      {
         currentCaseSection->mStatements.push_back(caseSectionStatement);
      }
   }

   return statement;
}

StatementWhile* Environment::parseStatementWhile(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "while");
      return nullptr;
   }

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(conditionClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   tokenIndex++;
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* loopStatement = parseStatement(pContext);

   StatementWhile* statement = (StatementWhile*)CflatMalloc(sizeof(StatementWhile));
   CflatInvokeCtor(StatementWhile, statement)(condition, loopStatement);

   return statement;
}

StatementDoWhile* Environment::parseStatementDoWhile(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   Statement* loopStatement = parseStatement(pContext);
   tokenIndex++;

   if(strncmp(tokens[tokenIndex].mStart, "while", 5u) != 0)
   {
      if(loopStatement)
      {
         CflatInvokeDtor(Statement, loopStatement);
         CflatFree(loopStatement);
      }

      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   tokenIndex++;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      if(loopStatement)
      {
         CflatInvokeDtor(Statement, loopStatement);
         CflatFree(loopStatement);
      }

      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(conditionClosureTokenIndex == 0u)
   {
      if(loopStatement)
      {
         CflatInvokeDtor(Statement, loopStatement);
         CflatFree(loopStatement);
      }

      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   tokenIndex++;
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   StatementDoWhile* statement = (StatementDoWhile*)CflatMalloc(sizeof(StatementDoWhile));
   CflatInvokeCtor(StatementDoWhile, statement)(condition, loopStatement);

   return statement;
}

Statement* Environment::parseStatementFor(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "for");
      return nullptr;
   }

   incrementScopeLevel(pContext);
   incrementScopeLevel(mExecutionContext);

   tokenIndex++;

   const size_t initializationClosureTokenIndex = findClosureTokenIndex(pContext, 0, ';');
   const size_t variableClosureTokenIndex = findClosureTokenIndex(pContext, 0, ':');

   if(initializationClosureTokenIndex == 0u && variableClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, "';' or variable declaration");
      return nullptr;
   }

   Statement* statement = nullptr;

   if((initializationClosureTokenIndex > 0u && variableClosureTokenIndex == 0u) ||
      (initializationClosureTokenIndex < variableClosureTokenIndex))
   {
      statement = parseStatementForRegular(pContext, initializationClosureTokenIndex);
   }
   else
   {
      statement = parseStatementForRangeBased(pContext, variableClosureTokenIndex);
   }

   decrementScopeLevel(mExecutionContext);
   decrementScopeLevel(pContext);

   return statement;
}

StatementFor* Environment::parseStatementForRegular(ParsingContext& pContext,
   size_t pInitializationClosureTokenIndex)
{
   size_t& tokenIndex = pContext.mTokenIndex;

   Statement* initialization = nullptr;
   
   if(pInitializationClosureTokenIndex > tokenIndex)
   {
      initialization = parseStatement(pContext);
   }

   tokenIndex = pInitializationClosureTokenIndex + 1u;

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

   if(conditionClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ";");
      return nullptr;
   }

   Expression* condition = nullptr;
   
   if(conditionClosureTokenIndex > tokenIndex)
   {
      condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   }

   tokenIndex = conditionClosureTokenIndex + 1u;

   const size_t incrementClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(incrementClosureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   Expression* increment = nullptr;
   
   if(incrementClosureTokenIndex > tokenIndex)
   {
      increment = parseExpression(pContext, incrementClosureTokenIndex - 1u);
   }

   tokenIndex = incrementClosureTokenIndex + 1u;

   Statement* loopStatement = parseStatement(pContext);

   StatementFor* statement = (StatementFor*)CflatMalloc(sizeof(StatementFor));
   CflatInvokeCtor(StatementFor, statement)(initialization, condition, increment, loopStatement);

   return statement;
}

StatementForRangeBased* Environment::parseStatementForRangeBased(ParsingContext& pContext,
   size_t pVariableClosureTokenIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   TypeUsage variableTypeUsage = parseTypeUsage(pContext, pVariableClosureTokenIndex - 1u);

   if(!variableTypeUsage.mType)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   Identifier variableIdentifier;

   if(tokens[tokenIndex].mType == TokenType::Identifier)
   {
      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      variableIdentifier = Identifier(pContext.mStringBuffer.c_str());
   }
   else
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   registerInstance(pContext, variableTypeUsage, variableIdentifier);

   tokenIndex = pVariableClosureTokenIndex + 1u;
   const size_t collectionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(collectionClosureTokenIndex <= tokenIndex)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   Expression* collection = parseExpression(pContext, collectionClosureTokenIndex - 1u);

   if(!collection)
   {
      return nullptr;
   }

   bool validStatement = false;
   const TypeUsage collectionTypeUsage = getTypeUsage(pContext, collection);

   if(collectionTypeUsage.isArray() && !variableTypeUsage.isArray() &&
      collectionTypeUsage.mPointerLevel == variableTypeUsage.mPointerLevel)
   {
      if(variableTypeUsage.mType == mTypeAuto)
      {
         variableTypeUsage.mType = collectionTypeUsage.mType;
      }

      validStatement = collectionTypeUsage.mType == variableTypeUsage.mType;
   }
   else if(collectionTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
   {
      Struct* collectionType = static_cast<Struct*>(collectionTypeUsage.mType);
      Method* beginMethod = findMethod(collectionType, "begin", TypeUsage::kEmptyList);

      if(beginMethod)
      {
         Method* endMethod = findMethod(collectionType, "end", TypeUsage::kEmptyList);

         if(endMethod && beginMethod->mReturnTypeUsage == endMethod->mReturnTypeUsage)
         {
            const TypeUsage iteratorTypeUsage = beginMethod->mReturnTypeUsage;
            Type* iteratorType = iteratorTypeUsage.mType;

            if(iteratorType->mCategory == TypeCategory::StructOrClass)
            {
               Method* indirectionOperatorMethod =
                  findMethod(iteratorType, "operator*", TypeUsage::kEmptyList);

               if(indirectionOperatorMethod)
               {
                  CflatArgsVector(TypeUsage) nonEqualOperatorParameterTypes;
                  nonEqualOperatorParameterTypes.push_back(iteratorTypeUsage);

                  validStatement = 
                     findMethod(iteratorType, "operator!=", nonEqualOperatorParameterTypes) &&
                     findMethod(iteratorType, "operator++", TypeUsage::kEmptyList);

                  if(validStatement && variableTypeUsage.mType == mTypeAuto)
                  {
                     variableTypeUsage.mType = indirectionOperatorMethod->mReturnTypeUsage.mType;
                  }
               }
            }
            else
            {
               if(variableTypeUsage.mType == mTypeAuto)
               {
                  variableTypeUsage.mType = iteratorTypeUsage.mType;
               }

               validStatement = iteratorTypeUsage.isPointer();
            }
         }
      }
   }

   if(!validStatement)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   tokenIndex = collectionClosureTokenIndex + 1u;
   Statement* loopStatement = parseStatement(pContext);

   StatementForRangeBased* statement =
      (StatementForRangeBased*)CflatMalloc(sizeof(StatementForRangeBased));
   CflatInvokeCtor(StatementForRangeBased, statement)
      (variableTypeUsage, variableIdentifier, collection, loopStatement);

   return statement;
}

StatementBreak* Environment::parseStatementBreak(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != ';')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "break");
      return nullptr;
   }

   StatementBreak* statement = (StatementBreak*)CflatMalloc(sizeof(StatementBreak));
   CflatInvokeCtor(StatementBreak, statement)();

   return statement;
}

StatementContinue* Environment::parseStatementContinue(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != ';')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "continue");
      return nullptr;
   }

   StatementContinue* statement = (StatementContinue*)CflatMalloc(sizeof(StatementContinue));
   CflatInvokeCtor(StatementContinue, statement)();

   return statement;
}

StatementReturn* Environment::parseStatementReturn(ParsingContext& pContext)
{
   if(pContext.mScopeLevel == 0u)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

   if(closureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return nullptr;
   }

   Expression* expression = parseExpression(pContext, closureTokenIndex - 1u, true);

   StatementReturn* statement = (StatementReturn*)CflatMalloc(sizeof(StatementReturn));
   CflatInvokeCtor(StatementReturn, statement)(expression);

   pContext.mTokenIndex = closureTokenIndex;

   return statement;
}

bool Environment::parseFunctionCallArguments(ParsingContext& pContext,
   CflatSTLVector(Expression*)* pArguments, CflatSTLVector(TypeUsage)* pTemplateTypes)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] == '<')
   {
      if(pTemplateTypes)
      {
         const size_t closureTokenIndex = findClosureTokenIndex(pContext, '<', '>');

         if(closureTokenIndex > 0u)
         {
            while(tokenIndex++ < closureTokenIndex)
            {
               TypeUsage typeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

               if(typeUsage.mType)
               {
                  pTemplateTypes->push_back(typeUsage);
               }
               else
               {
                  pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
                  throwCompileError(pContext, Environment::CompileError::UndefinedType,
                     pContext.mStringBuffer.c_str());
                  return false;
               }
            }
         }
         else
         {
            throwCompileError(pContext, CompileError::Expected, ">");
            return false;
         }
      }
      else
      {
         throwCompileError(pContext, CompileError::Expected, "(");
         return false;
      }
   }

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(closureTokenIndex > 0u)
   {
      while(tokenIndex++ < closureTokenIndex)
      {
         const size_t separatorTokenIndex =
            findSeparationTokenIndex(pContext, ',', closureTokenIndex);
         const size_t tokenLastIndex = 
           separatorTokenIndex > 0u ? separatorTokenIndex : closureTokenIndex;

         Expression* argument = parseExpression(pContext, tokenLastIndex - 1u, true);

         if(argument)
         {
            pArguments->push_back(argument);
         }

         tokenIndex = tokenLastIndex;
      }
   }
   else
   {
      throwCompileError(pContext, CompileError::Expected, ")");
      return false;
   }

   return true;
}

TypeUsage Environment::getTypeUsage(Context& pContext, Expression* pExpression)
{
   TypeUsage typeUsage;

   if(pExpression && mErrorMessage.empty())
   {
      switch(pExpression->getType())
      {
      case ExpressionType::Value:
         {
            ExpressionValue* expression = static_cast<ExpressionValue*>(pExpression);
            typeUsage = expression->mValue.mTypeUsage;
         }
         break;
      case ExpressionType::NullPointer:
         {
            typeUsage = mTypeUsageVoidPtr;
         }
         break;
      case ExpressionType::VariableAccess:
         {
            ExpressionVariableAccess* expression = static_cast<ExpressionVariableAccess*>(pExpression);
            Instance* instance = retrieveInstance(pContext, expression->mVariableIdentifier);
            typeUsage = instance->mTypeUsage;
         }
         break;
      case ExpressionType::MemberAccess:
         {
            ExpressionMemberAccess* expression = static_cast<ExpressionMemberAccess*>(pExpression);
            typeUsage = expression->mMemberTypeUsage;
         }
         break;
      case ExpressionType::ArrayElementAccess:
         {
            ExpressionArrayElementAccess* expression =
               static_cast<ExpressionArrayElementAccess*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mArray);
            CflatAssert(typeUsage.isArray() || typeUsage.isPointer());

            if(typeUsage.isArray())
            {
               CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Array);
               typeUsage.mArraySize = 1u;
            }
            else
            {
               typeUsage.mPointerLevel--;
            }
         }
         break;
      case ExpressionType::UnaryOperation:
         {
            ExpressionUnaryOperation* expression = static_cast<ExpressionUnaryOperation*>(pExpression);
            typeUsage = expression->mOperator[0] == '!'
               ? mTypeUsageBool
               : getTypeUsage(pContext, expression->mExpression);
            CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
         }
         break;
      case ExpressionType::BinaryOperation:
         {
            ExpressionBinaryOperation* expression = static_cast<ExpressionBinaryOperation*>(pExpression);

            if(expression->mOverloadedOperatorTypeUsage.mType)
            {
               typeUsage = expression->mOverloadedOperatorTypeUsage;
            }
            else
            {
               bool logicalOperator = false;

               for(size_t i = 0u; i < kCflatLogicalOperatorsCount; i++)
               {
                  if(strcmp(expression->mOperator, kCflatLogicalOperators[i]) == 0)
                  {
                     logicalOperator = true;
                     break;
                  }
               }

               if(logicalOperator)
               {
                  typeUsage = mTypeUsageBool;
               }
               else
               {
                  const TypeUsage leftTypeUsage = getTypeUsage(pContext, expression->mLeft);
                  const TypeUsage rightTypeUsage = getTypeUsage(pContext, expression->mRight);

                  if(leftTypeUsage.mType->isInteger() && !rightTypeUsage.mType->isInteger())
                  {
                     typeUsage = rightTypeUsage;
                  }
                  else
                  {
                     typeUsage = leftTypeUsage;
                  }

                  CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
               }
            }
         }
         break;
      case ExpressionType::Parenthesized:
         {
            ExpressionParenthesized* expression = static_cast<ExpressionParenthesized*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mExpression);
         }
         break;
      case ExpressionType::AddressOf:
         {
            ExpressionAddressOf* expression = static_cast<ExpressionAddressOf*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mExpression);
            typeUsage.mPointerLevel++;
         }
         break;
      case ExpressionType::Indirection:
         {
            ExpressionIndirection* expression = static_cast<ExpressionIndirection*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mExpression);
            typeUsage.mPointerLevel--;
         }
         break;
      case ExpressionType::SizeOf:
         {
            typeUsage = mTypeUsageSizeT;
         }
         break;
      case ExpressionType::Cast:
         {
            ExpressionCast* expression = static_cast<ExpressionCast*>(pExpression);
            typeUsage = expression->mTypeUsage;
         }
         break;
      case ExpressionType::Conditional:
         {
            ExpressionConditional* expression = static_cast<ExpressionConditional*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mIfExpression);
         }
         break;
      case ExpressionType::Assignment:
         {
            ExpressionAssignment* expression = static_cast<ExpressionAssignment*>(pExpression);
            typeUsage = getTypeUsage(pContext, expression->mRightValue);
         }
         break;
      case ExpressionType::FunctionCall:
         {
            ExpressionFunctionCall* expression = static_cast<ExpressionFunctionCall*>(pExpression);
            typeUsage = expression->mFunction->mReturnTypeUsage;
         }
         break;
      case ExpressionType::MethodCall:
         {
            ExpressionMethodCall* expression = static_cast<ExpressionMethodCall*>(pExpression);
            typeUsage = expression->mMethod->mReturnTypeUsage;
         }
         break;
      case ExpressionType::ArrayInitialization:
         {
            ExpressionArrayInitialization* expression =
               static_cast<ExpressionArrayInitialization*>(pExpression);
            typeUsage.mType = expression->mElementType;
            CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Array);
            typeUsage.mArraySize = (uint16_t)expression->mValues.size();
         }
         break;
      case ExpressionType::ObjectConstruction:
         {
            ExpressionObjectConstruction* expression =
              static_cast<ExpressionObjectConstruction*>(pExpression);
            typeUsage.mType = expression->mObjectType;
         }
         break;
      default:
         CflatAssert(false);
         break;
      }
   }

   return typeUsage;
}

Type* Environment::findType(const Context& pContext, const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   Type* type = nullptr;

   for(size_t i = 0u; i < pContext.mTypeAliases.size(); i++)
   {
      if(pContext.mTypeAliases[i].mIdentifier == pIdentifier)
      {
         const TypeUsage& typeUsage = pContext.mTypeAliases[i].mTypeUsage;

         if(typeUsage.mFlags == 0u)
         {
            type = typeUsage.mType;
            break;
         }
      }
   }

   if(!type)
   {
      type = pContext.mNamespaceStack.back()->getType(pIdentifier, pTemplateTypes, true);
   }

   if(!type)
   {
      for(size_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
      {
         type = pContext.mUsingDirectives[i].mNamespace->getType(pIdentifier, pTemplateTypes);

         if(type)
         {
            break;
         }
      }
   }

   return type;
}

Function* Environment::findFunction(const Context& pContext, const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes,
   const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   Namespace* ns = pContext.mNamespaceStack.back();
   Function* function = ns->getFunction(pIdentifier, pParameterTypes, pTemplateTypes, true);

   if(!function)
   {
      for(size_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
      {
         Namespace* usingNS = pContext.mUsingDirectives[i].mNamespace;
         function =
            usingNS->getFunction(pIdentifier, pParameterTypes, pTemplateTypes, true);

         if(function)
         {
            break;
         }
      }
   }

   return function;
}

Function* Environment::findFunction(const Context& pContext, const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments,
   const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   CflatArgsVector(TypeUsage) typeUsages;

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return findFunction(pContext, pIdentifier, typeUsages, pTemplateTypes);
}

void Environment::registerTypeAlias(
  Context& pContext, const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   if(pContext.mScopeLevel > 0u)
   {
      TypeAlias typeAlias(pIdentifier, pTypeUsage);
      typeAlias.mScopeLevel = pContext.mScopeLevel;
      pContext.mTypeAliases.push_back(typeAlias);
   }
   else
   {
      pContext.mNamespaceStack.back()->registerTypeAlias(pIdentifier, pTypeUsage);
   }
}

Instance* Environment::registerInstance(Context& pContext,
   const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   Instance* instance = nullptr;
   bool initializationRequired = false;

   if(pContext.mScopeLevel > 0u)
   {
      instance = pContext.mLocalInstancesHolder.registerInstance(pTypeUsage, pIdentifier);
      initializationRequired = true;
   }
   else
   {
      instance = pContext.mNamespaceStack.back()->retrieveInstance(pIdentifier);

      if(!instance)
      {
         instance = pContext.mNamespaceStack.back()->registerInstance(pTypeUsage, pIdentifier);
         initializationRequired = true;
      }
      else if(instance->mTypeUsage != pTypeUsage)
      {
         instance->mTypeUsage = pTypeUsage;
         initializationRequired = true;
      }
   }

   CflatAssert(instance);

   if(initializationRequired)
   {
      if(instance->mTypeUsage.isReference())
      {
         instance->mValue.initExternal(instance->mTypeUsage);
      }
      else if(pContext.mScopeLevel == 0u)
      {
         instance->mValue.initOnHeap(instance->mTypeUsage);
      }
      else
      {
         instance->mValue.initOnStack(instance->mTypeUsage, &pContext.mStack);
      }
   }

   CflatAssert(instance->mTypeUsage == pTypeUsage);

   instance->mScopeLevel = pContext.mScopeLevel;

   return instance;
}

Instance* Environment::registerStaticInstance(Context& pContext, const TypeUsage& pTypeUsage,
   const Identifier& pIdentifier, void* pUniquePtr)
{
   Instance* instance = pContext.mNamespaceStack.back()->registerInstance(pTypeUsage, pIdentifier);
   instance->mScopeLevel = pContext.mScopeLevel;

   Value* staticValue = nullptr;

   const uint64_t uniqueID = (uint64_t)pUniquePtr;
   StaticValuesRegistry::const_iterator it = mStaticValues.find(uniqueID);

   if(it == mStaticValues.end())
   {
      mStaticValues[uniqueID] = Value();
      staticValue = &mStaticValues[uniqueID];
      staticValue->initOnHeap(pTypeUsage);
   }

   instance->mValue = mStaticValues[uniqueID];

   return instance;
}

Instance* Environment::retrieveInstance(Context& pContext, const Identifier& pIdentifier)
{
   Instance* instance = pContext.mLocalInstancesHolder.retrieveInstance(pIdentifier);

   if(!instance)
   {
      instance = pContext.mNamespaceStack.back()->retrieveInstance(pIdentifier, true);

      if(!instance)
      {
         for(size_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
         {
            instance = pContext.mUsingDirectives[i].mNamespace->retrieveInstance(pIdentifier, true);

            if(instance)
            {
               break;
            }
         }
      }

      if(!instance)
      {
         const char* lastSeparator = pIdentifier.findLastSeparator();

         if(lastSeparator)
         {
            char buffer[kDefaultLocalStringBufferSize];
            const size_t typeIdentifierLength = lastSeparator - pIdentifier.mName;
            strncpy(buffer, pIdentifier.mName, typeIdentifierLength);
            buffer[typeIdentifierLength] = '\0';
            const Identifier typeIdentifier(buffer);
            const Identifier staticMemberIdentifier(lastSeparator + 2);

            Type* type = findType(pContext, typeIdentifier);

            if(type && type->mCategory == TypeCategory::StructOrClass)
            {
               instance = static_cast<Struct*>(type)->getStaticMemberInstance(staticMemberIdentifier);
            }
         }
      }
   }

   return instance;
}

void Environment::incrementBlockLevel(Context& pContext)
{
   pContext.mBlockLevel++;
}

void Environment::decrementBlockLevel(Context& pContext)
{
   while(!pContext.mUsingDirectives.empty() &&
      pContext.mUsingDirectives.back().mBlockLevel >= pContext.mBlockLevel)
   {
      pContext.mUsingDirectives.pop_back();
   }

   pContext.mBlockLevel--;
}

void Environment::incrementScopeLevel(Context& pContext)
{
   pContext.mScopeLevel++;
}

void Environment::decrementScopeLevel(Context& pContext)
{
   const bool isExecutionContext = pContext.mType == ContextType::Execution;

   if(!isExecutionContext)
   {
      ParsingContext& parsingContext = static_cast<ParsingContext&>(pContext);

      while(!parsingContext.mRegisteredInstances.empty() &&
         parsingContext.mRegisteredInstances.back().mScopeLevel >= pContext.mScopeLevel)
      {
         parsingContext.mRegisteredInstances.pop_back();
      }
   }

   while(!pContext.mTypeAliases.empty() &&
      pContext.mTypeAliases.back().mScopeLevel >= pContext.mScopeLevel)
   {
      pContext.mTypeAliases.pop_back();
   }

   pContext.mLocalInstancesHolder.releaseInstances(pContext.mScopeLevel, isExecutionContext);
   mGlobalNamespace.releaseInstances(pContext.mScopeLevel, isExecutionContext);

   pContext.mScopeLevel--;
}

void Environment::throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg)
{
   if(!mErrorMessage.empty())
      return;

   char errorMsg[kDefaultLocalStringBufferSize];
   sprintf(errorMsg, kRuntimeErrorStrings[(int)pError], pArg);

   char lineAsString[16];
   sprintf(lineAsString, "%d", pContext.mCallStack.back().mLine);

   mErrorMessage.assign("[Runtime Error] '");
   mErrorMessage.append(pContext.mProgram->mIdentifier.mName);
   mErrorMessage.append("' -- Line ");
   mErrorMessage.append(lineAsString);
   mErrorMessage.append(": ");
   mErrorMessage.append(errorMsg);
}

void Environment::evaluateExpression(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue)
{
   if(!mErrorMessage.empty())
      return;

   switch(pExpression->getType())
   {
   case ExpressionType::Value:
      {
         ExpressionValue* expression = static_cast<ExpressionValue*>(pExpression);
         *pOutValue = expression->mValue;
      }
      break;
   case ExpressionType::NullPointer:
      {
         assertValueInitialization(pContext, mTypeUsageVoidPtr, pOutValue);
         const void* nullPointer = nullptr;
         pOutValue->set(&nullPointer);
      }
      break;
   case ExpressionType::VariableAccess:
      {
         ExpressionVariableAccess* expression = static_cast<ExpressionVariableAccess*>(pExpression);
         Instance* instance = retrieveInstance(pContext, expression->mVariableIdentifier);

         if(pOutValue->mTypeUsage.isPointer() && instance->mTypeUsage.isArray())
         {
            getAddressOfValue(pContext, instance->mValue, pOutValue);
         }
         else
         {
            *pOutValue = instance->mValue;
         }
      }
      break;
   case ExpressionType::MemberAccess:
      {
         ExpressionMemberAccess* expression = static_cast<ExpressionMemberAccess*>(pExpression);
         getInstanceDataValue(pContext, expression, pOutValue);
      }
      break;
   case ExpressionType::ArrayElementAccess:
      {
         ExpressionArrayElementAccess* expression =
            static_cast<ExpressionArrayElementAccess*>(pExpression);

         TypeUsage arrayTypeUsage = getTypeUsage(pContext, expression->mArray);
         CflatAssert(arrayTypeUsage.isArray() || arrayTypeUsage.isPointer());

         TypeUsage arrayElementTypeUsage = arrayTypeUsage;

         if(arrayElementTypeUsage.isArray())
         {
            CflatResetFlag(arrayElementTypeUsage.mFlags, TypeUsageFlags::Array);
            arrayElementTypeUsage.mArraySize = 1u;
         }
         else
         {
            arrayElementTypeUsage.mPointerLevel--;
         }
         
         assertValueInitialization(pContext, arrayElementTypeUsage, pOutValue);

         Value arrayValue;
         arrayValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mArray, &arrayValue);

         Value indexValue;
         indexValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mArrayElementIndex, &indexValue);
         const size_t index = (size_t)getValueAsInteger(indexValue);
         
         if(arrayValue.mTypeUsage.isArray())
         {
            const size_t arraySize = (size_t)arrayTypeUsage.mArraySize;

            if(index < arraySize)
            {
               const size_t arrayElementSize = arrayElementTypeUsage.mType->mSize;
               const size_t offset = arrayElementSize * index;
               pOutValue->set(arrayValue.mValueBuffer + offset);
            }
            else
            {
               char buffer[kDefaultLocalStringBufferSize];
               sprintf(buffer, "size %zu, index %zu", arraySize, index);
               throwRuntimeError(pContext, RuntimeError::InvalidArrayIndex, buffer);
            }
         }
         else
         {
            const size_t arrayElementTypeSize = arrayElementTypeUsage.mType->mSize;
            const size_t ptrOffset = arrayElementTypeSize * index;
            const char* ptr = CflatValueAs(&arrayValue, char*) + ptrOffset;
            memcpy(pOutValue->mValueBuffer, ptr, arrayElementTypeSize);
         }
      }
      break;
   case ExpressionType::UnaryOperation:
      {
         ExpressionUnaryOperation* expression = static_cast<ExpressionUnaryOperation*>(pExpression);

         const TypeUsage typeUsage = getTypeUsage(pContext, expression->mExpression);
         assertValueInitialization(pContext, typeUsage, pOutValue);

         Value preValue;
         preValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mExpression, &preValue);

         pOutValue->set(preValue.mValueBuffer);

         const bool isIncrementOrDecrement =
            strncmp(expression->mOperator, "++", 2u) == 0 ||
            strncmp(expression->mOperator, "--", 2u) == 0;

         if(isIncrementOrDecrement)
         {
            applyUnaryOperator(pContext, preValue, expression->mOperator, &preValue);

            if(!expression->mPostOperator)
            {
               pOutValue->set(preValue.mValueBuffer);
            }
         }
         else
         {
            applyUnaryOperator(pContext, preValue, expression->mOperator, pOutValue);
         }
      }
      break;
   case ExpressionType::BinaryOperation:
      {
         ExpressionBinaryOperation* expression = static_cast<ExpressionBinaryOperation*>(pExpression);

         const TypeUsage typeUsage = getTypeUsage(pContext, expression);
         assertValueInitialization(pContext, typeUsage, pOutValue);

         Value leftValue;
         leftValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mLeft, &leftValue);

         Value rightValue;
         bool evaluateRightValue = true;

         if(strcmp(expression->mOperator, "&&") == 0)
         {
            if(!getValueAsInteger(leftValue))
            {
               const bool value = false;
               rightValue.initOnStack(mTypeUsageBool, &pContext.mStack);
               rightValue.set(&value);
               evaluateRightValue = false;
            }
         }
         else if(strcmp(expression->mOperator, "||") == 0)
         {
            if(getValueAsInteger(leftValue))
            {
               const bool value = true;
               rightValue.initOnStack(mTypeUsageBool, &pContext.mStack);
               rightValue.set(&value);
               evaluateRightValue = false;
            }
         }

         if(evaluateRightValue)
         {
            rightValue.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, expression->mRight, &rightValue);
         }

         applyBinaryOperator(pContext, leftValue, rightValue, expression->mOperator, pOutValue);
      }
      break;
   case ExpressionType::Parenthesized:
      {
         ExpressionParenthesized* expression = static_cast<ExpressionParenthesized*>(pExpression);
         evaluateExpression(pContext, expression->mExpression, pOutValue);
      }
      break;
   case ExpressionType::AddressOf:
      {
         ExpressionAddressOf* expression = static_cast<ExpressionAddressOf*>(pExpression);

         Value value;
         value.initExternal(getTypeUsage(pContext, expression->mExpression));
         evaluateExpression(pContext, expression->mExpression, &value);

         pOutValue->mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, value, pOutValue);
      }
      break;
   case ExpressionType::Indirection:
      {
         ExpressionIndirection* expression = static_cast<ExpressionIndirection*>(pExpression);

         const TypeUsage expressionTypeUsage = getTypeUsage(pContext, expression->mExpression);
         CflatAssert(expressionTypeUsage.mPointerLevel > 0u);

         TypeUsage typeUsage = expressionTypeUsage;
         typeUsage.mPointerLevel--;
         pOutValue->mValueInitializationHint = ValueInitializationHint::Stack;
         assertValueInitialization(pContext, typeUsage, pOutValue);

         Value value;
         value.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mExpression, &value);

         applyUnaryOperator(pContext, value, "*", pOutValue);
      }
      break;
   case ExpressionType::SizeOf:
      {
         ExpressionSizeOf* expression = static_cast<ExpressionSizeOf*>(pExpression);
         size_t size = 0u;
         
         if(expression->mTypeUsage.mType)
         {
            size = expression->mTypeUsage.getSize();
         }
         else if(expression->mExpression)
         {
            Value value;
            value.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, expression->mExpression, &value);
            size = value.mTypeUsage.getSize();
         }

         assertValueInitialization(pContext, mTypeUsageSizeT, pOutValue);
         pOutValue->set(&size);
      }
      break;
   case ExpressionType::Cast:
      {
         ExpressionCast* expression = static_cast<ExpressionCast*>(pExpression);

         assertValueInitialization(pContext, expression->mTypeUsage, pOutValue);

         Value valueToCast;
         valueToCast.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mExpression, &valueToCast);

         const TypeUsage& targetTypeUsage = expression->mTypeUsage;

         if(expression->mCastType == CastType::CStyle || expression->mCastType == CastType::Static)
         {
            performStaticCast(pContext, valueToCast, targetTypeUsage, pOutValue);
         }
         else if(expression->mCastType == CastType::Dynamic)
         {
            performInheritanceCast(pContext, valueToCast, targetTypeUsage, pOutValue);
         }
         else if(expression->mCastType == CastType::Reinterpret)
         {
            const void* ptr = CflatValueAs(&valueToCast, void*);
            pOutValue->set(&ptr);
         }
      }
      break;
   case ExpressionType::Conditional:
      {
         ExpressionConditional* expression = static_cast<ExpressionConditional*>(pExpression);

         bool conditionMet = false;
         {
            Value conditionValue;
            conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, expression->mCondition, &conditionValue);
            conditionMet = getValueAsInteger(conditionValue) != 0;
         }

         Expression* valueSource = conditionMet
            ? expression->mIfExpression
            : expression->mElseExpression;
         evaluateExpression(pContext, valueSource, pOutValue);
      }
      break;
   case ExpressionType::Assignment:
      {
         ExpressionAssignment* expression = static_cast<ExpressionAssignment*>(pExpression);

         const TypeUsage expressionTypeUsage = getTypeUsage(pContext, expression->mRightValue);
         Value expressionValue;
         expressionValue.initOnStack(expressionTypeUsage, &pContext.mStack);
         evaluateExpression(pContext, expression->mRightValue, &expressionValue);

         Value instanceDataValue;
         getInstanceDataValue(pContext, expression->mLeftValue, &instanceDataValue);

         performAssignment(pContext, expressionValue, expression->mOperator, &instanceDataValue);
         *pOutValue = instanceDataValue;
      }
      break;
   case ExpressionType::FunctionCall:
      {
         ExpressionFunctionCall* expression = static_cast<ExpressionFunctionCall*>(pExpression);

         Function* function = expression->mFunction;
         CflatAssert(function);

         assertValueInitialization(pContext, function->mReturnTypeUsage, pOutValue);

         CflatArgsVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         CflatArgsVector(Value) preparedArgumentValues;
         prepareArgumentsForFunctionCall(pContext, function->mParameters, argumentValues,
            preparedArgumentValues);

         const bool functionReturnValueIsConst =
            CflatHasFlag(function->mReturnTypeUsage.mFlags, TypeUsageFlags::Const);
         const bool outValueIsConst =
            CflatHasFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);

         if(outValueIsConst && !functionReturnValueIsConst)
         {
            CflatResetFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);
         }

         function->execute(preparedArgumentValues, pOutValue);

         if(outValueIsConst && !functionReturnValueIsConst)
         {
            CflatSetFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);
         }

         while(!preparedArgumentValues.empty())
         {
            preparedArgumentValues.pop_back();
         }

         while(!argumentValues.empty())
         {
            argumentValues.pop_back();
         }
      }
      break;
   case ExpressionType::MethodCall:
      {
         ExpressionMethodCall* expression = static_cast<ExpressionMethodCall*>(pExpression);
         ExpressionMemberAccess* memberAccess =
            static_cast<ExpressionMemberAccess*>(expression->mMemberAccess);

         Method* method = expression->mMethod;
         CflatAssert(method);

         assertValueInitialization(pContext, method->mReturnTypeUsage, pOutValue);

         Value instanceDataValue;
         getInstanceDataValue(pContext, memberAccess->mMemberOwner, &instanceDataValue);

         if(instanceDataValue.mTypeUsage.isPointer() && !CflatValueAs(&instanceDataValue, void*))
         {
            throwRuntimeError(pContext, RuntimeError::NullPointerAccess,
               memberAccess->mMemberIdentifier.mName);
         }

         if(!mErrorMessage.empty())
            break;

         CflatArgsVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         CflatArgsVector(Value) preparedArgumentValues;
         prepareArgumentsForFunctionCall(pContext, method->mParameters, argumentValues,
            preparedArgumentValues);

         Value thisPtr;

         if(instanceDataValue.mTypeUsage.isPointer())
         {
            thisPtr.initOnStack(instanceDataValue.mTypeUsage, &pContext.mStack);
            thisPtr.set(instanceDataValue.mValueBuffer);
         }
         else
         {
            thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, instanceDataValue, &thisPtr);
         }

         method->execute(thisPtr, preparedArgumentValues, pOutValue);

         thisPtr.reset();

         while(!preparedArgumentValues.empty())
         {
            preparedArgumentValues.pop_back();
         }

         while(!argumentValues.empty())
         {
            argumentValues.pop_back();
         }
      }
      break;
   case ExpressionType::ArrayInitialization:
      {
         ExpressionArrayInitialization* expression =
            static_cast<ExpressionArrayInitialization*>(pExpression);

         TypeUsage arrayElementTypeUsage;
         arrayElementTypeUsage.mType = expression->mElementType;

         TypeUsage arrayTypeUsage;
         arrayTypeUsage.mType = expression->mElementType;
         CflatSetFlag(arrayTypeUsage.mFlags, TypeUsageFlags::Array);
         arrayTypeUsage.mArraySize = (uint16_t)expression->mValues.size();

         assertValueInitialization(pContext, arrayTypeUsage, pOutValue);

         for(size_t i = 0u; i < expression->mValues.size(); i++)
         {
            Value arrayElementValue;
            arrayElementValue.initOnStack(arrayElementTypeUsage, &pContext.mStack);
            evaluateExpression(pContext, expression->mValues[i], &arrayElementValue);

            const size_t arrayElementSize = expression->mElementType->mSize;
            const size_t offset = i * arrayElementSize;
            memcpy(pOutValue->mValueBuffer + offset, arrayElementValue.mValueBuffer, arrayElementSize);
         }
      }
      break;
   case ExpressionType::ObjectConstruction:
      {
         ExpressionObjectConstruction* expression =
            static_cast<ExpressionObjectConstruction*>(pExpression);

         Method* ctor = expression->mConstructor;
         CflatAssert(ctor);

         TypeUsage typeUsage;
         typeUsage.mType = expression->mObjectType;
         assertValueInitialization(pContext, typeUsage, pOutValue);

         CflatArgsVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         Value thisPtr;
         thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, *pOutValue, &thisPtr);

         ctor->execute(thisPtr, argumentValues, nullptr);

         thisPtr.reset();

         while(!argumentValues.empty())
         {
            argumentValues.pop_back();
         }
      }
      break;
   default:
      break;
   }
}

void Environment::getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression,
   Value* pOutValue)
{
   if(pExpression->getType() == ExpressionType::VariableAccess)
   {
      ExpressionVariableAccess* variableAccess =
         static_cast<ExpressionVariableAccess*>(pExpression);
      Instance* instance = retrieveInstance(pContext, variableAccess->mVariableIdentifier);
      *pOutValue = instance->mValue;
   }
   else if(pExpression->getType() == ExpressionType::MemberAccess)
   {
      ExpressionMemberAccess* memberAccess =
         static_cast<ExpressionMemberAccess*>(pExpression);

      Member* member = nullptr;
      char* instanceDataPtr = nullptr;

      {
         Value memberOwner;
         evaluateExpression(pContext, memberAccess->mMemberOwner, &memberOwner);

         if(memberOwner.mTypeUsage.isPointer() && !CflatValueAs(&memberOwner, void*))
         {
            throwRuntimeError(pContext, RuntimeError::NullPointerAccess,
               memberAccess->mMemberIdentifier.mName);
         }

         if(!mErrorMessage.empty())
            return;

         Struct* type = static_cast<Struct*>(memberOwner.mTypeUsage.mType);

         for(size_t j = 0u; j < type->mMembers.size(); j++)
         {
            if(type->mMembers[j].mIdentifier == memberAccess->mMemberIdentifier)
            {
               member = &type->mMembers[j];
               instanceDataPtr = memberOwner.mTypeUsage.isPointer()
                  ? CflatValueAs(&memberOwner, char*)
                  : memberOwner.mValueBuffer;
               break;
            }
         }
      }

      if(member && instanceDataPtr)
      {
         if(pOutValue->mValueBufferType != ValueBufferType::Uninitialized)
         {
            pOutValue->reset();
         }

         pOutValue->initExternal(member->mTypeUsage);
         pOutValue->set(instanceDataPtr + member->mOffset);
      }
   }
   else if(pExpression->getType() == ExpressionType::ArrayElementAccess)
   {
      ExpressionArrayElementAccess* arrayElementAccess =
         static_cast<ExpressionArrayElementAccess*>(pExpression);

      Value arrayDataValue;
      getInstanceDataValue(pContext, arrayElementAccess->mArray, &arrayDataValue);

      Value arrayIndexValue;
      arrayIndexValue.mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, arrayElementAccess->mArrayElementIndex, &arrayIndexValue);
      const size_t arrayIndex = (size_t)getValueAsInteger(arrayIndexValue);

      TypeUsage arrayElementTypeUsage;
      arrayElementTypeUsage.mType = arrayDataValue.mTypeUsage.mType;

      pOutValue->initExternal(arrayElementTypeUsage);
      pOutValue->mValueBuffer =
         arrayDataValue.mValueBuffer + (arrayIndex * arrayElementTypeUsage.getSize());
   }
   else if(pExpression->getType() == ExpressionType::Indirection)
   {
      ExpressionIndirection* indirection = static_cast<ExpressionIndirection*>(pExpression);

      Value value;
      value.mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, indirection->mExpression, &value);
      CflatAssert(value.mTypeUsage.isPointer());

      TypeUsage typeUsage = value.mTypeUsage;
      typeUsage.mPointerLevel--;

      const void* ptr = CflatValueAs(&value, void*);
      pOutValue->initExternal(typeUsage);
      pOutValue->set(ptr);
   }
   else
   {
      Value expressionValue;
      expressionValue.mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, pExpression, &expressionValue);

      assertValueInitialization(pContext, expressionValue.mTypeUsage, pOutValue);
      *pOutValue = expressionValue;
   }
}

void Environment::getAddressOfValue(ExecutionContext& pContext, const Value& pInstanceDataValue,
   Value* pOutValue)
{
   TypeUsage pointerTypeUsage = pInstanceDataValue.mTypeUsage;
   pointerTypeUsage.mPointerLevel++;

   assertValueInitialization(pContext, pointerTypeUsage, pOutValue);
   pOutValue->set(&pInstanceDataValue.mValueBuffer);
}

void Environment::getArgumentValues(ExecutionContext& pContext,
   const CflatSTLVector(Expression*)& pExpressions, CflatArgsVector(Value)& pValues)
{
   pValues.resize(pExpressions.size());

   for(size_t i = 0u; i < pExpressions.size(); i++)
   {
      pValues[i].mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, pExpressions[i], &pValues[i]);
   }
}

void Environment::prepareArgumentsForFunctionCall(ExecutionContext& pContext,
   const CflatSTLVector(TypeUsage)& pParameters, const CflatArgsVector(Value)& pOriginalValues,
   CflatArgsVector(Value)& pPreparedValues)
{
   CflatAssert(pParameters.size() == pOriginalValues.size());
   pPreparedValues.resize(pParameters.size());

   for(size_t i = 0u; i < pParameters.size(); i++)
   {
      // pass by reference
      if(pParameters[i].isReference())
      {
         pPreparedValues[i] = pOriginalValues[i];
         CflatSetFlag(pPreparedValues[i].mTypeUsage.mFlags, TypeUsageFlags::Reference);
      }
      // pass by value
      else
      {
         pPreparedValues[i].initOnStack(pParameters[i], &pContext.mStack);
         assignValue(pContext, pOriginalValues[i], &pPreparedValues[i], false);
      }
   }
}

void Environment::applyUnaryOperator(ExecutionContext& pContext, const Value& pOperand,
   const char* pOperator, Value* pOutValue)
{
   Type* type = pOperand.mTypeUsage.mType;

   // integer built-in / pointer
   if(type->isInteger() || pOperand.mTypeUsage.isPointer())
   {
      if(pOperator[0] == '*')
      {
         CflatAssert(pOperand.mTypeUsage.isPointer());
         CflatAssert(pOperand.mTypeUsage.mType == pOutValue->mTypeUsage.mType);
         CflatAssert(pOperand.mTypeUsage.mPointerLevel == (pOutValue->mTypeUsage.mPointerLevel + 1u));

         const void* ptr = CflatValueAs(&pOperand, void*);
         pOutValue->set(ptr);
      }
      else
      {
         const int64_t valueAsInteger = getValueAsInteger(pOperand);

         if(pOperator[0] == '!')
         {
            setValueAsInteger(!valueAsInteger, pOutValue);
         }
         else if(strcmp(pOperator, "++") == 0)
         {
            const int64_t increment = pOutValue->mTypeUsage.isPointer()
               ? (int64_t)pOutValue->mTypeUsage.mType->mSize
               : 1;
            setValueAsInteger(valueAsInteger + increment, pOutValue);
         }
         else if(strcmp(pOperator, "--") == 0)
         {
            const int64_t decrement = pOutValue->mTypeUsage.isPointer()
               ? (int64_t)pOutValue->mTypeUsage.mType->mSize
               : 1;
            setValueAsInteger(valueAsInteger - decrement, pOutValue);
         }
         else if(pOperator[0] == '-')
         {
            setValueAsInteger(-valueAsInteger, pOutValue);
         }
         else if(pOperator[0] == '~')
         {
            setValueAsInteger(~valueAsInteger, pOutValue);
         }
      }
   }
   // decimal built-in
   else if(type->mCategory == TypeCategory::BuiltIn)
   {
      if(pOperator[0] == '-')
      {
         const double valueAsDecimal = getValueAsDecimal(pOperand);
         setValueAsDecimal(-valueAsDecimal, pOutValue);
      }
   }
   // struct or class
   else
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      CflatArgsVector(Value) args;

      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = findMethod(type, operatorIdentifier);
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pOperand, &thisPtrValue);

         operatorMethod->execute(thisPtrValue, args, pOutValue);
      }
      else
      {
         args.push_back(pOperand);

         Function* operatorFunction = type->mNamespace->getFunction(operatorIdentifier, args);

         if(!operatorFunction)
         {
            operatorFunction = findFunction(pContext, operatorIdentifier, args);
         }

         CflatAssert(operatorFunction);
         operatorFunction->execute(args, pOutValue);
      }
   }
}

void Environment::applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
   const char* pOperator, Value* pOutValue)
{
  if(!mErrorMessage.empty())
     return;

   Type* leftType = pLeft.mTypeUsage.mType;
   Type* rightType = pRight.mTypeUsage.mType;

   const bool leftIsNumericValue =
      leftType->mCategory == TypeCategory::BuiltIn ||
      leftType->mCategory == TypeCategory::Enum ||
      pLeft.mTypeUsage.isPointer();
   const bool rightIsNumericValue =
      rightType->mCategory == TypeCategory::BuiltIn ||
      rightType->mCategory == TypeCategory::Enum ||
      pRight.mTypeUsage.isPointer();

   if(leftIsNumericValue && rightIsNumericValue)
   {
      const bool integerLeftValue = leftType->isInteger() || pLeft.mTypeUsage.isPointer();
      const bool integerRightValue = rightType->isInteger() || pRight.mTypeUsage.isPointer();
      const bool integerValues = integerLeftValue && integerRightValue;

      int64_t leftValueAsInteger = 0;
      int64_t rightValueAsInteger = 0;
      double leftValueAsDecimal = 0.0;
      double rightValueAsDecimal = 0.0;

      if(integerLeftValue)
      {
         leftValueAsInteger = getValueAsInteger(pLeft);
 
         if(!integerRightValue)
         {
            leftValueAsDecimal = (double)leftValueAsInteger;
         }
      }
      else
      {
         leftValueAsDecimal = getValueAsDecimal(pLeft);
      }

      if(integerRightValue)
      {
         rightValueAsInteger = getValueAsInteger(pRight);

         if(!integerLeftValue)
         {
            rightValueAsDecimal = (double)rightValueAsInteger;
         }
      }
      else
      {
         rightValueAsDecimal = getValueAsDecimal(pRight);
      }

      if(strcmp(pOperator, "==") == 0)
      {
         const bool result = leftValueAsInteger == rightValueAsInteger;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "!=") == 0)
      {
         const bool result = leftValueAsInteger != rightValueAsInteger;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "<") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger < rightValueAsInteger
            : leftValueAsDecimal < rightValueAsDecimal;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, ">") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger > rightValueAsInteger
            : leftValueAsDecimal > rightValueAsDecimal;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "<=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger <= rightValueAsInteger
            : leftValueAsDecimal <= rightValueAsDecimal;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, ">=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger >= rightValueAsInteger
            : leftValueAsDecimal >= rightValueAsDecimal;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "&&") == 0)
      {
         const bool result = leftValueAsInteger && rightValueAsInteger;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "||") == 0)
      {
         const bool result = leftValueAsInteger || rightValueAsInteger;
         pOutValue->assign(&result);
      }
      else if(strcmp(pOperator, "+") == 0)
      {
         if(integerValues)
         {
            if(pLeft.mTypeUsage.isPointer())
            {
               rightValueAsInteger *= (int64_t)pLeft.mTypeUsage.mType->mSize;
            }

            setValueAsInteger(leftValueAsInteger + rightValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(leftValueAsDecimal + rightValueAsDecimal, pOutValue);
         }
      }
      else if(strcmp(pOperator, "-") == 0)
      {
         if(integerValues)
         {
            if(pLeft.mTypeUsage.isPointer())
            {
               rightValueAsInteger *= (int64_t)pLeft.mTypeUsage.mType->mSize;
            }

            setValueAsInteger(leftValueAsInteger - rightValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(leftValueAsDecimal - rightValueAsDecimal, pOutValue);
         }
      }
      else if(strcmp(pOperator, "*") == 0)
      {
         if(integerValues)
         {
            setValueAsInteger(leftValueAsInteger * rightValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(leftValueAsDecimal * rightValueAsDecimal, pOutValue);
         }
      }
      else if(strcmp(pOperator, "/") == 0)
      {
         if(integerValues)
         {
            if(rightValueAsInteger != 0)
            {
               setValueAsInteger(leftValueAsInteger / rightValueAsInteger, pOutValue);
            }
            else
            {
               throwRuntimeError(pContext, RuntimeError::DivisionByZero);
            }
         }
         else
         {
            if(fabs(rightValueAsDecimal) > 0.000000001)
            {
               setValueAsDecimal(leftValueAsDecimal / rightValueAsDecimal, pOutValue);
            }
            else
            {
               throwRuntimeError(pContext, RuntimeError::DivisionByZero);
            }
         }
      }
      else if(strcmp(pOperator, "%") == 0)
      {
         setValueAsInteger(leftValueAsInteger % rightValueAsInteger, pOutValue);
      }
      else if(strcmp(pOperator, "&") == 0)
      {
         setValueAsInteger(leftValueAsInteger & rightValueAsInteger, pOutValue);
      }
      else if(strcmp(pOperator, "|") == 0)
      {
         setValueAsInteger(leftValueAsInteger | rightValueAsInteger, pOutValue);
      }
      else if(strcmp(pOperator, "^") == 0)
      {
         setValueAsInteger(leftValueAsInteger ^ rightValueAsInteger, pOutValue);
      }
      else if(strcmp(pOperator, "<<") == 0)
      {
         setValueAsInteger(leftValueAsInteger << rightValueAsInteger, pOutValue);
      }
      else if(strcmp(pOperator, ">>") == 0)
      {
         setValueAsInteger(leftValueAsInteger >> rightValueAsInteger, pOutValue);
      }
   }
   else
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      CflatArgsVector(Value) args;
      args.push_back(pRight);

      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = findMethod(leftType, operatorIdentifier, args);
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pLeft, &thisPtrValue);

         operatorMethod->execute(thisPtrValue, args, pOutValue);
      }
      else
      {
         args.insert(args.begin(), pLeft);

         Function* operatorFunction = leftType->mNamespace->getFunction(operatorIdentifier, args);

         if(!operatorFunction)
         {
            operatorFunction = findFunction(pContext, operatorIdentifier, args);
         }

         CflatAssert(operatorFunction);
         operatorFunction->execute(args, pOutValue);
      }
   }
}

void Environment::performAssignment(ExecutionContext& pContext, const Value& pValue,
   const char* pOperator, Value* pInstanceDataValue)
{
   if(strcmp(pOperator, "=") == 0)
   {
      assignValue(pContext, pValue, pInstanceDataValue, false);
   }
   else
   {
      char binaryOperator[2];
      binaryOperator[0] = pOperator[0];
      binaryOperator[1] = '\0';

      applyBinaryOperator(pContext, *pInstanceDataValue, pValue, binaryOperator, pInstanceDataValue);
   }
}

void Environment::performStaticCast(ExecutionContext& pContext, const Value& pValueToCast,
   const TypeUsage& pTargetTypeUsage, Value* pOutValue)
{
   const TypeUsage& sourceTypeUsage = pValueToCast.mTypeUsage;

   if(sourceTypeUsage.mType->mCategory == TypeCategory::BuiltIn &&
      pTargetTypeUsage.mType->mCategory == TypeCategory::BuiltIn)
   {
      if(sourceTypeUsage.mType->isInteger())
      {
         const int64_t sourceValueAsInteger = getValueAsInteger(pValueToCast);

         if(pTargetTypeUsage.mType->isInteger())
         {
            setValueAsInteger(sourceValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal((double)sourceValueAsInteger, pOutValue);
         }
      }
      else
      {
         const double sourceValueAsDecimal = getValueAsDecimal(pValueToCast);

         if(pTargetTypeUsage.mType->isInteger())
         {
            setValueAsInteger((int64_t)sourceValueAsDecimal, pOutValue);
         }
         else
         {
            setValueAsDecimal(sourceValueAsDecimal, pOutValue);
         }
      }
   }
   else if(sourceTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
      pTargetTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
   {
      performInheritanceCast(pContext, pValueToCast, pTargetTypeUsage, pOutValue);
   }
}

void Environment::performIntegerCast(ExecutionContext& pContext, const Value& pValueToCast,
   const TypeUsage& pTargetTypeUsage, Value* pOutValue)
{
   setValueAsInteger(getValueAsInteger(pValueToCast), pOutValue);
}

void Environment::performIntegerFloatCast(ExecutionContext& pContext, const Value& pValueToCast,
   const TypeUsage& pTargetTypeUsage, Value* pOutValue)
{
   Type* sourceType = pValueToCast.mTypeUsage.mType;
   Type* targetType = pTargetTypeUsage.mType;

   CflatAssert(sourceType->mCategory == TypeCategory::BuiltIn);
   CflatAssert(targetType->mCategory == TypeCategory::BuiltIn);

   if(sourceType->isInteger() && targetType->isDecimal())
   {
      const int64_t integerValue = getValueAsInteger(pValueToCast);
      setValueAsDecimal((double)integerValue, pOutValue);
   }
   else if(sourceType->isDecimal() && targetType->isInteger())
   {
      const double decimalValue = getValueAsDecimal(pValueToCast);
      setValueAsInteger((int64_t)decimalValue, pOutValue);
   }
}

void Environment::performFloatCast(ExecutionContext& pContext, const Value& pValueToCast,
   const TypeUsage& pTargetTypeUsage, Value* pOutValue)
{
   Type* sourceType = pValueToCast.mTypeUsage.mType;
   Type* targetType = pTargetTypeUsage.mType;

   CflatAssert(sourceType->mCategory == TypeCategory::BuiltIn);
   CflatAssert(targetType->mCategory == TypeCategory::BuiltIn);

   const double decimalValue = getValueAsDecimal(pValueToCast);
   setValueAsDecimal(decimalValue, pOutValue);
}

void Environment::performInheritanceCast(ExecutionContext& pContext, const Value& pValueToCast,
   const TypeUsage& pTargetTypeUsage, Value* pOutValue)
{
   CflatAssert(pValueToCast.mTypeUsage.mType->mCategory == TypeCategory::StructOrClass);
   CflatAssert(pTargetTypeUsage.mType->mCategory == TypeCategory::StructOrClass);

   Struct* sourceType = static_cast<Struct*>(pValueToCast.mTypeUsage.mType);
   Struct* targetType = static_cast<Struct*>(pTargetTypeUsage.mType);

   char* ptr = nullptr;

   if(sourceType == targetType)
   {
      ptr = CflatValueAs(&pValueToCast, char*);
   }
   else if(sourceType->derivedFrom(targetType))
   {
      ptr = CflatValueAs(&pValueToCast, char*) + sourceType->getOffset(targetType);
   }
   else if(targetType->derivedFrom(sourceType))
   {
      ptr = CflatValueAs(&pValueToCast, char*) - targetType->getOffset(sourceType);
   }

   pOutValue->set(&ptr);
}

void Environment::assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
   bool pDeclaration)
{
   const TypeUsage typeUsage = pTarget->mTypeUsage;
   const TypeHelper::Compatibility compatibility =
      TypeHelper::getCompatibility(typeUsage, pSource.mTypeUsage);

   if(compatibility == TypeHelper::Compatibility::ImplicitCastableInteger)
   {
      performIntegerCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(compatibility == TypeHelper::Compatibility::ImplicitCastableIntegerFloat)
   {
      performIntegerFloatCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(compatibility == TypeHelper::Compatibility::ImplicitCastableFloat)
   {
      performFloatCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(compatibility == TypeHelper::Compatibility::ImplicitCastableInheritance)
   {
      performInheritanceCast(pContext, pSource, typeUsage, pTarget);
   }
   else
   {
      bool valueAssigned = false;

      if(!pTarget->mTypeUsage.isPointer() &&
         pTarget->mTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
      {
         Struct* type = static_cast<Struct*>(pTarget->mTypeUsage.mType);

         CflatArgsVector(Value) args;
         args.push_back(pSource);

         const Identifier operatorIdentifier("operator=");
         Method* operatorMethod = findMethod(type, operatorIdentifier, args);

         if(operatorMethod && operatorMethod->mReturnTypeUsage.mType == type)
         {
            Value thisPtrValue;
            thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, *pTarget, &thisPtrValue);

            operatorMethod->execute(thisPtrValue, args, pTarget);

            valueAssigned = true;
         }
      }

      if(!valueAssigned)
      {
         if(pDeclaration)
         {
            *pTarget = pSource;
         }
         else
         {
            pTarget->assign(pSource.mValueBuffer);
         }
      }
   }
}

void Environment::execute(ExecutionContext& pContext, const Program& pProgram)
{
   pContext.mJumpStatement = JumpStatement::None;

   pContext.mCallStack.emplace_back(&pProgram);

   for(size_t i = 0u; i < pProgram.mStatements.size(); i++)
   {
      execute(pContext, pProgram.mStatements[i]);

      if(!mErrorMessage.empty())
      {
         break;
      }
   }

   pContext.mCallStack.pop_back();

   if(mExecutionHook)
   {
      mExecutionHook(this, pContext.mCallStack);
   }

   pContext.mUsingDirectives.clear();
}

void Environment::assertValueInitialization(ExecutionContext& pContext, const TypeUsage& pTypeUsage,
   Value* pOutValue)
{
   if(pOutValue->mValueBufferType == ValueBufferType::Uninitialized)
   {
      if(pTypeUsage.isReference())
      {
         pOutValue->initExternal(pTypeUsage);
      }
      else if(pOutValue->mValueInitializationHint == ValueInitializationHint::Stack)
      {
         pOutValue->initOnStack(pTypeUsage, &pContext.mStack);
      }
      else
      {
         pOutValue->initOnHeap(pTypeUsage);
      }
   }
}

int64_t Environment::getValueAsInteger(const Value& pValue)
{
   const bool signedType =
      !pValue.mTypeUsage.isPointer() &&
      pValue.mTypeUsage.mType->mCategory == TypeCategory::BuiltIn &&
      pValue.mTypeUsage.mType->mIdentifier.mName[0] == 'i';

   const size_t typeUsageSize = pValue.mTypeUsage.getSize();
   int64_t valueAsInteger = 0;

   if(typeUsageSize == sizeof(int32_t))
   {
      valueAsInteger = signedType
         ? (int64_t)CflatValueAs(&pValue, int32_t)
         : (int64_t)CflatValueAs(&pValue, uint32_t);
   }
   else if(typeUsageSize == sizeof(int64_t))
   {
      valueAsInteger = CflatValueAs(&pValue, int64_t);
   }
   else if(typeUsageSize == sizeof(int16_t))
   {
      valueAsInteger = signedType
         ? (int64_t)CflatValueAs(&pValue, int16_t)
         : (int64_t)CflatValueAs(&pValue, uint16_t);
   }
   else if(typeUsageSize == sizeof(int8_t))
   {
      valueAsInteger = signedType
         ? (int64_t)CflatValueAs(&pValue, int8_t)
         : (int64_t)CflatValueAs(&pValue, uint8_t);
   }
   else
   {
      CflatAssert(false); // Unsupported
   }

   return valueAsInteger;
}

double Environment::getValueAsDecimal(const Value& pValue)
{
   const size_t typeUsageSize = pValue.mTypeUsage.getSize();
   double valueAsDecimal = 0.0;

   if(typeUsageSize == sizeof(float))
   {
      valueAsDecimal = (double)CflatValueAs(&pValue, float);
   }
   else if(typeUsageSize == sizeof(double))
   {
      valueAsDecimal = CflatValueAs(&pValue, double);
   }
   else
   {
      CflatAssert(false); // Unsupported
   }

   return valueAsDecimal;
}

void Environment::setValueAsInteger(int64_t pInteger, Value* pOutValue)
{
   const size_t typeUsageSize = pOutValue->mTypeUsage.getSize();

   if(typeUsageSize == sizeof(int32_t))
   {
      const int32_t value = (int32_t)pInteger;
      pOutValue->assign(&value);
   }
   else if(typeUsageSize == sizeof(int64_t))
   {
      pOutValue->assign(&pInteger);
   }
   else if(typeUsageSize == sizeof(int16_t))
   {
      const int16_t value = (int16_t)pInteger;
      pOutValue->assign(&value);
   }
   else if(typeUsageSize == sizeof(int8_t))
   {
      const int8_t value = (int8_t)pInteger;
      pOutValue->assign(&value);
   }
   else
   {
      CflatAssert(false); // Unsupported
   }
}

void Environment::setValueAsDecimal(double pDecimal, Value* pOutValue)
{
   const size_t typeUsageSize = pOutValue->mTypeUsage.getSize();

   if(typeUsageSize == sizeof(float))
   {
      const float value = (float)pDecimal;
      pOutValue->assign(&value);
   }
   else if(typeUsageSize == sizeof(double))
   {
      pOutValue->assign(&pDecimal);
   }
   else
   {
      CflatAssert(false); // Unsupported
   }
}

Method* Environment::getDefaultConstructor(Type* pType)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);

   Method* defaultConstructor = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(type->mMethods[i].mParameters.empty() &&
         type->mMethods[i].mIdentifier.mHash == 0u)
      {
         defaultConstructor = &type->mMethods[i];
         break;
      }
   }

   return defaultConstructor;
}

Method* Environment::getDestructor(Type* pType)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);

   Method* destructor = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(type->mMethods[i].mIdentifier.mName[0] == '~')
      {
         destructor = &type->mMethods[i];
         break;
      }
   }

   return destructor;
}

Method* Environment::findConstructor(Type* pType, const CflatArgsVector(TypeUsage)& pParameterTypes)
{
   const Identifier emptyId;
   return findMethod(pType, emptyId, pParameterTypes);
}

Method* Environment::findConstructor(Type* pType, const CflatArgsVector(Value)& pArguments)
{
   const Identifier emptyId;
   return findMethod(pType, emptyId, pArguments);
}

Method* Environment::findMethod(Type* pType, const Identifier& pIdentifier)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);

   Method* method = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(type->mMethods[i].mIdentifier == pIdentifier)
      {
         method = &type->mMethods[i];
         break;
      }
   }

   return method;
}

Method* Environment::findMethod(Type* pType, const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);

   Method* method = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   // first pass: look for a perfect argument match
   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(type->mMethods[i].mIdentifier == pIdentifier &&         
         type->mMethods[i].mParameters.size() == pParameterTypes.size() &&
         type->mMethods[i].mTemplateTypes == pTemplateTypes)
      {
         bool parametersMatch = true;

         for(size_t j = 0u; j < pParameterTypes.size(); j++)
         {
            const TypeHelper::Compatibility compatibility =
               TypeHelper::getCompatibility(type->mMethods[i].mParameters[j], pParameterTypes[j]);

            if(compatibility != TypeHelper::Compatibility::PerfectMatch)
            {
               parametersMatch = false;
               break;
            }
         }

         if(parametersMatch)
         {
            method = &type->mMethods[i];
            break;
         }
      }
   }

   // second pass: look for a compatible argument match
   if(!method)
   {
      for(size_t i = 0u; i < type->mMethods.size(); i++)
      {
         if(type->mMethods[i].mIdentifier == pIdentifier &&
            type->mMethods[i].mParameters.size() == pParameterTypes.size() &&
            type->mMethods[i].mTemplateTypes == pTemplateTypes)
         {
            bool parametersMatch = true;

            for(size_t j = 0u; j < pParameterTypes.size(); j++)
            {
               const TypeHelper::Compatibility compatibility =
                  TypeHelper::getCompatibility(type->mMethods[i].mParameters[j], pParameterTypes[j]);

               if(compatibility == TypeHelper::Compatibility::Incompatible)
               {
                  parametersMatch = false;
                  break;
               }
            }

            if(parametersMatch)
            {
               method = &type->mMethods[i];
               break;
            }
         }
      }
   }

   return method;
}

Method* Environment::findMethod(Type* pType, const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   CflatArgsVector(TypeUsage) typeUsages;

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return findMethod(pType, pIdentifier, typeUsages);
}

void Environment::initArgumentsForFunctionCall(Function* pFunction, CflatArgsVector(Value)& pArgs)
{
   pArgs.resize(pFunction->mParameters.size());

   for(size_t i = 0u; i < pFunction->mParameters.size(); i++)
   {
      const TypeUsage& typeUsage = pFunction->mParameters[i];

      if(typeUsage.isReference())
      {
         pArgs[i].initExternal(typeUsage);
      }
      else
      {
         pArgs[i].initOnStack(typeUsage, &mExecutionContext.mStack);
      }
   }
}

void Environment::execute(ExecutionContext& pContext, Statement* pStatement)
{
   if(!mErrorMessage.empty())
      return;

   pContext.mProgram = pStatement->mProgram;

   pContext.mCallStack.back().mProgram = pStatement->mProgram;
   pContext.mCallStack.back().mLine = pStatement->mLine;

   if(mExecutionHook)
   {
      mExecutionHook(this, pContext.mCallStack);
   }

   switch(pStatement->getType())
   {
   case StatementType::Expression:
      {
         StatementExpression* statement = static_cast<StatementExpression*>(pStatement);

         Value unusedValue;
         unusedValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, statement->mExpression, &unusedValue);
      }
      break;
   case StatementType::Block:
      {
         StatementBlock* statement = static_cast<StatementBlock*>(pStatement);

         incrementBlockLevel(pContext);

         if(statement->mAlterScope)
         {
            incrementScopeLevel(pContext);
         }

         for(size_t i = 0u; i < statement->mStatements.size(); i++)
         {
            execute(pContext, statement->mStatements[i]);

            if(pContext.mJumpStatement != JumpStatement::None)
            {
               break;
            }
         }

         if(statement->mAlterScope)
         {
            decrementScopeLevel(pContext);
         }

         decrementBlockLevel(pContext);
      }
      break;
   case StatementType::UsingDirective:
      {
         StatementUsingDirective* statement =
            static_cast<StatementUsingDirective*>(pStatement);

         if(statement->mNamespace)
         {
            UsingDirective usingDirective(statement->mNamespace);
            usingDirective.mBlockLevel = pContext.mBlockLevel;
            pContext.mUsingDirectives.push_back(usingDirective);
         }
         else
         {
            registerTypeAlias(pContext, statement->mAliasIdentifier, statement->mAliasTypeUsage);
         }
      }
      break;
   case StatementType::TypeDefinition:
      {
         StatementTypeDefinition* statement =
            static_cast<StatementTypeDefinition*>(pStatement);

         TypeAlias typeAlias(statement->mAlias, statement->mReferencedTypeUsage);
         typeAlias.mScopeLevel = pContext.mScopeLevel;
         pContext.mTypeAliases.push_back(typeAlias);
      }
      break;
   case StatementType::NamespaceDeclaration:
      {
         StatementNamespaceDeclaration* statement =
            static_cast<StatementNamespaceDeclaration*>(pStatement);
         Namespace* ns =
            pContext.mNamespaceStack.back()->requestNamespace(statement->mNamespaceIdentifier);

         pContext.mNamespaceStack.push_back(ns);
         execute(pContext, statement->mBody);
         pContext.mNamespaceStack.pop_back();
      }
      break;
   case StatementType::VariableDeclaration:
      {
         StatementVariableDeclaration* statement = static_cast<StatementVariableDeclaration*>(pStatement);

         const bool isStaticVariable =
            statement->mStatic && pContext.mScopeLevel > 0u && !statement->mTypeUsage.isConst();
         Instance* instance = isStaticVariable
            ? registerStaticInstance(pContext, statement->mTypeUsage, statement->mVariableIdentifier, statement)
            : registerInstance(pContext, statement->mTypeUsage, statement->mVariableIdentifier);

         const bool isStructOrClassInstance =
            instance->mTypeUsage.mType &&
            instance->mTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
            !instance->mTypeUsage.isPointer() &&
            !instance->mTypeUsage.isReference();

         if(isStructOrClassInstance)
         {
            Method* defaultCtor = getDefaultConstructor(instance->mTypeUsage.mType);

            if(defaultCtor)
            {
               instance->mValue.mTypeUsage = instance->mTypeUsage;

               Value thisPtr;
               thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
               getAddressOfValue(pContext, instance->mValue, &thisPtr);

               CflatArgsVector(Value) args;
               defaultCtor->execute(thisPtr, args, nullptr);
            }
         }

         if(statement->mInitialValue)
         {
            Value initialValue;
            initialValue.mTypeUsage = instance->mTypeUsage;
            initialValue.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, statement->mInitialValue, &initialValue);
            assignValue(pContext, initialValue, &instance->mValue, true);
         }
      }
      break;
   case StatementType::FunctionDeclaration:
      {
         StatementFunctionDeclaration* statement = static_cast<StatementFunctionDeclaration*>(pStatement);

         CflatArgsVector(TypeUsage) parameterTypes;
         toArgsVector(statement->mParameterTypes, parameterTypes);

         Namespace* functionNS = pContext.mNamespaceStack.back();
         Function* function = functionNS->getFunction(statement->mFunctionIdentifier, parameterTypes);
         CflatAssert(function);

         function->mProgram = statement->mProgram;
         function->mLine = statement->mLine;

         statement->mFunction = function;

         if(statement->mBody)
         {
            function->mUsingDirectives = pContext.mUsingDirectives;
            function->execute =
               [this, &pContext, function, functionNS, statement]
               (const CflatArgsVector(Value)& pArguments, Value* pOutReturnValue)
            {
               CflatAssert(function->mParameters.size() == pArguments.size());

               const bool mustReturnValue =
                  function->mReturnTypeUsage.mType && function->mReturnTypeUsage.mType != mTypeVoid;
               
               if(mustReturnValue)
               {
                  if(pOutReturnValue)
                  {
                     assertValueInitialization(pContext, function->mReturnTypeUsage, pOutReturnValue);
                  }

                  pContext.mReturnValues.push_back(Value());
                  pContext.mReturnValues.back().initOnStack(function->mReturnTypeUsage, &pContext.mStack);
               }

               pContext.mNamespaceStack.push_back(functionNS);

               for(size_t i = 0u; i < pArguments.size(); i++)
               {
                  const TypeUsage parameterType = statement->mParameterTypes[i];
                  const Identifier& parameterIdentifier = statement->mParameterIdentifiers[i];

                  pContext.mScopeLevel++;
                  Instance* argumentInstance =
                     registerInstance(pContext, parameterType, parameterIdentifier);
                  pContext.mScopeLevel--;

                  assignValue(pContext, pArguments[i], &argumentInstance->mValue, true);
               }

               for(size_t i = 0u; i < function->mUsingDirectives.size(); i++)
               {
                  pContext.mUsingDirectives.push_back(function->mUsingDirectives[i]);
                  pContext.mUsingDirectives.back().mBlockLevel = 0u;
               }

               pContext.mCallStack.emplace_back(statement->mProgram, function);

               execute(pContext, statement->mBody);

               pContext.mCallStack.pop_back();

               for(size_t i = 0u; i < function->mUsingDirectives.size(); i++)
               {
                  pContext.mUsingDirectives.pop_back();
               }

               if(mExecutionHook && pContext.mCallStack.empty())
               {
                  mExecutionHook(this, pContext.mCallStack);
               }

               pContext.mNamespaceStack.pop_back();

               if(mustReturnValue)
               {
                  if(pOutReturnValue)
                  {
                     pOutReturnValue->set(pContext.mReturnValues.back().mValueBuffer);
                  }

                  pContext.mReturnValues.pop_back();
               }

               pContext.mJumpStatement = JumpStatement::None;
            };
         }
      }
      break;
   case StatementType::If:
      {
         StatementIf* statement = static_cast<StatementIf*>(pStatement);

         Value conditionValue;
         conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, statement->mCondition, &conditionValue);

         if(getValueAsInteger(conditionValue))
         {
            execute(pContext, statement->mIfStatement);
         }
         else if(statement->mElseStatement)
         {
            execute(pContext, statement->mElseStatement);
         }
      }
      break;
   case StatementType::Switch:
      {
         StatementSwitch* statement = static_cast<StatementSwitch*>(pStatement);

         Value conditionValue;
         conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, statement->mCondition, &conditionValue);

         const int64_t conditionValueAsInteger = getValueAsInteger(conditionValue);
         bool statementExecution = false;

         for(size_t i = 0u; i < statement->mCaseSections.size(); i++)
         {
            const StatementSwitch::CaseSection& caseSection = statement->mCaseSections[i];

            if(!statementExecution)
            {
               // case
               if(caseSection.mExpression)
               {
                  Value caseValue;
                  caseValue.mValueInitializationHint = ValueInitializationHint::Stack;
                  evaluateExpression(pContext, caseSection.mExpression, &caseValue);

                  const int64_t caseValueAsInteger = getValueAsInteger(caseValue);

                  if(caseValueAsInteger == conditionValueAsInteger)
                  {
                     statementExecution = true;
                  }
               }
               // default
               else
               {
                  statementExecution = true;
               }
            }

            if(statementExecution)
            {
               for(size_t j = 0u; j < caseSection.mStatements.size(); j++)
               {
                  execute(pContext, caseSection.mStatements[j]);

                  if(pContext.mJumpStatement == JumpStatement::Break)
                  {
                     break;
                  }
               }
            }

            if(pContext.mJumpStatement == JumpStatement::Break)
            {
               pContext.mJumpStatement = JumpStatement::None;
               break;
            }
         }
      }
      break;
   case StatementType::While:
      {
         StatementWhile* statement = static_cast<StatementWhile*>(pStatement);

         Value conditionValue;
         conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, statement->mCondition, &conditionValue);

         while(getValueAsInteger(conditionValue))
         {
            execute(pContext, statement->mLoopStatement);

            if(pContext.mJumpStatement == JumpStatement::Continue)
            {
               pContext.mJumpStatement = JumpStatement::None;
            }
            else if(pContext.mJumpStatement == JumpStatement::Break)
            {
               pContext.mJumpStatement = JumpStatement::None;
               break;
            }

            evaluateExpression(pContext, statement->mCondition, &conditionValue);
         }
      }
      break;
   case StatementType::DoWhile:
      {
         StatementDoWhile* statement = static_cast<StatementDoWhile*>(pStatement);

         Value conditionValue;
         conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;

         do
         {
            execute(pContext, statement->mLoopStatement);

            if(pContext.mJumpStatement == JumpStatement::Continue)
            {
               pContext.mJumpStatement = JumpStatement::None;
            }
            else if(pContext.mJumpStatement == JumpStatement::Break)
            {
               pContext.mJumpStatement = JumpStatement::None;
               break;
            }

            evaluateExpression(pContext, statement->mCondition, &conditionValue);
         }
         while(getValueAsInteger(conditionValue));
      }
      break;
   case StatementType::For:
      {
         StatementFor* statement = static_cast<StatementFor*>(pStatement);

         incrementScopeLevel(pContext);

         if(statement->mInitialization)
         {
            execute(pContext, statement->mInitialization);
         }

         {
            const bool defaultConditionValue = true;

            Value conditionValue;
            conditionValue.initOnStack(mTypeUsageBool, &pContext.mStack);
            conditionValue.set(&defaultConditionValue);

            bool conditionMet = defaultConditionValue;

            if(statement->mCondition)
            {
               evaluateExpression(pContext, statement->mCondition, &conditionValue);
               conditionMet = getValueAsInteger(conditionValue) != 0;
            }

            while(conditionMet)
            {
               execute(pContext, statement->mLoopStatement);

               if(pContext.mJumpStatement == JumpStatement::Continue)
               {
                  pContext.mJumpStatement = JumpStatement::None;
               }
               else if(pContext.mJumpStatement == JumpStatement::Break)
               {
                  pContext.mJumpStatement = JumpStatement::None;
                  break;
               }

               if(statement->mIncrement)
               {
                  Value unusedValue;
                  evaluateExpression(pContext, statement->mIncrement, &unusedValue);
               }

               if(statement->mCondition)
               {
                  evaluateExpression(pContext, statement->mCondition, &conditionValue);
                  conditionMet = getValueAsInteger(conditionValue) != 0;
               }
            }
         }

         decrementScopeLevel(pContext);
      }
      break;
   case StatementType::ForRangeBased:
      {
         StatementForRangeBased* statement = static_cast<StatementForRangeBased*>(pStatement);

         incrementScopeLevel(pContext);

         Instance* elementInstance = registerInstance(
            pContext, statement->mVariableTypeUsage, statement->mVariableIdentifier);

         Value collectionDataValue;
         collectionDataValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getInstanceDataValue(pContext, statement->mCollection, &collectionDataValue);

         Value collectionThisValue;
         collectionThisValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, collectionDataValue, &collectionThisValue);

         if(collectionDataValue.mTypeUsage.isArray())
         {
            size_t elementIndex = 0u;

            while(elementIndex < collectionDataValue.mTypeUsage.mArraySize)
            {
               const size_t arrayElementSize = statement->mVariableTypeUsage.getSize();
               const char* arrayElementData =
                  collectionDataValue.mValueBuffer + (arrayElementSize * elementIndex);
               elementInstance->mValue.set(arrayElementData);

               execute(pContext, statement->mLoopStatement);

               if(pContext.mJumpStatement == JumpStatement::Continue)
               {
                  pContext.mJumpStatement = JumpStatement::None;
               }
               else if(pContext.mJumpStatement == JumpStatement::Break)
               {
                  pContext.mJumpStatement = JumpStatement::None;
                  break;
               }

               elementIndex++;
            }
         }
         else
         {
            Struct* collectionType = static_cast<Struct*>(collectionDataValue.mTypeUsage.mType);

            Method* collectionBeginMethod =
               findMethod(collectionType, "begin", TypeUsage::kEmptyList);
            Value iteratorValue;
            iteratorValue.initOnStack(collectionBeginMethod->mReturnTypeUsage, &pContext.mStack);
            collectionBeginMethod->execute(collectionThisValue, Value::kEmptyList, &iteratorValue);

            Method* collectionEndMethod =
               findMethod(collectionType, "end", TypeUsage::kEmptyList);
            Value collectionEndValue;
            collectionEndValue.initOnStack(collectionEndMethod->mReturnTypeUsage, &pContext.mStack);
            collectionEndMethod->execute(collectionThisValue, Value::kEmptyList, &collectionEndValue);

            Type* iteratorType = collectionBeginMethod->mReturnTypeUsage.mType;

            Value conditionValue;
            conditionValue.initOnStack(mTypeUsageBool, &pContext.mStack);
            applyBinaryOperator(pContext, iteratorValue, collectionEndValue, "!=", &conditionValue);

            while(CflatValueAs(&conditionValue, bool))
            {
               applyUnaryOperator(pContext, iteratorValue, "*", &elementInstance->mValue);

               execute(pContext, statement->mLoopStatement);

               if(pContext.mJumpStatement == JumpStatement::Continue)
               {
                  pContext.mJumpStatement = JumpStatement::None;
               }
               else if(pContext.mJumpStatement == JumpStatement::Break)
               {
                  pContext.mJumpStatement = JumpStatement::None;
                  break;
               }

               applyUnaryOperator(pContext, iteratorValue, "++", &iteratorValue);
               applyBinaryOperator(pContext, iteratorValue, collectionEndValue, "!=", &conditionValue);
            }
         }

         collectionThisValue.reset();
         collectionDataValue.reset();

         decrementScopeLevel(pContext);
      }
      break;
   case StatementType::Break:
      {
         pContext.mJumpStatement = JumpStatement::Break;
      }
      break;
   case StatementType::Continue:
      {
         pContext.mJumpStatement = JumpStatement::Continue;
      }
      break;
   case StatementType::Return:
      {
         StatementReturn* statement = static_cast<StatementReturn*>(pStatement);

         if(statement->mExpression)
         {
            evaluateExpression(pContext, statement->mExpression, &pContext.mReturnValues.back());
         }

         pContext.mJumpStatement = JumpStatement::Return;
      }
      break;
   default:
      break;
   }
}

Namespace* Environment::getGlobalNamespace()
{
   return &mGlobalNamespace;
}

Namespace* Environment::getNamespace(const Identifier& pIdentifier)
{
   return mGlobalNamespace.getNamespace(pIdentifier);
}

Namespace* Environment::requestNamespace(const Identifier& pIdentifier)
{
   return mGlobalNamespace.requestNamespace(pIdentifier);
}

Type* Environment::getType(const Identifier& pIdentifier)
{
   return mGlobalNamespace.getType(pIdentifier);
}

Type* Environment::getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
{
   return mGlobalNamespace.getType(pIdentifier, pTemplateTypes);
}

TypeUsage Environment::getTypeUsage(const char* pTypeName, Namespace* pNamespace)
{
   mTypesParsingContext.mTokenIndex = 0u;

   mTypesParsingContext.mNamespaceStack.clear();
   mTypesParsingContext.mNamespaceStack.push_back(pNamespace ? pNamespace : &mGlobalNamespace);

   mTypesParsingContext.mPreprocessedCode.assign(pTypeName);
   mTypesParsingContext.mPreprocessedCode.push_back('\n');

   tokenize(mTypesParsingContext);

   return parseTypeUsage(mTypesParsingContext, 0u);
}

Function* Environment::registerFunction(const Identifier& pIdentifier)
{
   return mGlobalNamespace.registerFunction(pIdentifier);
}

Function* Environment::getFunction(const Identifier& pIdentifier)
{
   return mGlobalNamespace.getFunction(pIdentifier);
}

Function* Environment::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes)
{
   return mGlobalNamespace.getFunction(pIdentifier, pParameterTypes);
}

Function* Environment::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments)
{
   return mGlobalNamespace.getFunction(pIdentifier, pArguments);
}

CflatSTLVector(Function*)* Environment::getFunctions(const Identifier& pIdentifier)
{
   return mGlobalNamespace.getFunctions(pIdentifier);
}

void Environment::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   mGlobalNamespace.setVariable(pTypeUsage, pIdentifier, pValue);
}

Value* Environment::getVariable(const Identifier& pIdentifier)
{
   return mGlobalNamespace.getVariable(pIdentifier);
}

void Environment::voidFunctionCall(Function* pFunction)
{
   CflatAssert(pFunction);

   mErrorMessage.clear();

   Value returnValue;

   CflatArgsVector(Value) args;
   pFunction->execute(args, &returnValue);
}

bool Environment::load(const char* pProgramName, const char* pCode)
{
   const Identifier programIdentifier(pProgramName);

   Program* program = (Program*)CflatMalloc(sizeof(Program));
   CflatInvokeCtor(Program, program);

   program->mIdentifier = programIdentifier;
   program->mCode.assign(pCode);

   mErrorMessage.clear();

   ParsingContext parsingContext(&mGlobalNamespace);
   parsingContext.mProgram = program;

   preprocess(parsingContext, pCode);

   if(mErrorMessage.empty())
   {
      tokenize(parsingContext);
      parse(parsingContext);
   }

   if(!mErrorMessage.empty())
   {
      CflatInvokeDtor(Program, program);
      CflatFree(program);
      return false;
   }

   execute(mExecutionContext, *program);

   ProgramsRegistry::const_iterator it = mPrograms.find(programIdentifier.mHash);

   if(it != mPrograms.end())
   {
      CflatInvokeDtor(Program, it->second);
      CflatFree(it->second);
   }

   mPrograms[programIdentifier.mHash] = program;

   if(!mErrorMessage.empty())
   {
      return false;
   }

   return true;
}

bool Environment::load(const char* pFilePath)
{
   FILE* file = fopen(pFilePath, "rb");

   if(!file)
      return false;

   fseek(file, 0, SEEK_END);
   const size_t fileSize = (size_t)ftell(file);
   rewind(file);

   char* code = (char*)CflatMalloc(fileSize + 1u);
   code[fileSize] = '\0';

   fread(code, 1u, fileSize, file);
   fclose(file);

   const bool success = load(pFilePath, code);
   CflatFree(code);

   return success;
}

const char* Environment::getErrorMessage()
{
   return mErrorMessage.empty() ? nullptr : mErrorMessage.c_str();
}

void Environment::setExecutionHook(ExecutionHook pExecutionHook)
{
   mExecutionHook = pExecutionHook;
}

bool Environment::evaluateExpression(const char* pExpression, Value* pOutValue)
{
   ParsingContext parsingContext(&mGlobalNamespace);
   parsingContext.mProgram = mExecutionContext.mProgram;
   parsingContext.mScopeLevel = mExecutionContext.mScopeLevel;
   parsingContext.mNamespaceStack = mExecutionContext.mNamespaceStack;
   parsingContext.mUsingDirectives = mExecutionContext.mUsingDirectives;
   parsingContext.mLocalInstancesHolder = mExecutionContext.mLocalInstancesHolder;

   preprocess(parsingContext, pExpression);
   tokenize(parsingContext);
   
   CflatSTLVector(Token)& tokens = parsingContext.mTokens;
   
   if(!tokens.empty())
   {
      Expression* expression = parseExpression(parsingContext, tokens.size() - 1u, true);

      if(expression)
      {
         CflatAssert(pOutValue);
         evaluateExpression(mExecutionContext, expression, pOutValue);
         mErrorMessage.clear();

         return pOutValue->mValueBufferType != ValueBufferType::Uninitialized;
      }
   }

   mErrorMessage.clear();

   return false;
}

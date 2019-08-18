
////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.10
//  Embeddable lightweight scripting language with C++ syntax
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

#include "Cflat.h"

#include <iostream>


//
//  Memory management
//
namespace Cflat
{
   void* (*Memory::malloc)(size_t pSize) = ::malloc;
   void (*Memory::free)(void* pPtr) = ::free;
}


//
//  Global functions
//
namespace Cflat
{
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

   uint32_t hash(const char* pString)
   {
      const uint32_t kOffsetBasis = 2166136261u;
      const uint32_t kFNVPrime = 16777619u;

      uint32_t charIndex = 0u;
      uint32_t hash = kOffsetBasis;

      while(pString[charIndex] != '\0')
      {
         hash ^= pString[charIndex++];
         hash *= kFNVPrime;
      }

      return hash;
   }
}


//
//  Static members
//
namespace Cflat
{
   Identifier::NamesRegistry Identifier::smNames;
}


//
//  AST Types
//
namespace Cflat
{
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
      Conditional,
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
      CflatSTLVector(Identifier) mIdentifiers;

      ExpressionMemberAccess()
      {
         mType = ExpressionType::MemberAccess;
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

      ExpressionBinaryOperation(Expression* pLeft, Expression* pRight, const char* pOperator)
         : mLeft(pLeft)
         , mRight(pRight)
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

   struct ExpressionFunctionCall : Expression
   {
      Identifier mFunctionIdentifier;
      CflatSTLVector(Expression*) mArguments;

      ExpressionFunctionCall(const Identifier& pFunctionIdentifier)
         : mFunctionIdentifier(pFunctionIdentifier)
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

      ExpressionMethodCall(Expression* pMemberAccess)
         : mMemberAccess(pMemberAccess)
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

      ExpressionObjectConstruction(Type* pObjectType)
         : mObjectType(pObjectType)
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
      NamespaceDeclaration,
      VariableDeclaration,
      FunctionDeclaration,
      Assignment,
      If,
      Switch,
      While,
      DoWhile,
      For,
      Break,
      Continue,
      Return
   };

   struct Statement
   {
   protected:
      StatementType mType;

      Statement()
         : mLine(0u)
      {
      }

   public:
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

      StatementBlock()
         : mAlterScope(true)
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

      StatementUsingDirective(Namespace* pNamespace)
         : mNamespace(pNamespace)
      {
         mType = StatementType::UsingDirective;
      }
   };

   struct StatementVariableDeclaration : Statement
   {
      TypeUsage mTypeUsage;
      Identifier mVariableIdentifier;
      Expression* mInitialValue;

      StatementVariableDeclaration(const TypeUsage& pTypeUsage, const Identifier& pVariableIdentifier,
         Expression* pInitialValue)
         : mTypeUsage(pTypeUsage)
         , mVariableIdentifier(pVariableIdentifier)
         , mInitialValue(pInitialValue)
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

      StatementFunctionDeclaration(const TypeUsage& pReturnType, const Identifier& pFunctionIdentifier)
         : mReturnType(pReturnType)
         , mFunctionIdentifier(pFunctionIdentifier)
         , mBody(nullptr)
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
      }
   };

   struct StatementAssignment : Statement
   {
      Expression* mLeftValue;
      Expression* mRightValue;
      char mOperator[4];

      StatementAssignment(Expression* pLeftValue, Expression* pRightValue, const char* pOperator)
         : mLeftValue(pLeftValue)
         , mRightValue(pRightValue)
      {
         mType = StatementType::Assignment;
         strcpy(mOperator, pOperator);
      }

      virtual ~StatementAssignment()
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
      Statement* mIncrement;
      Statement* mLoopStatement;

      StatementFor(Statement* pInitialization, Expression* pCondition, Statement* pIncrement,
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
            CflatInvokeDtor(Statement, mIncrement);
            CflatFree(mIncrement);
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


   const char* kCompileErrorStrings[] = 
   {
      "unexpected symbol after '%s'",
      "undefined variable ('%s')",
      "undefined function ('%s')",
      "variable redefinition ('%s')",
      "array initialization expected",
      "no default constructor defined for the '%s' type",
      "invalid member access operator ('%s' is a pointer)",
      "invalid member access operator ('%s' is not a pointer)",
      "invalid operator for the '%s' type",
      "invalid conditional expression",
      "no member named '%s'",
      "'%s' must be an integer value",
      "unknown namespace ('%s')"
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
//  Program
//
using namespace Cflat;

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
Namespace::Namespace(const Identifier& pIdentifier, Namespace* pParent)
   : mName(pIdentifier)
   , mFullName(pIdentifier)
   , mParent(pParent)
{
   if(pParent && pParent->getParent())
   {
      char buffer[256];
      sprintf(buffer, "%s::%s", mParent->mFullName.mName, mName.mName);
      mFullName = Identifier(buffer);
   }

   mInstances.reserve(kMaxInstances);
}

Namespace::~Namespace()
{
   releaseInstances(0u);

   for(NamespacesRegistry::iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      Namespace* ns = it->second;
      CflatInvokeDtor(Namespace, ns);
      CflatFree(ns);
   }

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

   for(TypesRegistry::iterator it = mTypes.begin(); it != mTypes.end(); it++)
   {
      Type* type = it->second;
      CflatInvokeDtor(Type, type);
      CflatFree(type);
   }
}

Namespace* Namespace::getChild(uint32_t pNameHash)
{
   NamespacesRegistry::const_iterator it = mNamespaces.find(pNameHash);
   return it != mNamespaces.end() ? it->second : nullptr;
}

const char* Namespace::findFirstSeparator(const char* pString)
{
   const size_t length = strlen(pString);

   for(size_t i = 1u; i < (length - 1u); i++)
   {
      if(pString[i] == ':' && pString[i + 1u] == ':')
         return (pString + i);
   }

   return nullptr;
}

const char* Namespace::findLastSeparator(const char* pString)
{
   const size_t length = strlen(pString);

   for(size_t i = (length - 1u); i > 1u; i--)
   {
      if(pString[i] == ':' && pString[i - 1u] == ':')
         return (pString + i - 1);
   }

   return nullptr;
}

Namespace* Namespace::getNamespace(const Identifier& pName)
{
   const char* separator = findFirstSeparator(pName.mName);

   if(separator)
   {
      char buffer[256];
      const size_t childIdentifierLength = separator - pName.mName;
      strncpy(buffer, pName.mName, childIdentifierLength);
      buffer[childIdentifierLength] = '\0';

      const uint32_t childNameHash = hash(buffer);
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
   const char* separator = findFirstSeparator(pName.mName);

   if(separator)
   {
      char buffer[256];
      const size_t childIdentifierLength = separator - pName.mName;
      strncpy(buffer, pName.mName, childIdentifierLength);
      buffer[childIdentifierLength] = '\0';

      const Identifier childIdentifier(buffer);
      Namespace* child = getChild(childIdentifier.mHash);

      if(!child)
      {
         child = (Namespace*)CflatMalloc(sizeof(Namespace));
         CflatInvokeCtor(Namespace, child)(childIdentifier, this);
         mNamespaces[childIdentifier.mHash] = child;
      }

      const Identifier subIdentifier(separator + 2);
      return child->requestNamespace(subIdentifier);
   }

   Namespace* child = getChild(pName.mHash);

   if(!child)
   {
      child = (Namespace*)CflatMalloc(sizeof(Namespace));
      CflatInvokeCtor(Namespace, child)(pName, this);
      mNamespaces[pName.mHash] = child;
   }

   return child;
}

Type* Namespace::getType(const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier typeIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);
      return ns ? ns->getType(typeIdentifier) : nullptr;
   }

   TypesRegistry::const_iterator it = mTypes.find(pIdentifier.mHash);

   if(it != mTypes.end())
   {
      return it->second;
   }

   if(mParent)
   {
      return mParent->getType(pIdentifier);
   }

   return nullptr;
}

Type* Namespace::getType(const Identifier& pIdentifier, const CflatSTLVector(TypeUsage)& pTemplateTypes)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier typeIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);
      return ns ? ns->getType(typeIdentifier, pTemplateTypes) : nullptr;
   }

   uint32_t hash = pIdentifier.mHash;

   for(size_t i = 0u; i < pTemplateTypes.size(); i++)
   {
      hash += pTemplateTypes[i].mType->getHash();
      hash += (uint32_t)pTemplateTypes[i].mPointerLevel;
   }

   TypesRegistry::const_iterator it = mTypes.find(hash);

   if(it != mTypes.end())
   {
      return it->second;
   }

   if(mParent)
   {
      return mParent->getType(pIdentifier, pTemplateTypes);
   }

   return nullptr;
}

TypeUsage Namespace::getTypeUsage(const char* pTypeName)
{
   TypeUsage typeUsage;

   const size_t typeNameLength = strlen(pTypeName);
   const char* baseTypeNameStart = pTypeName;
   const char* baseTypeNameEnd = pTypeName + typeNameLength - 1u;

   // is it const?
   const char* typeNameConst = strstr(pTypeName, "const");

   if(typeNameConst)
   {
      CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Const);
      baseTypeNameStart = typeNameConst + 6u;
   }

   // is it a pointer?
   const char* typeNamePtr = strchr(baseTypeNameStart, '*');

   if(typeNamePtr)
   {
      typeUsage.mPointerLevel++;
      baseTypeNameEnd = typeNamePtr - 1u;
   }
   else
   {
      // is it a reference?
      const char* typeNameRef = strchr(baseTypeNameStart, '&');

      if(typeNameRef)
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
         baseTypeNameEnd = typeNameRef - 1u;
      }
   }

   // remove empty spaces
   while(baseTypeNameStart[0] == ' ')
   {
      baseTypeNameStart++;
   }

   while(baseTypeNameEnd[0] == ' ')
   {
      baseTypeNameEnd--;
   }

   // assign the type
   char baseTypeName[32];
   size_t baseTypeNameLength = baseTypeNameEnd - baseTypeNameStart + 1u;
   strncpy(baseTypeName, baseTypeNameStart, baseTypeNameLength);
   baseTypeName[baseTypeNameLength] = '\0';

   typeUsage.mType = getType(baseTypeName);

   return typeUsage;
}

Function* Namespace::getFunction(const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);
      return ns ? ns->getFunction(functionIdentifier) : nullptr;
   }

   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? it->second.at(0) : nullptr;
}

Function* Namespace::getFunction(const Identifier& pIdentifier,
   const CflatSTLVector(TypeUsage)& pParameterTypes)
{
   Function* function = nullptr;
   CflatSTLVector(Function*)* functions = getFunctions(pIdentifier);

   if(functions)
   {
      for(size_t i = 0u; i < functions->size(); i++)
      {
         Function* functionOverload = functions->at(i);

         if(functionOverload->mParameters.size() == pParameterTypes.size())
         {
            bool parametersMatch = true;

            for(size_t j = 0u; j < pParameterTypes.size(); j++)
            {
               if(!functionOverload->mParameters[j].compatibleWith(pParameterTypes[j]))
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

   return function;
}

Function* Namespace::getFunction(const Identifier& pIdentifier, const CflatSTLVector(Value)& pArguments)
{
   CflatSTLVector(TypeUsage) typeUsages;
   typeUsages.reserve(pArguments.size());

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return getFunction(pIdentifier, typeUsages);
}

CflatSTLVector(Function*)* Namespace::getFunctions(const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);
      return ns ? ns->getFunctions(functionIdentifier) : nullptr;
   }

   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? &it->second : nullptr;
}

Function* Namespace::registerFunction(const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier functionIdentifier(lastSeparator + 2);

      Namespace* ns = requestNamespace(nsIdentifier);
      return ns->registerFunction(functionIdentifier);
   }

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

void Namespace::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue)
{
   Instance* instance = retrieveInstance(pIdentifier);

   if(!instance)
   {
      instance = registerInstance(pTypeUsage, pIdentifier);
   }

   instance->mValue.initOnHeap(pTypeUsage);
   instance->mValue.set(pValue.mValueBuffer);
}

Value* Namespace::getVariable(const Identifier& pIdentifier)
{
   Instance* instance = retrieveInstance(pIdentifier);
   return instance ? &instance->mValue : nullptr;
}

Instance* Namespace::registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier instanceIdentifier(lastSeparator + 2);

      Namespace* ns = requestNamespace(nsIdentifier);
      return ns->registerInstance(pTypeUsage, instanceIdentifier);
   }

   CflatAssert(mInstances.size() < kMaxInstances);

   mInstances.emplace_back(pTypeUsage, pIdentifier);
   return &mInstances.back();
}

Instance* Namespace::retrieveInstance(const Identifier& pIdentifier)
{
   const char* lastSeparator = findLastSeparator(pIdentifier.mName);

   if(lastSeparator)
   {
      char buffer[256];
      const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
      strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
      buffer[nsIdentifierLength] = '\0';
      const Identifier nsIdentifier(buffer);
      const Identifier instanceIdentifier(lastSeparator + 2);

      Namespace* ns = getNamespace(nsIdentifier);
      return ns ? ns->retrieveInstance(instanceIdentifier) : nullptr;
   }

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

void Namespace::releaseInstances(uint32_t pScopeLevel)
{
   while(!mInstances.empty() && mInstances.back().mScopeLevel >= pScopeLevel)
   {
      mInstances.pop_back();
   }

   for(NamespacesRegistry::iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      Namespace* ns = it->second;
      ns->releaseInstances(pScopeLevel);
   }
}



//
//  Environment
//
const char* kCflatPunctuation[] = 
{
   ".", ",", ":", ";", "->", "(", ")", "{", "}", "[", "]", "::"
};
const size_t kCflatPunctuationCount = sizeof(kCflatPunctuation) / sizeof(const char*);

const char* kCflatOperators[] = 
{
   "+", "-", "*", "/",
   "++", "--", "!",
   "=", "+=", "-=", "*=", "/=",
   "<<", ">>",
   "==", "!=", ">", "<", ">=", "<=",
   "&&", "||", "&", "|", "~", "^"
};
const size_t kCflatOperatorsCount = sizeof(kCflatOperators) / sizeof(const char*);

const char* kCflatAssignmentOperators[] = 
{
   "=", "+=", "-=", "*=", "/="
};
const size_t kCflatAssignmentOperatorsCount = sizeof(kCflatAssignmentOperators) / sizeof(const char*);

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

Environment::Environment()
   : mGlobalNamespace("", nullptr)
   , mExecutionContext(&mGlobalNamespace)
{
   static_assert(kCompileErrorStringsCount == (size_t)Environment::CompileError::Count,
      "Missing compile error strings");
   static_assert(kRuntimeErrorStringsCount == (size_t)Environment::RuntimeError::Count,
      "Missing runtime error strings");

   mAutoType = registerType<BuiltInType>("auto");

   registerBuiltInTypes();
   registerStdout();
}

Environment::~Environment()
{
}

void Environment::registerBuiltInTypes()
{
   CflatRegisterBuiltInType(this, int);
   CflatRegisterBuiltInType(this, uint32_t);
   CflatRegisterBuiltInType(this, size_t);
   CflatRegisterBuiltInType(this, char);
   CflatRegisterBuiltInType(this, bool);
   CflatRegisterBuiltInType(this, uint8_t);
   CflatRegisterBuiltInType(this, short);
   CflatRegisterBuiltInType(this, uint16_t);
   CflatRegisterBuiltInType(this, float);
   CflatRegisterBuiltInType(this, double);
}

void Environment::registerStdout()
{
  {
    CflatRegisterClass(this, std::ostream);
    CflatClassAddMethodReturnParams1(this, std::ostream, std::ostream,&, operator<<, int,);
    CflatClassAddMethodReturnParams1(this, std::ostream, std::ostream,&, operator<<, float,);
  }

  CflatRegisterFunctionReturnParams2(this, std::ostream,&, operator<<, std::ostream,&, const char*,);

  Cflat::TypeUsage coutTypeUsage = getTypeUsage("std::ostream");
  Cflat::Value coutValue;
  coutValue.initOnStack(coutTypeUsage, &mExecutionContext.mStack);
  coutValue.set(&std::cout);
  setVariable(coutTypeUsage, "std::cout", coutValue);
}

TypeUsage Environment::parseTypeUsage(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   size_t cachedTokenIndex = tokenIndex;

   TypeUsage typeUsage;
   char baseTypeName[128];

   pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);

   while(tokens[tokenIndex + 1u].mLength == 2u && strncmp(tokens[tokenIndex + 1u].mStart, "::", 2u) == 0)
   {
      tokenIndex += 2u;
      pContext.mStringBuffer.append("::");
      pContext.mStringBuffer.append(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
   }

   strcpy(baseTypeName, pContext.mStringBuffer.c_str());
   const Identifier baseTypeIdentifier(baseTypeName);
   CflatSTLVector(TypeUsage) templateTypes;

   if(tokens[tokenIndex + 1u].mStart[0] == '<')
   {
      tokenIndex += 2u;
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, '<', '>');

      while(tokenIndex < closureTokenIndex)
      {
         templateTypes.push_back(parseTypeUsage(pContext));
         tokenIndex++;
      }
   }
   
   Type* type = getType(baseTypeIdentifier, templateTypes);   

   if(!type)
   {
      for(size_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
      {
         type = pContext.mUsingDirectives[i].mNamespace->getType(baseTypeIdentifier, templateTypes);

         if(type)
            break;
      }
   }

   if(type)
   {
      typeUsage.mType = type;

      if(tokenIndex > 0u && strncmp(tokens[tokenIndex - 1u].mStart, "const", 5u) == 0)
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Const);
      }

      if(*tokens[tokenIndex + 1u].mStart == '&')
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
         tokenIndex++;
      }

      while(*tokens[tokenIndex + 1u].mStart == '*')
      {
         typeUsage.mPointerLevel++;
         tokenIndex++;
      }
   }
   else
   {
      tokenIndex = cachedTokenIndex;
   }

   return typeUsage;
}

void Environment::throwCompileError(ParsingContext& pContext, CompileError pError,
   const char* pArg1, const char* pArg2)
{
   const Token& token = pContext.mTokens[pContext.mTokenIndex];

   char errorMsg[256];
   sprintf(errorMsg, kCompileErrorStrings[(int)pError], pArg1, pArg2);

   char lineAsString[16];
   sprintf(lineAsString, "%d", token.mLine);

   pContext.mErrorMessage.assign("[Compile Error] Line ");
   pContext.mErrorMessage.append(lineAsString);
   pContext.mErrorMessage.append(": ");
   pContext.mErrorMessage.append(errorMsg);
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

         continue;
      }
      // preprocessor directive
      else if(pCode[cursor] == '#')
      {
         //TODO
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

      // add the current character to the preprocessed code
      if(pCode[cursor] != '\\')
      {
         preprocessedCode.push_back(pCode[cursor]);
      }
      else
      {
         cursor++;

         switch(pCode[cursor])
         {
         case 'n':
            preprocessedCode.push_back('\n');
            break;
         default:
            preprocessedCode.push_back('\\');
         }
      }

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
   CflatSTLVector(Token)& tokens = pContext.mTokens;

   char* cursor = const_cast<char*>(pContext.mPreprocessedCode.c_str());
   uint16_t currentLine = 1u;

   tokens.clear();

   while(*cursor != '\0')
   {
      while(*cursor == ' ' || *cursor == '\n')
      {
         if(*cursor == '\n')
            currentLine++;

         cursor++;
      }

      if(*cursor == '\0')
         return;

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
         tokens.push_back(token);
         continue;
      }

      // numeric value
      if(isdigit(*cursor))
      {
         do
         {
            cursor++;
         }
         while(isdigit(*cursor) || *cursor == '.' || *cursor == 'f' || *cursor == 'x' || *cursor == 'u');

         token.mLength = cursor - token.mStart;
         token.mType = TokenType::Number;
         tokens.push_back(token);
         continue;
      }

      // punctuation (2 characters)
      const size_t tokensCount = tokens.size();

      for(size_t i = 0u; i < kCflatPunctuationCount; i++)
      {
         if(strncmp(token.mStart, kCflatPunctuation[i], 2u) == 0)
         {
            cursor += 2;
            token.mLength = cursor - token.mStart;
            token.mType = TokenType::Punctuation;
            tokens.push_back(token);
            break;
         }
      }

      if(tokens.size() > tokensCount)
         continue;

      // operator (2 characters)
      for(size_t i = 0u; i < kCflatOperatorsCount; i++)
      {
         if(strncmp(token.mStart, kCflatOperators[i], 2u) == 0)
         {
            cursor += 2;
            token.mLength = cursor - token.mStart;
            token.mType = TokenType::Operator;
            tokens.push_back(token);
            break;
         }
      }

      if(tokens.size() > tokensCount)
         continue;

      // punctuation (1 character)
      for(size_t i = 0u; i < kCflatPunctuationCount; i++)
      {
         if(token.mStart[0] == kCflatPunctuation[i][0] && kCflatPunctuation[i][1] == '\0')
         {
            cursor++;
            token.mType = TokenType::Punctuation;
            tokens.push_back(token);
            break;
         }
      }

      if(tokens.size() > tokensCount)
         continue;

      // operator (1 character)
      for(size_t i = 0u; i < kCflatOperatorsCount; i++)
      {
         if(token.mStart[0] == kCflatOperators[i][0])
         {
            cursor++;
            token.mType = TokenType::Operator;
            tokens.push_back(token);
            break;
         }
      }

      if(tokens.size() > tokensCount)
         continue;

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
            tokens.push_back(token);
            break;
         }
      }

      if(tokens.size() > tokensCount)
         continue;

      // identifier
      do
      {
         cursor++;
      }
      while(isalnum(*cursor) || *cursor == '_');

      token.mLength = cursor - token.mStart;
      token.mType = TokenType::Identifier;
      tokens.push_back(token);
   }
}

void Environment::parse(ParsingContext& pContext, Program& pProgram)
{
   size_t& tokenIndex = pContext.mTokenIndex;

   for(tokenIndex = 0u; tokenIndex < pContext.mTokens.size(); tokenIndex++)
   {
      Statement* statement = parseStatement(pContext);

      if(statement)
      {
         pProgram.mStatements.push_back(statement);
      }

      if(!pContext.mErrorMessage.empty())
         break;
   }
}

Expression* Environment::parseExpression(ParsingContext& pContext, size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Expression* expression = nullptr;

   const size_t tokensCount = pTokenLastIndex - pContext.mTokenIndex + 1u;

   if(tokensCount == 1u)
   {
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
               typeUsage.mType = getType("float");
               const float number = (float)strtod(numberStr, nullptr);
               value.initOnStack(typeUsage, &mExecutionContext.mStack);
               value.set(&number);
            }
            // double
            else
            {
               typeUsage.mType = getType("double");
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
               typeUsage.mType = getType("uint32_t");
               const uint32_t number = (uint32_t)atoi(numberStr);
               value.initOnStack(typeUsage, &mExecutionContext.mStack);
               value.set(&number);
            }
            // signed
            else
            {
               typeUsage.mType = getType("int");
               const int number = atoi(numberStr);
               value.initOnStack(typeUsage, &mExecutionContext.mStack);
               value.set(&number);
            }
         }

         expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
         CflatInvokeCtor(ExpressionValue, expression)(value);
      }
      else if(token.mType == TokenType::String)
      {
         pContext.mStringBuffer.assign(token.mStart + 1, token.mLength - 1u);
         pContext.mStringBuffer[token.mLength - 2u] = '\0';

         const uint32_t stringHash = hash(pContext.mStringBuffer.c_str());
         const char* string =
            mLiteralStringsPool.registerString(stringHash, pContext.mStringBuffer.c_str());

         TypeUsage typeUsage = getTypeUsage("const char*");
         Value value;
         value.initOnStack(typeUsage, &mExecutionContext.mStack);
         value.set(&string);

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
            value.initOnStack(getTypeUsage("bool"), &mExecutionContext.mStack);

            const bool boolValue = true;
            value.set(&boolValue);

            expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
            CflatInvokeCtor(ExpressionValue, expression)(value);
         }
         else if(strncmp(token.mStart, "false", 5u) == 0)
         {
            Value value;
            value.initOnStack(getTypeUsage("bool"), &mExecutionContext.mStack);

            const bool boolValue = false;
            value.set(&boolValue);

            expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
            CflatInvokeCtor(ExpressionValue, expression)(value);
         }
      }
   }
   else if(token.mType == TokenType::Operator)
   {
      // address of
      if(token.mStart[0] == '&')
      {
         tokenIndex++;
         Expression* addressOfExpression = parseImmediateExpression(pContext, pTokenLastIndex);

         expression = (ExpressionAddressOf*)CflatMalloc(sizeof(ExpressionAddressOf));
         CflatInvokeCtor(ExpressionAddressOf, expression)(addressOfExpression);
      }
      // unary operator (pre)
      else
      {
         const Token& operatorToken = token;
         CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);
         tokenIndex++;      

         expression = (ExpressionUnaryOperation*)CflatMalloc(sizeof(ExpressionUnaryOperation));
         CflatInvokeCtor(ExpressionUnaryOperation, expression)
            (parseExpression(pContext, pTokenLastIndex), operatorStr.c_str(), false);
      }
   }
   else if(tokens[pTokenLastIndex].mType == TokenType::Operator)   
   {
      // unary operator (post)
      const Token& operatorToken = tokens[pTokenLastIndex];
      CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

      expression = (ExpressionUnaryOperation*)CflatMalloc(sizeof(ExpressionUnaryOperation));
      CflatInvokeCtor(ExpressionUnaryOperation, expression)
         (parseExpression(pContext, pTokenLastIndex - 1u), operatorStr.c_str(), true);
   }
   else if(tokens[pTokenLastIndex].mStart[0] == ']')
   {
      // array element access
      const size_t openingIndex = findOpeningTokenIndex(pContext, '[', ']', pTokenLastIndex);
      Expression* arrayAccess = parseExpression(pContext, openingIndex - 1u);

      tokenIndex = openingIndex + 1u;
      Expression* arrayElementIndex = parseExpression(pContext, pTokenLastIndex - 1u);

      expression = (ExpressionArrayElementAccess*)CflatMalloc(sizeof(ExpressionArrayElementAccess));
      CflatInvokeCtor(ExpressionArrayElementAccess, expression)(arrayAccess, arrayElementIndex);

      tokenIndex = pTokenLastIndex + 1u;
   }
   else
   {
      const size_t conditionalTokenIndex = findClosureTokenIndex(pContext, '?');

      // conditional expression
      if(conditionalTokenIndex && conditionalTokenIndex < pTokenLastIndex)
      {
         Expression* condition = parseExpression(pContext, conditionalTokenIndex - 1u);
         tokenIndex = conditionalTokenIndex + 1u;

         const size_t elseTokenIndex = findClosureTokenIndex(pContext, ':');

         if(elseTokenIndex && elseTokenIndex < pTokenLastIndex)
         {
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
      else
      {
         size_t operatorTokenIndex = 0u;
         uint32_t parenthesisLevel = tokens[pTokenLastIndex].mStart[0] == ')' ? 1u : 0u;

         for(size_t i = pTokenLastIndex - 1u; i > tokenIndex; i--)
         {
            if(tokens[i].mType == TokenType::Operator && parenthesisLevel == 0u)
            {
               operatorTokenIndex = i;
               break;
            }

            if(tokens[i].mStart[0] == ')')
            {
               parenthesisLevel++;
            }
            else if(tokens[i].mStart[0] == '(')
            {
               parenthesisLevel--;
            }
         }

         // binary operator
         if(operatorTokenIndex > 0u)
         {
            Expression* left = parseExpression(pContext, operatorTokenIndex - 1u);
            TypeUsage typeUsage = getTypeUsage(pContext, left);

            const Token& operatorToken = pContext.mTokens[operatorTokenIndex];
            CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

            bool operatorIsValid = true;

            if(typeUsage.mType->mCategory == TypeCategory::StructOrClass)
            {
               pContext.mStringBuffer.assign("operator");
               pContext.mStringBuffer.append(operatorStr);
               Method* operatorMethod = findMethod(typeUsage.mType, pContext.mStringBuffer.c_str());

               if(!operatorMethod)
               {
                  const char* typeName = typeUsage.mType->mIdentifier.mName;
                  throwCompileError(pContext, CompileError::InvalidOperator, typeName, operatorStr.c_str());
                  operatorIsValid = false;
               }
            }

            if(operatorIsValid)
            {
               tokenIndex = operatorTokenIndex + 1u;
               Expression* right = parseExpression(pContext, pTokenLastIndex);

               expression = (ExpressionBinaryOperation*)CflatMalloc(sizeof(ExpressionBinaryOperation));
               CflatInvokeCtor(ExpressionBinaryOperation, expression)(left, right, operatorStr.c_str());
            }
         }
         // parenthesized expression
         else if(tokens[tokenIndex].mStart[0] == '(')
         {
            const size_t closureTokenIndex = findClosureTokenIndex(pContext, '(', ')');
            tokenIndex++;

            expression = (ExpressionParenthesized*)CflatMalloc(sizeof(ExpressionParenthesized));
            CflatInvokeCtor(ExpressionParenthesized, expression)(parseExpression(pContext, closureTokenIndex - 1u));
            tokenIndex = closureTokenIndex + 1u;
         }
         // array initialization
         else if(tokens[tokenIndex].mStart[0] == '{')
         {
            tokenIndex++;

            ExpressionArrayInitialization* castedExpression = 
               (ExpressionArrayInitialization*)CflatMalloc(sizeof(ExpressionArrayInitialization));
            CflatInvokeCtor(ExpressionArrayInitialization, castedExpression)();
            expression = castedExpression;

            const size_t closureIndex = findClosureTokenIndex(pContext, '{', '}');

            while(tokenIndex < closureIndex)
            {
               const size_t separatorIndex = findClosureTokenIndex(pContext, ',');
               const size_t lastArrayValueIndex = separatorIndex > 0u
                  ? separatorIndex - 1u
                  : closureIndex - 1u;

               Expression* arrayValueExpression = parseExpression(pContext, lastArrayValueIndex);
               castedExpression->mValues.push_back(arrayValueExpression);

               tokenIndex = lastArrayValueIndex + 2u;
            }
         }
         else if(token.mType == TokenType::Identifier)
         {
            const Token& nextToken = tokens[tokenIndex + 1u];

            // function call/object construction
            if(nextToken.mStart[0] == '(')
            {
               pContext.mStringBuffer.assign(token.mStart, token.mLength);
               Identifier identifier(pContext.mStringBuffer.c_str());
               Type* type = getType(identifier);
               
               if(type)
               {
                  ExpressionObjectConstruction* castedExpression = 
                     (ExpressionObjectConstruction*)CflatMalloc(sizeof(ExpressionObjectConstruction));
                  CflatInvokeCtor(ExpressionObjectConstruction, castedExpression)(type);
                  expression = castedExpression;

                  tokenIndex++;
                  parseFunctionCallArguments(pContext, castedExpression->mArguments);
               }
               else
               {
                  ExpressionFunctionCall* castedExpression = 
                     (ExpressionFunctionCall*)CflatMalloc(sizeof(ExpressionFunctionCall));
                  CflatInvokeCtor(ExpressionFunctionCall, castedExpression)(identifier);
                  expression = castedExpression;

                  tokenIndex++;
                  parseFunctionCallArguments(pContext, castedExpression->mArguments);
               }
            }
            // member access
            else if(nextToken.mStart[0] == '.' || strncmp(nextToken.mStart, "->", 2u) == 0)
            {
               ExpressionMemberAccess* memberAccess =
                  (ExpressionMemberAccess*)CflatMalloc(sizeof(ExpressionMemberAccess));
               CflatInvokeCtor(ExpressionMemberAccess, memberAccess)();
               expression = memberAccess;

               parseMemberAccessIdentifiers(pContext, memberAccess->mIdentifiers);

               // method call
               if(tokens[tokenIndex].mStart[0] == '(')
               {
                  ExpressionMethodCall* methodCall = 
                     (ExpressionMethodCall*)CflatMalloc(sizeof(ExpressionMethodCall));
                  CflatInvokeCtor(ExpressionMethodCall, methodCall)(memberAccess);
                  expression = methodCall;

                  parseFunctionCallArguments(pContext, methodCall->mArguments);
               }
            }
            // static member access
            else if(strncmp(nextToken.mStart, "::", 2u) == 0)
            {
               pContext.mStringBuffer.assign(token.mStart, token.mLength);

               while(strncmp(tokens[++tokenIndex].mStart, "::", 2u) == 0)
               {
                  tokenIndex++;
                  pContext.mStringBuffer.append("::");
                  pContext.mStringBuffer.append(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
               }

               const Identifier staticMemberIdentifier(pContext.mStringBuffer.c_str());

               // static method call
               if(tokens[tokenIndex].mStart[0] == '(')
               {
                  Identifier identifier(pContext.mStringBuffer.c_str());

                  ExpressionFunctionCall* castedExpression = 
                     (ExpressionFunctionCall*)CflatMalloc(sizeof(ExpressionFunctionCall));
                  CflatInvokeCtor(ExpressionFunctionCall, castedExpression)(identifier);
                  expression = castedExpression;

                  parseFunctionCallArguments(pContext, castedExpression->mArguments);
               }
               // static member access
               else
               {
                  expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
                  CflatInvokeCtor(ExpressionVariableAccess, expression)(staticMemberIdentifier);
               }
            }
         }
      }
   }

   return expression;
}

Expression* Environment::parseImmediateExpression(ParsingContext& pContext, size_t pTokenLastIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   size_t lastTokenIndex = Cflat::min(pTokenLastIndex, findClosureTokenIndex(pContext, ';') - 1u);

   for(size_t i = (tokenIndex + 1u); i < lastTokenIndex; i++)
   {
      if(tokens[i].mType == TokenType::Operator)
      {
         lastTokenIndex = i - 1u;
         break;
      }
   }

   Expression* expression = parseExpression(pContext, lastTokenIndex);
   tokenIndex = lastTokenIndex;

   return expression;
}

size_t Environment::findClosureTokenIndex(ParsingContext& pContext, char pClosureChar)
{
   return findClosureTokenIndex(pContext, ' ', pClosureChar);
}

size_t Environment::findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t closureTokenIndex = 0u;

   if(tokens[pContext.mTokenIndex].mStart[0] == pClosureChar)
   {
      closureTokenIndex = pContext.mTokenIndex;
   }
   else
   {
      uint32_t scopeLevel = 0u;

      for(size_t i = (pContext.mTokenIndex + 1u); i < pContext.mTokens.size(); i++)
      {
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

size_t Environment::findOpeningTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar, size_t pClosureIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t openingTokenIndex = pClosureIndex;

   if(openingTokenIndex > 0u)
   {
      uint32_t scopeLevel = 0u;

      for(int i = (int)pClosureIndex - 1; i >= (int)pContext.mTokenIndex; i--)
      {
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

TypeUsage Environment::getTypeUsage(ParsingContext& pContext, Expression* pExpression)
{
   TypeUsage typeUsage;

   switch(pExpression->getType())
   {
   case ExpressionType::Value:
      {
         ExpressionValue* expression = static_cast<ExpressionValue*>(pExpression);
         typeUsage = expression->mValue.mTypeUsage;
      }
      break;
   case ExpressionType::VariableAccess:
      {
         ExpressionVariableAccess* expression = static_cast<ExpressionVariableAccess*>(pExpression);
         Instance* instance = retrieveInstance(pContext, expression->mVariableIdentifier);
         typeUsage = instance->mTypeUsage;
      }
      break;
   case ExpressionType::BinaryOperation:
      {
         ExpressionBinaryOperation* expression = static_cast<ExpressionBinaryOperation*>(pExpression);
         typeUsage = getTypeUsage(pContext, expression->mLeft);
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
   case ExpressionType::FunctionCall:
      {
         ExpressionFunctionCall* expression = static_cast<ExpressionFunctionCall*>(pExpression);
         Function* function = getFunction(expression->mFunctionIdentifier);
         typeUsage = function->mReturnTypeUsage;
      }
      break;
   default:
      break;
   }

   return typeUsage;
}

Statement* Environment::parseStatement(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   Statement* statement = nullptr;
   const uint16_t statementLine = token.mLine;

   switch(token.mType)
   {
      case TokenType::Punctuation:
      {
         // block
         if(token.mStart[0] == '{')
         {
            statement = parseStatementBlock(pContext);
         }
      }
      break;

      case TokenType::Keyword:
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
         // function declaration
         else if(strncmp(token.mStart, "void", 4u) == 0)
         {
            tokenIndex++;
            statement = parseStatementFunctionDeclaration(pContext);
         }
         // return
         else if(strncmp(token.mStart, "return", 6u) == 0)
         {
            tokenIndex++;
            statement = parseStatementReturn(pContext);
         }
         // usign namespace
         else if(strncmp(token.mStart, "using", 5u) == 0)
         {
            tokenIndex++;
            statement = parseStatementUsingDirective(pContext);
         }
         // namespace
         else if(strncmp(token.mStart, "namespace", 9u) == 0)
         {
            tokenIndex++;
            statement = parseStatementNamespaceDeclaration(pContext);
         }
      }
      break;

      case TokenType::Identifier:
      {
         // type
         TypeUsage typeUsage = parseTypeUsage(pContext);

         if(typeUsage.mType)
         {
            tokenIndex++;
            const Token& identifierToken = tokens[tokenIndex];
            pContext.mStringBuffer.assign(identifierToken.mStart, identifierToken.mLength);
            const Identifier identifier(pContext.mStringBuffer.c_str());

            tokenIndex++;
            const Token& nextToken = tokens[tokenIndex];

            if(nextToken.mType != TokenType::Operator && nextToken.mType != TokenType::Punctuation)
            {
               pContext.mStringBuffer.assign(token.mStart, token.mLength);
               throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
               return nullptr;
            }

            bool isFunctionDeclaration = nextToken.mStart[0] == '(';

            if(isFunctionDeclaration)
            {
               const size_t nextSemicolonIndex = findClosureTokenIndex(pContext, ';');
               const size_t nextBracketIndex = findClosureTokenIndex(pContext, '{');
               
               if(nextBracketIndex == 0u || nextSemicolonIndex < nextBracketIndex)
               {
                  // object construction
                  isFunctionDeclaration = false;
               }
            }

            // function declaration
            if(isFunctionDeclaration)
            {
               tokenIndex--;
               statement = parseStatementFunctionDeclaration(pContext);
            }
            // variable / const declaration
            else
            {
               statement = parseStatementVariableDeclaration(pContext, typeUsage, identifier);
            }

            break;
         }
         // assignment / variable access / function call
         else
         {
            size_t cursor = tokenIndex;
            size_t operatorTokenIndex = 0u;
            uint32_t parenthesisLevel = 0u;

            while(cursor < tokens.size() && tokens[cursor++].mStart[0] != ';')
            {
               if(tokens[cursor].mType == TokenType::Operator && parenthesisLevel == 0u)
               {
                  bool isAssignmentOperator = false;

                  for(size_t i = 0u; i < kCflatAssignmentOperatorsCount; i++)
                  {
                     const size_t operatorStrLength = strlen(kCflatAssignmentOperators[i]);

                     if(tokens[cursor].mLength == operatorStrLength &&
                        strncmp(tokens[cursor].mStart, kCflatAssignmentOperators[i], operatorStrLength) == 0)
                     {
                        isAssignmentOperator = true;
                        break;
                     }
                  }

                  if(isAssignmentOperator)
                  {
                     operatorTokenIndex = cursor;
                     break;
                  }
               }

               if(tokens[cursor].mStart[0] == '(')
               {
                  parenthesisLevel++;
               }
               else if(tokens[cursor].mStart[0] == ')')
               {
                  parenthesisLevel--;
               }
            }

            // assignment
            if(operatorTokenIndex > 0u)
            {
               statement = parseStatementAssignment(pContext, operatorTokenIndex);
            }
            else
            {
               const Token& nextToken = tokens[tokenIndex + 1u];

               if(nextToken.mType == TokenType::Punctuation)
               {
                  // function call
                  if(nextToken.mStart[0] == '(')
                  {
                     pContext.mStringBuffer.assign(token.mStart, token.mLength);
                     Identifier identifier(pContext.mStringBuffer.c_str());

                     ExpressionFunctionCall* expression = 
                        (ExpressionFunctionCall*)CflatMalloc(sizeof(ExpressionFunctionCall));
                     CflatInvokeCtor(ExpressionFunctionCall, expression)(identifier);

                     tokenIndex++;
                     parseFunctionCallArguments(pContext, expression->mArguments);

                     statement = (StatementExpression*)CflatMalloc(sizeof(StatementExpression));
                     CflatInvokeCtor(StatementExpression, statement)(expression);
                  }
                  // member access
                  else
                  {
                     Expression* memberAccess =
                        parseExpression(pContext, findClosureTokenIndex(pContext, ';') - 1u);

                     if(memberAccess)
                     {
                        // method call
                        if(tokens[tokenIndex].mStart[0] == '(')
                        {
                           ExpressionMethodCall* expression = 
                              (ExpressionMethodCall*)CflatMalloc(sizeof(ExpressionMethodCall));
                           CflatInvokeCtor(ExpressionMethodCall, expression)(memberAccess);

                           parseFunctionCallArguments(pContext, expression->mArguments);

                           statement = (StatementExpression*)CflatMalloc(sizeof(StatementExpression));
                           CflatInvokeCtor(StatementExpression, statement)(expression);
                        }
                        // static method call
                        else
                        {
                           statement = (StatementExpression*)CflatMalloc(sizeof(StatementExpression));
                           CflatInvokeCtor(StatementExpression, statement)(memberAccess);
                        }
                     }
                  }
               }
               else if(nextToken.mType == TokenType::Operator)
               {
                  pContext.mStringBuffer.assign(token.mStart, token.mLength);
                  const char* variableName = pContext.mStringBuffer.c_str();
                  Instance* instance = retrieveInstance(pContext, variableName);

                  if(instance)
                  {
                     // increment / decrement
                     if(strncmp(nextToken.mStart, "++", 2u) == 0 ||
                        strncmp(nextToken.mStart, "--", 2u) == 0)
                     {
                        if(instance->mTypeUsage.mType->isInteger())
                        {
                           Expression* expression = parseExpression(pContext, tokenIndex + 1u);
                           statement = (StatementExpression*)CflatMalloc(sizeof(StatementExpression));
                           CflatInvokeCtor(StatementExpression, statement)(expression);
                           tokenIndex += 2u;
                        }
                        else
                        {
                           throwCompileError(pContext, CompileError::NonIntegerValue, variableName);
                        }
                     }
                  }
                  else
                  {
                     pContext.mStringBuffer.assign(token.mStart, token.mLength);
                     throwCompileError(pContext, CompileError::UndefinedVariable, pContext.mStringBuffer.c_str());
                  }
               }
               else
               {
                  pContext.mStringBuffer.assign(token.mStart, token.mLength);
                  throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
               }
            }
         }
      }
      break;
   }

   if(statement)
   {
      statement->mLine = statementLine;
   }

   return statement;
}

StatementBlock* Environment::parseStatementBlock(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   if(token.mStart[0] != '{')
      return nullptr;

   StatementBlock* block = (StatementBlock*)CflatMalloc(sizeof(StatementBlock));
   CflatInvokeCtor(StatementBlock, block)();

   incrementScopeLevel(pContext);

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, '{', '}');

   while(tokenIndex < closureTokenIndex)
   {
      tokenIndex++;
      Statement* statement = parseStatement(pContext);

      if(statement)
      {
         block->mStatements.push_back(statement);
      }
   }

   decrementScopeLevel(pContext);

   return block;
}

StatementUsingDirective* Environment::parseStatementUsingDirective(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   StatementUsingDirective* statement = nullptr;

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
      Namespace* ns = pContext.mNamespaceStack.back()->getNamespace(identifier);

      if(ns)
      {
         UsingDirective usingDirective(ns);
         usingDirective.mScopeLevel = pContext.mScopeLevel;
         pContext.mUsingDirectives.push_back(usingDirective);

         statement = (StatementUsingDirective*)CflatMalloc(sizeof(StatementUsingDirective));
         CflatInvokeCtor(StatementUsingDirective, statement)(ns);
      }
      else
      {
         throwCompileError(pContext, CompileError::UnknownNamespace, identifier.mName);
      }
   }
   else
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "using");
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

      statement = (StatementNamespaceDeclaration*)CflatMalloc(sizeof(StatementNamespaceDeclaration));
      CflatInvokeCtor(StatementNamespaceDeclaration, statement)(nsIdentifier);

      tokenIndex++;
      statement->mBody = parseStatementBlock(pContext);

      if(statement->mBody)
      {
         statement->mBody->mAlterScope = false;
      }
   }
   else
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "namespace");
   }

   return statement;
}

StatementVariableDeclaration* Environment::parseStatementVariableDeclaration(ParsingContext& pContext,
   TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   StatementVariableDeclaration* statement = nullptr;

   bool instanceAlreadyRegistered = false;

   for(size_t i = 0u; i < pContext.mRegisteredInstances.size(); i++)
   {
      if(pContext.mRegisteredInstances[i].mIdentifier == pIdentifier &&
         pContext.mRegisteredInstances[i].mNamespace == pContext.mNamespaceStack.back())
      {
         instanceAlreadyRegistered = true;
         break;
      }
   }

   if(!instanceAlreadyRegistered)
   {
      Expression* initialValue = nullptr;

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
            arraySizeValue.initOnStack(getTypeUsage("size_t"), &mExecutionContext.mStack);
            evaluateExpression(mExecutionContext, arraySizeExpression, &arraySizeValue);

            arraySize = (uint16_t)CflatValueAs(&arraySizeValue, size_t);

            CflatInvokeDtor(Expression, arraySizeExpression);
            CflatFree(arraySizeExpression);
         }

         tokenIndex = arrayClosure + 1u;

         if(tokens[tokenIndex].mStart[0] == '=')
         {
            tokenIndex++;
            initialValue =
               parseExpression(pContext, findClosureTokenIndex(pContext, ';') - 1u);

            if(!initialValue || initialValue->getType() != ExpressionType::ArrayInitialization)
            {
               throwCompileError(pContext, CompileError::ArrayInitializationExpected);
               return nullptr;
            }

            ExpressionArrayInitialization* arrayInitialization =
               static_cast<ExpressionArrayInitialization*>(initialValue);
            arrayInitialization->mElementType = pTypeUsage.mType;

            if(!arraySizeSpecified)
            {
               arraySize = (uint16_t)arrayInitialization->mValues.size();
            }
         }
         else if(!arraySizeSpecified)
         {
            throwCompileError(pContext, CompileError::ArrayInitializationExpected);
            return nullptr;
         }

         CflatAssert(arraySize > 0u);
         pTypeUsage.mArraySize = arraySize;
      }
      // variable/object
      else if(token.mStart[0] == '=')
      {
         tokenIndex++;
         initialValue =
            parseExpression(pContext, findClosureTokenIndex(pContext, ';') - 1u);

         if(pTypeUsage.mType == mAutoType)
         {
            Value value;
            value.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(mExecutionContext, initialValue, &value);
            pTypeUsage.mType = value.mTypeUsage.mType;
         }
      }
      // object with construction
      else if(pTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
         !pTypeUsage.isPointer())
      {
         Type* type = pTypeUsage.mType;

         if(token.mStart[0] == '(')
         {
            initialValue =
               (ExpressionObjectConstruction*)CflatMalloc(sizeof(ExpressionObjectConstruction));
            CflatInvokeCtor(ExpressionObjectConstruction, initialValue)(pTypeUsage.mType);
            parseFunctionCallArguments(pContext,
               static_cast<ExpressionObjectConstruction*>(initialValue)->mArguments);
         }
         else
         {
            Method* anyCtor = findMethod(type, type->mIdentifier);

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

      registerInstance(pContext, pTypeUsage, pIdentifier);

      ParsingContext::RegisteredInstance registeredInstance;
      registeredInstance.mIdentifier = pIdentifier;
      registeredInstance.mNamespace = pContext.mNamespaceStack.back();
      pContext.mRegisteredInstances.push_back(registeredInstance);

      statement = (StatementVariableDeclaration*)CflatMalloc(sizeof(StatementVariableDeclaration));
      CflatInvokeCtor(StatementVariableDeclaration, statement)
         (pTypeUsage, pIdentifier, initialValue);
   }
   else
   {
      throwCompileError(pContext, CompileError::VariableRedefinition, pIdentifier.mName);
   }

   return statement;
}

StatementFunctionDeclaration* Environment::parseStatementFunctionDeclaration(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   const Token& previousToken = tokens[tokenIndex - 1u];

   pContext.mStringBuffer.assign(previousToken.mStart, previousToken.mLength);
   TypeUsage returnType = getTypeUsage(pContext.mStringBuffer.c_str());

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   const Identifier functionIdentifier(pContext.mStringBuffer.c_str());

   StatementFunctionDeclaration* statement =
      (StatementFunctionDeclaration*)CflatMalloc(sizeof(StatementFunctionDeclaration));
   CflatInvokeCtor(StatementFunctionDeclaration, statement)(returnType, functionIdentifier);

   tokenIndex++;

   while(tokens[tokenIndex++].mStart[0] != ')')
   {
      // no arguments
      if(tokens[tokenIndex].mStart[0] == ')')
      {
         tokenIndex++;
         break;
      }

      TypeUsage parameterType = parseTypeUsage(pContext);
      statement->mParameterTypes.push_back(parameterType);
      tokenIndex++;

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      Identifier parameterIdentifier(pContext.mStringBuffer.c_str());
      statement->mParameterIdentifiers.push_back(parameterIdentifier);
      tokenIndex++;

      Instance* parameterInstance =
         registerInstance(pContext, parameterType, parameterIdentifier);
      parameterInstance->mScopeLevel++;
   }

   statement->mBody = parseStatementBlock(pContext);

   return statement;
}

StatementAssignment* Environment::parseStatementAssignment(ParsingContext& pContext, size_t pOperatorTokenIndex)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   Expression* leftValue = parseExpression(pContext, pOperatorTokenIndex - 1u);

   const Token& operatorToken = pContext.mTokens[pOperatorTokenIndex];
   CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, ';');
   tokenIndex = pOperatorTokenIndex + 1u;
   Expression* rightValue = parseExpression(pContext, closureTokenIndex - 1u);

   StatementAssignment* statement = (StatementAssignment*)CflatMalloc(sizeof(StatementAssignment));
   CflatInvokeCtor(StatementAssignment, statement)(leftValue, rightValue, operatorStr.c_str());

   tokenIndex = closureTokenIndex;

   return statement;
}

StatementIf* Environment::parseStatementIf(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "if");
      return nullptr;
   }

   tokenIndex++;
   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* ifStatement = parseStatement(pContext);

   if(ifStatement->getType() == StatementType::Block)
   {
      tokenIndex++;
   }

   Statement* elseStatement = nullptr;

   if(tokens[tokenIndex].mType == TokenType::Keyword &&
      strncmp(tokens[tokenIndex].mStart, "else", 4u) == 0)
   {
      tokenIndex++;
      elseStatement = parseStatement(pContext);
   }

   StatementIf* statement = (StatementIf*)CflatMalloc(sizeof(StatementIf));
   CflatInvokeCtor(StatementIf, statement)(condition, ifStatement, elseStatement);

   return statement;
}

StatementSwitch* Environment::parseStatementSwitch(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "switch");
      return nullptr;
   }

   tokenIndex++;
   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   if(tokens[conditionClosureTokenIndex + 1u].mStart[0] != '{')
   {
      pContext.mStringBuffer.assign(tokens[tokenIndex - 1u].mStart, tokens[tokenIndex - 1u].mLength);
      throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
      return nullptr;
   }

   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   StatementSwitch* statement = (StatementSwitch*)CflatMalloc(sizeof(StatementSwitch));
   CflatInvokeCtor(StatementSwitch, statement)(condition);

   tokenIndex++;
   const size_t lastSwitchTokenIndex = findClosureTokenIndex(pContext, '{', '}');

   StatementSwitch::CaseSection* currentCaseSection = nullptr;

   for(; tokenIndex < lastSwitchTokenIndex; tokenIndex++)
   {
      if(tokens[tokenIndex].mType == TokenType::Keyword)
      {
         if(strncmp(tokens[tokenIndex].mStart, "case", 4u) == 0)
         {
            tokenIndex++;
            const size_t lastCaseTokenIndex = findClosureTokenIndex(pContext, ':');

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
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "while");
      return nullptr;
   }

   tokenIndex++;
   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* loopStatement = parseStatement(pContext);

   StatementWhile* statement = (StatementWhile*)CflatMalloc(sizeof(StatementWhile));
   CflatInvokeCtor(StatementWhile, statement)(condition, loopStatement);

   return statement;
}

StatementDoWhile* Environment::parseStatementDoWhile(ParsingContext& pContext)
{
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

   tokenIndex++;
   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');

   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);

   StatementDoWhile* statement = (StatementDoWhile*)CflatMalloc(sizeof(StatementDoWhile));
   CflatInvokeCtor(StatementDoWhile, statement)(condition, loopStatement);

   return statement;
}

StatementFor* Environment::parseStatementFor(ParsingContext& pContext)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      throwCompileError(pContext, CompileError::UnexpectedSymbol, "for");
      return nullptr;
   }

   incrementScopeLevel(pContext);

   tokenIndex++;
   const size_t initializationClosureTokenIndex = findClosureTokenIndex(pContext, ';');
   Statement* initialization = parseStatement(pContext);
   tokenIndex = initializationClosureTokenIndex + 1u;

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, ';');
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   const size_t incrementClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');
   Statement* increment = parseStatement(pContext);
   tokenIndex = incrementClosureTokenIndex + 1u;

   Statement* loopStatement = parseStatement(pContext);

   decrementScopeLevel(pContext);

   StatementFor* statement = (StatementFor*)CflatMalloc(sizeof(StatementFor));
   CflatInvokeCtor(StatementFor, statement)(initialization, condition, increment, loopStatement);

   return statement;
}

StatementBreak* Environment::parseStatementBreak(ParsingContext& pContext)
{
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
   Expression* expression = parseExpression(pContext, findClosureTokenIndex(pContext, ';') - 1u);

   StatementReturn* statement = (StatementReturn*)CflatMalloc(sizeof(StatementReturn));
   CflatInvokeCtor(StatementReturn, statement)(expression);

   return statement;
}

bool Environment::parseFunctionCallArguments(ParsingContext& pContext,
   CflatSTLVector(Expression*)& pArguments)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   while(tokens[tokenIndex++].mStart[0] != ')')
   {
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, ')');
      const size_t separatorTokenIndex = findClosureTokenIndex(pContext, ',');

      size_t tokenLastIndex = closureTokenIndex;

      if(separatorTokenIndex > 0u && separatorTokenIndex < closureTokenIndex)
      {
         tokenLastIndex = separatorTokenIndex;
      }

      Expression* argument = parseExpression(pContext, tokenLastIndex - 1u);

      if(argument)
      {
         pArguments.push_back(argument);
         tokenIndex++;
      }
   }

   return true;
}

bool Environment::parseMemberAccessIdentifiers(ParsingContext& pContext,
   CflatSTLVector(Identifier)& pIdentifiers)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   TypeUsage typeUsage;
   bool anyRemainingMemberAccessIdentifiers = true;

   while(anyRemainingMemberAccessIdentifiers)
   {
      const bool memberAccess = tokens[tokenIndex + 1u].mStart[0] == '.';
      const bool ptrMemberAccess = !memberAccess && strncmp(tokens[tokenIndex + 1u].mStart, "->", 2u) == 0;

      anyRemainingMemberAccessIdentifiers = memberAccess || ptrMemberAccess;

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      pIdentifiers.push_back(pContext.mStringBuffer.c_str());

      if(pIdentifiers.size() == 1u)
      {
         Instance* instance = retrieveInstance(pContext, pIdentifiers.back());
         typeUsage = instance->mValue.mTypeUsage;
      }
      else if(tokens[tokenIndex + 1u].mStart[0] != '(')
      {
         const Identifier& memberName = pIdentifiers.back();
         Struct* type = static_cast<Struct*>(typeUsage.mType);
         Member* member = nullptr;

         for(size_t j = 0u; j < type->mMembers.size(); j++)
         {
            if(type->mMembers[j].mIdentifier == memberName)
            {
               member = &type->mMembers[j];
               break;
            }
         }

         if(member)
         {
            typeUsage = member->mTypeUsage;
         }
         else
         {
            throwCompileError(pContext, CompileError::MissingMember, memberName.mName);
            return false;
         }
      }
      else
      {
         // method call
         typeUsage = TypeUsage();
      }

      if(typeUsage.isPointer())
      {
         if(!ptrMemberAccess)
         {
            throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorPtr,
               pIdentifiers.back().mName);
            return false;
         }
      }
      else
      {
         if(ptrMemberAccess)
         {
            throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorNonPtr,
               pIdentifiers.back().mName);
            return false;
         }
      }

      tokenIndex++;

      if(anyRemainingMemberAccessIdentifiers)
      {
         tokenIndex++;
      }
   }

   return true;
}

Instance* Environment::registerInstance(Context& pContext,
   const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   Instance* instance = pContext.mNamespaceStack.back()->retrieveInstance(pIdentifier);

   if(!instance)
   {   
      instance = pContext.mNamespaceStack.back()->registerInstance(pTypeUsage, pIdentifier);

      if(instance->mTypeUsage.isReference())
      {
         instance->mValue.initExternal(instance->mTypeUsage);
      }
      else
      {
         instance->mValue.initOnStack(instance->mTypeUsage, &mExecutionContext.mStack);
      }
   }

   CflatAssert(instance);
   CflatAssert(instance->mTypeUsage == pTypeUsage);

   instance->mScopeLevel = pContext.mScopeLevel;

   return instance;
}

Instance* Environment::retrieveInstance(const Context& pContext, const Identifier& pIdentifier)
{
   Instance* instance = pContext.mNamespaceStack.back()->retrieveInstance(pIdentifier);

   if(!instance)
   {
      for(size_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
      {
         instance = pContext.mUsingDirectives[i].mNamespace->retrieveInstance(pIdentifier);

         if(instance)
            break;
      }
   }

   return instance;
}

void Environment::incrementScopeLevel(Context& pContext)
{
   pContext.mScopeLevel++;
}

void Environment::decrementScopeLevel(Context& pContext)
{
   mGlobalNamespace.releaseInstances(pContext.mScopeLevel);

   while(!pContext.mUsingDirectives.empty() &&
      pContext.mUsingDirectives.back().mScopeLevel >= pContext.mScopeLevel)
   {
      pContext.mUsingDirectives.pop_back();
   }

   pContext.mScopeLevel--;
}

void Environment::throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg)
{
   char errorMsg[256];
   sprintf(errorMsg, kRuntimeErrorStrings[(int)pError], pArg);

   char lineAsString[16];
   sprintf(lineAsString, "%d", pContext.mCurrentLine);

   pContext.mErrorMessage.assign("[Runtime Error] Line ");
   pContext.mErrorMessage.append(lineAsString);
   pContext.mErrorMessage.append(": ");
   pContext.mErrorMessage.append(errorMsg);
}

void Environment::evaluateExpression(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue)
{
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
         void* nullPointer = nullptr;
         pOutValue->set(&nullPointer);
      }
      break;
   case ExpressionType::VariableAccess:
      {
         ExpressionVariableAccess* expression = static_cast<ExpressionVariableAccess*>(pExpression);
         Instance* instance = retrieveInstance(pContext, expression->mVariableIdentifier);
         *pOutValue = instance->mValue;
      }
      break;
   case ExpressionType::ArrayElementAccess:
      {
         ExpressionArrayElementAccess* expression =
            static_cast<ExpressionArrayElementAccess*>(pExpression);

         Value arrayValue;
         arrayValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mArray, &arrayValue);
         const size_t arraySize = (size_t)arrayValue.mTypeUsage.mArraySize;

         Value indexValue;
         indexValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mArrayElementIndex, &indexValue);
         const size_t index = (size_t)getValueAsInteger(indexValue);

         if(index < arraySize)
         {
            TypeUsage arrayElementTypeUsage;
            arrayElementTypeUsage.mType = arrayValue.mTypeUsage.mType;
            assertValueInitialization(pContext, arrayElementTypeUsage, pOutValue);

            const size_t arrayElementSize = arrayElementTypeUsage.mType->mSize;
            const size_t offset = arrayElementSize * index;
            memcpy(pOutValue->mValueBuffer, arrayValue.mValueBuffer + offset, arrayElementSize);
         }
         else
         {
            char buffer[256];
            sprintf(buffer, "size %zu, index %zu", arraySize, index);
            throwRuntimeError(pContext, RuntimeError::InvalidArrayIndex, buffer);
         }         
      }
      break;
   case ExpressionType::UnaryOperation:
      {
         ExpressionUnaryOperation* expression = static_cast<ExpressionUnaryOperation*>(pExpression);

         Value preValue;
         preValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mExpression, &preValue);

         assertValueInitialization(pContext, preValue.mTypeUsage, pOutValue);
         pOutValue->set(preValue.mValueBuffer);

         const bool isIncrementOrDecrement =
            strncmp(expression->mOperator, "++", 2u) == 0 ||
            strncmp(expression->mOperator, "--", 2u) == 0;

         if(isIncrementOrDecrement)
         {
            preValue.mValueBufferType = ValueBufferType::Heap;
            applyUnaryOperator(pContext, expression->mOperator, &preValue);
            preValue.mValueBufferType = ValueBufferType::External;

            if(!expression->mPostOperator)
            {
               pOutValue->set(preValue.mValueBuffer);
            }
         }
         else
         {
            applyUnaryOperator(pContext, expression->mOperator, pOutValue);
         }
      }
      break;
   case ExpressionType::BinaryOperation:
      {
         ExpressionBinaryOperation* expression = static_cast<ExpressionBinaryOperation*>(pExpression);

         Value leftValue;
         leftValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mLeft, &leftValue);
         Value rightValue;
         rightValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mRight, &rightValue);

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

         if(expression->mExpression->getType() == ExpressionType::VariableAccess)
         {
            ExpressionVariableAccess* variableAccess =
               static_cast<ExpressionVariableAccess*>(expression->mExpression);
            Instance* instance = retrieveInstance(pContext, variableAccess->mVariableIdentifier);
            pOutValue->mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, &instance->mValue, pOutValue);
         }
      }
      break;
   case ExpressionType::Conditional:
      {
         ExpressionConditional* expression = static_cast<ExpressionConditional*>(pExpression);

         Value conditionValue;
         conditionValue.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mCondition, &conditionValue);

         Expression* valueSource = getValueAsInteger(conditionValue)
            ? expression->mIfExpression
            : expression->mElseExpression;
         evaluateExpression(pContext, valueSource, pOutValue);
      }
      break;
   case ExpressionType::FunctionCall:
      {
         ExpressionFunctionCall* expression = static_cast<ExpressionFunctionCall*>(pExpression);

         CflatSTLVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         Function* function =
            pContext.mNamespaceStack.back()->getFunction(expression->mFunctionIdentifier, argumentValues);

         if(!function)
         {
            for(uint32_t i = 0u; i < pContext.mUsingDirectives.size(); i++)
            {
               function =
                  pContext.mUsingDirectives[i].mNamespace->getFunction(
                     expression->mFunctionIdentifier, argumentValues);

               if(function)
                  break;
            }
         }

         CflatAssert(function);

         prepareArgumentsForFunctionCall(pContext, function->mParameters, argumentValues);

         const bool functionReturnValueIsConst =
            CflatHasFlag(function->mReturnTypeUsage.mFlags, TypeUsageFlags::Const);
         const bool outValueIsConst =
            CflatHasFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);

         if(outValueIsConst && !functionReturnValueIsConst)
         {
            CflatResetFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);
         }

         function->execute(argumentValues, pOutValue);

         if(outValueIsConst && !functionReturnValueIsConst)
         {
            CflatSetFlag(pOutValue->mTypeUsage.mFlags, TypeUsageFlags::Const);
         }
      }
      break;
   case ExpressionType::MethodCall:
      {
         ExpressionMethodCall* expression = static_cast<ExpressionMethodCall*>(pExpression);
         ExpressionMemberAccess* memberAccess =
            static_cast<ExpressionMemberAccess*>(expression->mMemberAccess);

         Value instanceDataValue;
         getInstanceDataValue(pContext, memberAccess, &instanceDataValue);

         if(!pContext.mErrorMessage.empty())
            break;

         CflatSTLVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         const Identifier& methodIdentifier = memberAccess->mIdentifiers.back();
         Method* method =
            findMethod(instanceDataValue.mTypeUsage.mType, methodIdentifier, argumentValues);
         CflatAssert(method);

         Value thisPtr;

         if(instanceDataValue.mTypeUsage.isPointer())
         {
            thisPtr.initOnStack(instanceDataValue.mTypeUsage, &pContext.mStack);
            thisPtr.set(instanceDataValue.mValueBuffer);
         }
         else
         {
            thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, &instanceDataValue, &thisPtr);
         }

         prepareArgumentsForFunctionCall(pContext, method->mParameters, argumentValues);

         assertValueInitialization(pContext, method->mReturnTypeUsage, pOutValue);
         method->execute(thisPtr, argumentValues, pOutValue);
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

         CflatSTLVector(Value) argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

         Struct* type = static_cast<Struct*>(expression->mObjectType);
         Method* ctor = findMethod(type, type->mIdentifier, argumentValues);
         CflatAssert(ctor);

         Value thisPtr;
         thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pOutValue, &thisPtr);

         ctor->execute(thisPtr, argumentValues, nullptr);
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

      const Identifier& instanceIdentifier = memberAccess->mIdentifiers[0];
      Instance* instance = retrieveInstance(pContext, instanceIdentifier);
      *pOutValue = instance->mValue;

      if(pOutValue->mTypeUsage.isPointer() && !CflatValueAs(pOutValue, void*))
      {
         throwRuntimeError(pContext, RuntimeError::NullPointerAccess,
            instanceIdentifier.mName);
         return;
      }

      for(size_t i = 1u; i < memberAccess->mIdentifiers.size(); i++)
      {
         const Identifier& memberIdentifier = memberAccess->mIdentifiers[i];
         Struct* type = static_cast<Struct*>(pOutValue->mTypeUsage.mType);
         Member* member = nullptr;

         for(size_t j = 0u; j < type->mMembers.size(); j++)
         {
            if(type->mMembers[j].mIdentifier == memberIdentifier)
            {
               member = &type->mMembers[j];
               break;
            }
         }

         // the symbol is a member
         if(member)
         {
            char* instanceDataPtr = pOutValue->mTypeUsage.isPointer()
               ? CflatValueAs(pOutValue, char*)
               : pOutValue->mValueBuffer;

            pOutValue->mTypeUsage = member->mTypeUsage;
            pOutValue->mValueBuffer = instanceDataPtr + member->mOffset;

            if(pOutValue->mTypeUsage.isPointer() && !CflatValueAs(pOutValue, void*))
            {
               throwRuntimeError(pContext, RuntimeError::NullPointerAccess,
                  member->mIdentifier.mName);
               break;
            }
         }
         // the symbol is a method
         else
         {
            break;
         }
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
      pOutValue->mValueBuffer = arrayDataValue.mValueBuffer + (arrayIndex * arrayElementTypeUsage.getSize());
   }
}

void Environment::getAddressOfValue(ExecutionContext& pContext, Value* pInstanceDataValue, Value* pOutValue)
{
   TypeUsage pointerTypeUsage = pInstanceDataValue->mTypeUsage;
   pointerTypeUsage.mPointerLevel++;

   assertValueInitialization(pContext, pointerTypeUsage, pOutValue);
   pOutValue->set(&pInstanceDataValue->mValueBuffer);
}

void Environment::getArgumentValues(ExecutionContext& pContext,
   const CflatSTLVector(Expression*)& pExpressions, CflatSTLVector(Value)& pValues)
{
   pValues.resize(pExpressions.size());

   for(size_t i = 0u; i < pExpressions.size(); i++)
   {
      evaluateExpression(pContext, pExpressions[i], &pValues[i]);
   }
}

void Environment::prepareArgumentsForFunctionCall(ExecutionContext& pContext,
   const CflatSTLVector(TypeUsage)& pParameters, CflatSTLVector(Value)& pValues)
{
   CflatAssert(pParameters.size() == pValues.size());

   for(size_t i = 0u; i < pParameters.size(); i++)
   {
      if(pParameters[i].isReference())
      {
         // pass by reference
         if(pValues[i].mValueBufferType != ValueBufferType::External)
         {
            TypeUsage cachedTypeUsage = pValues[i].mTypeUsage;
            char* cachedValueBuffer = pValues[i].mValueBuffer;
            pValues[i].reset();

            pValues[i].initExternal(cachedTypeUsage);
            pValues[i].set(cachedValueBuffer);
         }

         CflatSetFlag(pValues[i].mTypeUsage.mFlags, TypeUsageFlags::Reference);
      }
      else
      {
         // pass by value
         if(pValues[i].mValueBufferType == ValueBufferType::External)
         {
            TypeUsage cachedTypeUsage = pValues[i].mTypeUsage;
            char* cachedValueBuffer = pValues[i].mValueBuffer;
            pValues[i].reset();

            pValues[i].initOnStack(cachedTypeUsage, &pContext.mStack);
            pValues[i].set(cachedValueBuffer);
         }
      }
   }
}

void Environment::applyUnaryOperator(ExecutionContext& pContext, const char* pOperator, Value* pOutValue)
{
   Type* type = pOutValue->mTypeUsage.mType;

   if(type->mCategory == TypeCategory::BuiltIn)
   {
      const bool integerValue = type->isInteger();

      const int64_t valueAsInteger = getValueAsInteger(*pOutValue);
      const double valueAsDecimal = getValueAsDecimal(*pOutValue);

      if(strcmp(pOperator, "++") == 0)
      {
         setValueAsInteger(valueAsInteger + 1u, pOutValue);
      }
      else if(strcmp(pOperator, "--") == 0)
      {
         setValueAsInteger(valueAsInteger - 1u, pOutValue);
      }
      else if(pOperator[0] == '-')
      {
         if(integerValue)
         {
            setValueAsInteger(-valueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(-valueAsDecimal, pOutValue);
         }
      }
   }
   else
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      CflatSTLVector(Value) args;

      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = findMethod(type, operatorIdentifier);
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pOutValue, &thisPtrValue);

         assertValueInitialization(pContext, operatorMethod->mReturnTypeUsage, pOutValue);
         operatorMethod->execute(thisPtrValue, args, pOutValue);
      }
      else
      {
         args.push_back(*pOutValue);

         Function* operatorFunction = getFunction(operatorIdentifier, args);

         if(operatorFunction)
         {
            assertValueInitialization(pContext, operatorMethod->mReturnTypeUsage, pOutValue);
            operatorFunction->execute(args, pOutValue);
         }
      }
   }
}

void Environment::applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
   const char* pOperator, Value* pOutValue)
{
   Type* leftType = pLeft.mTypeUsage.mType;

   if(leftType->mCategory == TypeCategory::BuiltIn)
   {
      const bool integerValues = leftType->isInteger();

      const int64_t leftValueAsInteger = getValueAsInteger(pLeft);
      const int64_t rightValueAsInteger = getValueAsInteger(pRight);
      const double leftValueAsDecimal = getValueAsDecimal(pLeft);
      const double rightValueAsDecimal = getValueAsDecimal(pRight);

      if(strcmp(pOperator, "==") == 0)
      {
         const bool result = leftValueAsInteger == rightValueAsInteger;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "!=") == 0)
      {
         const bool result = leftValueAsInteger != rightValueAsInteger;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "<") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger < rightValueAsInteger
            : leftValueAsDecimal < rightValueAsDecimal;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, ">") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger > rightValueAsInteger
            : leftValueAsDecimal > rightValueAsDecimal;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "<=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger <= rightValueAsInteger
            : leftValueAsDecimal <= rightValueAsDecimal;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, ">=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger >= rightValueAsInteger
            : leftValueAsDecimal >= rightValueAsDecimal;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "&&") == 0)
      {
         const bool result = leftValueAsInteger && rightValueAsInteger;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "||") == 0)
      {
         const bool result = leftValueAsInteger || rightValueAsInteger;

         const TypeUsage typeUsage = getTypeUsage("bool");
         assertValueInitialization(pContext, typeUsage, pOutValue);
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "+") == 0)
      {
         assertValueInitialization(pContext, pLeft.mTypeUsage, pOutValue);

         if(integerValues)
         {
            setValueAsInteger(leftValueAsInteger + rightValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(leftValueAsDecimal + rightValueAsDecimal, pOutValue);
         }
      }
      else if(strcmp(pOperator, "-") == 0)
      {
         assertValueInitialization(pContext, pLeft.mTypeUsage, pOutValue);

         if(integerValues)
         {
            setValueAsInteger(leftValueAsInteger - rightValueAsInteger, pOutValue);
         }
         else
         {
            setValueAsDecimal(leftValueAsDecimal - rightValueAsDecimal, pOutValue);
         }
      }
      else if(strcmp(pOperator, "*") == 0)
      {
         assertValueInitialization(pContext, pLeft.mTypeUsage, pOutValue);

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
         assertValueInitialization(pContext, pLeft.mTypeUsage, pOutValue);

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
   }
   else
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      CflatSTLVector(Value) args;
      args.push_back(pRight);

      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = findMethod(leftType, operatorIdentifier, args);
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, &const_cast<Cflat::Value&>(pLeft), &thisPtrValue);

         assertValueInitialization(pContext, operatorMethod->mReturnTypeUsage, pOutValue);
         operatorMethod->execute(thisPtrValue, args, pOutValue);
      }
      else
      {
         args.insert(args.begin(), pLeft);

         Function* operatorFunction = getFunction(operatorIdentifier, args);

         if(operatorFunction)
         {
            assertValueInitialization(pContext, operatorFunction->mReturnTypeUsage, pOutValue);
            operatorFunction->execute(args, pOutValue);
         }
      }
   }
}

void Environment::performAssignment(ExecutionContext& pContext, Value* pValue,
   const char* pOperator, Value* pInstanceDataValue)
{
   if(strcmp(pOperator, "=") == 0)
   {
      memcpy(pInstanceDataValue->mValueBuffer, pValue->mValueBuffer, pValue->mTypeUsage.getSize());
   }
   else
   {
      char binaryOperator[2];
      binaryOperator[0] = pOperator[0];
      binaryOperator[1] = '\0';

      const ValueBufferType cachedValueBufferType = pInstanceDataValue->mValueBufferType;
      pInstanceDataValue->mValueBufferType = ValueBufferType::Heap;
      applyBinaryOperator(pContext, *pInstanceDataValue, *pValue, binaryOperator, pInstanceDataValue);
      pInstanceDataValue->mValueBufferType = cachedValueBufferType;
   }
}

void Environment::execute(ExecutionContext& pContext, const Program& pProgram)
{
   for(size_t i = 0u; i < pProgram.mStatements.size(); i++)
   {
      execute(pContext, pProgram.mStatements[i]);

      if(!pContext.mErrorMessage.empty())
         break;
   }
}

void Environment::assertValueInitialization(ExecutionContext& pContext, const TypeUsage& pTypeUsage,
   Value* pOutValue)
{
   if(pOutValue->mValueBufferType == ValueBufferType::Uninitialized &&
      pTypeUsage.isReference())
   {
      pOutValue->initExternal(pTypeUsage);
   }
   else if(pOutValue->mValueBufferType == ValueBufferType::Uninitialized &&
      pOutValue->mValueInitializationHint == ValueInitializationHint::Stack)
   {
      pOutValue->initOnStack(pTypeUsage, &pContext.mStack);
   }
   else if(pOutValue->mValueBufferType == ValueBufferType::Uninitialized ||
      !pOutValue->mTypeUsage.compatibleWith(pTypeUsage))
   {
      pOutValue->initOnHeap(pTypeUsage);
   }
}

int64_t Environment::getValueAsInteger(const Value& pValue)
{
   int64_t valueAsInteger = 0u;

   if(pValue.mTypeUsage.mType->mSize == 4u)
   {
      valueAsInteger = (int64_t)CflatValueAs(&pValue, int32_t);
   }
   else if(pValue.mTypeUsage.mType->mSize == 8u)
   {
      valueAsInteger = CflatValueAs(&pValue, int64_t);
   }
   else if(pValue.mTypeUsage.mType->mSize == 2u)
   {
      valueAsInteger = (int64_t)CflatValueAs(&pValue, int16_t);
   }
   else if(pValue.mTypeUsage.mType->mSize == 1u)
   {
      valueAsInteger = (int64_t)CflatValueAs(&pValue, int8_t);
   }

   return valueAsInteger;
}

double Environment::getValueAsDecimal(const Value& pValue)
{
   double valueAsDecimal = 0.0;

   if(pValue.mTypeUsage.mType->mSize == 4u)
   {
      valueAsDecimal = (double)CflatValueAs(&pValue, float);
   }
   else if(pValue.mTypeUsage.mType->mSize == 8u)
   {
      valueAsDecimal = CflatValueAs(&pValue, double);
   }

   return valueAsDecimal;
}

void Environment::setValueAsInteger(int64_t pInteger, Value* pOutValue)
{
   const size_t typeSize = pOutValue->mTypeUsage.mType->mSize;

   if(typeSize == 4u)
   {
      const int32_t value = (int32_t)pInteger;
      pOutValue->set(&value);
   }
   else if(typeSize == 8u)
   {
      pOutValue->set(&pInteger);
   }
   else if(typeSize == 2u)
   {
      const int16_t value = (int16_t)pInteger;
      pOutValue->set(&value);
   }
   else if(typeSize == 1u)
   {
      const int8_t value = (int8_t)pInteger;
      pOutValue->set(&value);
   }
}

void Environment::setValueAsDecimal(double pDecimal, Value* pOutValue)
{
   const size_t typeSize = pOutValue->mTypeUsage.mType->mSize;

   if(typeSize == 4u)
   {
      const float value = (float)pDecimal;
      pOutValue->set(&value);
   }
   else if(typeSize == 8u)
   {
      pOutValue->set(&pDecimal);
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
         type->mMethods[i].mIdentifier == type->mIdentifier)
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
   const CflatSTLVector(TypeUsage)& pParameterTypes)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);

   Method* method = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(type->mMethods[i].mIdentifier == pIdentifier &&
         type->mMethods[i].mParameters.size() == pParameterTypes.size())
      {
         bool parametersMatch = true;

         for(size_t j = 0u; j < pParameterTypes.size(); j++)
         {
            if(!type->mMethods[i].mParameters[j].compatibleWith(pParameterTypes[j]))
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

   return method;
}

Method* Environment::findMethod(Type* pType, const Identifier& pIdentifier,
   const CflatSTLVector(Value)& pArguments)
{
   CflatSTLVector(TypeUsage) typeUsages;
   typeUsages.reserve(pArguments.size());

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return findMethod(pType, pIdentifier, typeUsages);
}

void Environment::execute(ExecutionContext& pContext, Statement* pStatement)
{
   pContext.mCurrentLine = pStatement->mLine;

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
      }
      break;
   case StatementType::UsingDirective:
      {
         StatementUsingDirective* statement =
            static_cast<StatementUsingDirective*>(pStatement);

         UsingDirective usingDirective(statement->mNamespace);
         usingDirective.mScopeLevel = pContext.mScopeLevel;
         pContext.mUsingDirectives.push_back(usingDirective);
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
         Instance* instance = registerInstance(pContext, statement->mTypeUsage, statement->mVariableIdentifier);

         // if there is an assignment in the declaration, set the value
         if(statement->mInitialValue)
         {
            evaluateExpression(pContext, statement->mInitialValue, &instance->mValue);
         }
         // otherwise, call the default constructor if the type is a struct or a class
         else if(instance->mTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
            !instance->mTypeUsage.isPointer())
         {
            instance->mValue.mTypeUsage = instance->mTypeUsage;
            Value thisPtr;
            thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, &instance->mValue, &thisPtr);

            Method* defaultCtor = getDefaultConstructor(instance->mTypeUsage.mType);

            if(defaultCtor)
            {
               CflatSTLVector(Value) args;
               defaultCtor->execute(thisPtr, args, nullptr);
            }
         }
      }
      break;
   case StatementType::FunctionDeclaration:
      {
         StatementFunctionDeclaration* statement = static_cast<StatementFunctionDeclaration*>(pStatement);

         Function* function =
            pContext.mNamespaceStack.back()->getFunction(statement->mFunctionIdentifier,
               statement->mParameterTypes);

         if(!function)
         {
            function = pContext.mNamespaceStack.back()->registerFunction(statement->mFunctionIdentifier);
            function->mReturnTypeUsage = statement->mReturnType;

            for(size_t i = 0u; i < statement->mParameterTypes.size(); i++)
            {
               function->mParameters.push_back(statement->mParameterTypes[i]);
            }
         }

         if(statement->mBody)
         {
            function->execute =
               [this, &pContext, function, statement](CflatSTLVector(Value)& pArguments, Value* pOutReturnValue)
            {
               CflatAssert(function->mParameters.size() == pArguments.size());
               
               for(size_t i = 0u; i < pArguments.size(); i++)
               {
                  const TypeUsage parameterType = statement->mParameterTypes[i];
                  const Identifier& parameterIdentifier = statement->mParameterIdentifiers[i];
                  Instance* argumentInstance = registerInstance(pContext, parameterType, parameterIdentifier);
                  argumentInstance->mScopeLevel++;
                  argumentInstance->mValue.set(pArguments[i].mValueBuffer);
               }

               if(function->mReturnTypeUsage.mType)
               {
                  pContext.mReturnValue.initOnStack(function->mReturnTypeUsage, &pContext.mStack);
               }

               execute(pContext, statement->mBody);

               if(function->mReturnTypeUsage.mType)
               {
                  if(pOutReturnValue)
                  {
                     assertValueInitialization(pContext, function->mReturnTypeUsage, pOutReturnValue);
                     pOutReturnValue->set(pContext.mReturnValue.mValueBuffer);
                  }

                  pContext.mReturnValue.reset();
               }

               pContext.mJumpStatement = JumpStatement::None;
            };
         }
      }
      break;
   case StatementType::Assignment:
      {
         StatementAssignment* statement = static_cast<StatementAssignment*>(pStatement);

         Value instanceDataValue;
         getInstanceDataValue(pContext, statement->mLeftValue, &instanceDataValue);

         Value rightValue;
         evaluateExpression(pContext, statement->mRightValue, &rightValue);

         performAssignment(pContext, &rightValue, statement->mOperator, &instanceDataValue);
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
            if(pContext.mJumpStatement == JumpStatement::Continue)
            {
               pContext.mJumpStatement = JumpStatement::None;
            }

            execute(pContext, statement->mLoopStatement);

            if(pContext.mJumpStatement == JumpStatement::Break)
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
            if(pContext.mJumpStatement == JumpStatement::Continue)
            {
               pContext.mJumpStatement = JumpStatement::None;
            }

            execute(pContext, statement->mLoopStatement);

            if(pContext.mJumpStatement == JumpStatement::Break)
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
            conditionValue.initOnStack(getTypeUsage("bool"), &pContext.mStack);
            conditionValue.set(&defaultConditionValue);

            bool conditionMet = defaultConditionValue;

            if(statement->mCondition)
            {
               evaluateExpression(pContext, statement->mCondition, &conditionValue);
               conditionMet = getValueAsInteger(conditionValue) != 0;
            }

            while(conditionMet)
            {
               if(pContext.mJumpStatement == JumpStatement::Continue)
               {
                  pContext.mJumpStatement = JumpStatement::None;
               }

               execute(pContext, statement->mLoopStatement);

               if(pContext.mJumpStatement == JumpStatement::Break)
               {
                  pContext.mJumpStatement = JumpStatement::None;
                  break;
               }

               if(statement->mIncrement)
               {
                  execute(pContext, statement->mIncrement);
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
            evaluateExpression(pContext, statement->mExpression, &pContext.mReturnValue);
         }

         pContext.mJumpStatement = JumpStatement::Return;
      }
      break;
   default:
      break;
   }
}

bool Environment::load(const char* pProgramName, const char* pCode)
{
   const uint32_t programNameHash = hash(pProgramName);
   ProgramsRegistry::iterator it = mPrograms.find(programNameHash);

   if(it == mPrograms.end())
   {
      mPrograms[programNameHash] = Program();
      it = mPrograms.find(programNameHash);
   }

   Program& program = it->second;
   program.~Program();

   strcpy(program.mName, pProgramName);
   program.mCode.assign(pCode);
   program.mCode.shrink_to_fit();

   mErrorMessage.clear();

   ParsingContext parsingContext(&mGlobalNamespace);

   preprocess(parsingContext, pCode);
   tokenize(parsingContext);
   parse(parsingContext, program);

   if(!parsingContext.mErrorMessage.empty())
   {
      mErrorMessage.assign(parsingContext.mErrorMessage);
      return false;
   }

   mExecutionContext.mCurrentLine = 0u;
   mExecutionContext.mJumpStatement = JumpStatement::None;

   execute(mExecutionContext, program);

   if(!mExecutionContext.mErrorMessage.empty())
   {
      mErrorMessage.assign(mExecutionContext.mErrorMessage);
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


///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.80
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2025 Arturo Cepeda Pérez and contributors
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
      SizeOf,
      Cast,
      Conditional,
      Assignment,
      FunctionCall,
      MethodCall,
      ArrayInitialization,
      AggregateInitialization,
      ObjectConstruction
   };

   struct Expression
   {
   protected:
      ExpressionType mType;
      TypeUsage mTypeUsage;

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

      const TypeUsage& getTypeUsage() const
      {
         return mTypeUsage;
      }
   };

   struct ExpressionValue : Expression
   {
      Value mValue;

      ExpressionValue(const Value& pValue)
      {
         mType = ExpressionType::Value;

         mTypeUsage = pValue.mTypeUsage;
         CflatSetFlag(mTypeUsage.mFlags, TypeUsageFlags::Const);

         mValue.initOnHeap(pValue.mTypeUsage);
         mValue.set(pValue.mValueBuffer);
      }
   };

   struct ExpressionNullPointer : Expression
   {
      ExpressionNullPointer(const TypeUsage& pTypeUsage)
      {
         mType = ExpressionType::NullPointer;
         mTypeUsage = pTypeUsage;
      }
   };

   struct ExpressionVariableAccess : Expression
   {
      Identifier mVariableIdentifier;

      ExpressionVariableAccess(const Identifier& pVariableIdentifier,
         const TypeUsage& pVariableTypeUsage)
         : mVariableIdentifier(pVariableIdentifier)
      {
         mType = ExpressionType::VariableAccess;
         mTypeUsage = pVariableTypeUsage;
      }
   };

   struct ExpressionMemberAccess : Expression
   {
      Expression* mMemberOwner;
      Value mMemberOwnerValue;
      Identifier mMemberIdentifier;

      ExpressionMemberAccess(Expression* pMemberOwner, const Identifier& pMemberIdentifier,
         const TypeUsage& pMemberTypeUsage)
         : mMemberOwner(pMemberOwner)
         , mMemberIdentifier(pMemberIdentifier)
      {
         mType = ExpressionType::MemberAccess;
         mTypeUsage = pMemberTypeUsage;
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

      ExpressionArrayElementAccess(Expression* pArray, Expression* pArrayElementIndex,
         const TypeUsage& pTypeUsage)
         : mArray(pArray)
         , mArrayElementIndex(pArrayElementIndex)
      {
         mType = ExpressionType::ArrayElementAccess;
         mTypeUsage = pTypeUsage;
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

      ExpressionUnaryOperation(Expression* pExpression, const char* pOperator, bool pPostOperator,
         const TypeUsage& pTypeUsage)
         : mExpression(pExpression)
         , mPostOperator(pPostOperator)
      {
         mType = ExpressionType::UnaryOperation;
         mTypeUsage = pTypeUsage;
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

      ExpressionBinaryOperation(Expression* pLeft, Expression* pRight, const char* pOperator,
         const TypeUsage& pTypeUsage)
         : mLeft(pLeft)
         , mRight(pRight)
      {
         mType = ExpressionType::BinaryOperation;
         mTypeUsage = pTypeUsage;
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
         mTypeUsage = pExpression->getTypeUsage();
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

   struct ExpressionSizeOf : Expression
   {
      TypeUsage mSizeOfTypeUsage;
      Expression* mSizeOfExpression;

      ExpressionSizeOf(const TypeUsage& pTypeUsage)
         : mSizeOfExpression(nullptr)
      {
         mType = ExpressionType::SizeOf;
         mTypeUsage = pTypeUsage;
      }

      virtual ~ExpressionSizeOf()
      {
         if(mSizeOfExpression)
         {
            CflatInvokeDtor(Expression, mSizeOfExpression);
            CflatFree(mSizeOfExpression);
         }
      }
   };

   struct ExpressionCast : Expression
   {
      CastType mCastType;
      Expression* mExpression;

      ExpressionCast(CastType pCastType, const TypeUsage& pTypeUsage, Expression* pExpression)
         : mCastType(pCastType)
         , mExpression(pExpression)
      {
         mType = ExpressionType::Cast;
         mTypeUsage = pTypeUsage;
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

         if(pIfExpression)
         {
            mTypeUsage = pIfExpression->getTypeUsage();
         }
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
         mTypeUsage = pRightValue->getTypeUsage();
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

      void assignTypeUsage(const TypeUsage& pTypeUsageVoid)
      {
         CflatAssert(mFunction);

         if(mFunction->mReturnTypeUsage.mType)
         {
            mTypeUsage = mFunction->mReturnTypeUsage;

            if(!mTypeUsage.isReference())
            {
               CflatSetFlag(mTypeUsage.mFlags, TypeUsageFlags::Const);
            }
         }
         else
         {
            mTypeUsage = pTypeUsageVoid;
         }
      }
   };

   struct ExpressionMethodCall : Expression
   {
      Expression* mMemberAccess;
      CflatSTLVector(Expression*) mArguments;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      MethodUsage mMethodUsage;

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

      void assignTypeUsage(const TypeUsage& pTypeUsageVoid)
      {
         CflatAssert(mMethodUsage.mMethod);

         if(mMethodUsage.mMethod->mReturnTypeUsage.mType)
         {
            mTypeUsage = mMethodUsage.mMethod->mReturnTypeUsage;

            if(!mTypeUsage.isReference())
            {
               CflatSetFlag(mTypeUsage.mFlags, TypeUsageFlags::Const);
            }
         }
         else
         {
            mTypeUsage = pTypeUsageVoid;
         }
      }
   };

   struct ExpressionArrayInitialization : Expression
   {
      TypeUsage mElementTypeUsage;
      CflatSTLVector(Expression*) mValues;

      ExpressionArrayInitialization()
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

      void assignTypeUsage()
      {
         mTypeUsage.mType = mElementTypeUsage.mType;
         mTypeUsage.mArraySize = (uint16_t)mValues.size();
         mTypeUsage.mPointerLevel = mElementTypeUsage.mPointerLevel;
         CflatSetFlag(mTypeUsage.mFlags, TypeUsageFlags::Array);
      }
   };

   struct ExpressionAggregateInitialization : Expression
   {
      CflatSTLVector(Expression*) mValues;

      ExpressionAggregateInitialization(Type* pInitializedType)
      {
         mType = ExpressionType::AggregateInitialization;
         mTypeUsage.mType = pInitializedType;
      }

      virtual ~ExpressionAggregateInitialization()
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
      CflatSTLVector(Expression*) mArguments;
      Method* mConstructor;

      ExpressionObjectConstruction(Type* pObjectType)
         : mConstructor(nullptr)
      {
         mType = ExpressionType::ObjectConstruction;
         mTypeUsage.mType = pObjectType;
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
}


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

#include "Cflat.h"

//
//  AST Types
//
namespace Cflat
{
   enum class ExpressionType
   {
      Value,
      VariableAccess,
      MemberAccess,
      UnaryOperation,
      BinaryOperation,
      Parenthesized,
      AddressOf,
      Conditional,
      ReturnFunctionCall,
      ReturnMethodCall,
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

   struct ExpressionValue : public Expression
   {
      Value mValue;

      ExpressionValue(const Value& pValue)
         : mValue(pValue)
      {
         mType = ExpressionType::Value;
      }
   };

   struct ExpressionVariableAccess : public Expression
   {
      Symbol mVariableName;

      ExpressionVariableAccess(const Symbol& pVariableName)
         : mVariableName(pVariableName)
      {
         mType = ExpressionType::VariableAccess;
      }
   };

   struct ExpressionMemberAccess : public Expression
   {
      CflatSTLVector<Symbol> mSymbols;

      ExpressionMemberAccess()
      {
         mType = ExpressionType::MemberAccess;
      }
   };

   struct ExpressionBinaryOperation : public Expression
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

   struct ExpressionParenthesized : public Expression
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

   struct ExpressionAddressOf : public Expression
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

   struct ExpressionReturnFunctionCall : public Expression
   {
      Symbol mFunctionName;
      CflatSTLVector<Expression*> mArguments;

      ExpressionReturnFunctionCall(const Symbol& pFunctionName)
         : mFunctionName(pFunctionName)
      {
         mType = ExpressionType::ReturnFunctionCall;
      }

      virtual ~ExpressionReturnFunctionCall()
      {
         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };

   struct ExpressionReturnMethodCall : public Expression
   {
      Symbol mMethodName;
      CflatSTLVector<Symbol> mAccessSymbols;
      CflatSTLVector<Expression*> mArguments;

      ExpressionReturnMethodCall(const Symbol& pMethodName)
         : mMethodName(pMethodName)
      {
         mType = ExpressionType::ReturnMethodCall;
      }

      virtual ~ExpressionReturnMethodCall()
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
      Block,
      UsingDirective,
      NamespaceDeclaration,
      VariableDeclaration,
      FunctionDeclaration,
      Assignment,
      Increment,
      Decrement,
      If,
      While,
      For,
      Break,
      Continue,
      VoidFunctionCall,
      VoidMethodCall,
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

   struct StatementBlock : public Statement
   {
      CflatSTLVector<Statement*> mStatements;

      StatementBlock()
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

   struct StatementVariableDeclaration : public Statement
   {
      TypeUsage mTypeUsage;
      Symbol mVariableName;
      Expression* mInitialValue;

      StatementVariableDeclaration(const TypeUsage& pTypeUsage, const Symbol& pVariableName, Expression* pInitialValue)
         : mTypeUsage(pTypeUsage)
         , mVariableName(pVariableName)
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

   struct StatementFunctionDeclaration : public Statement
   {
      TypeUsage mReturnType;
      Symbol mFunctionName;
      CflatSTLVector<Symbol> mParameterNames;
      CflatSTLVector<TypeUsage> mParameterTypes;
      StatementBlock* mBody;

      StatementFunctionDeclaration(const TypeUsage& pReturnType, const Symbol& pFunctionName)
         : mReturnType(pReturnType)
         , mFunctionName(pFunctionName)
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

   struct StatementAssignment : public Statement
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

   struct StatementIncrement : public Statement
   {
      Symbol mVariableName;

      StatementIncrement(const Symbol& pVariableName)
         : mVariableName(pVariableName)
      {
         mType = StatementType::Increment;
      }
   };

   struct StatementDecrement : public Statement
   {
      Symbol mVariableName;

      StatementDecrement(const Symbol& pVariableName)
         : mVariableName(pVariableName)
      {
         mType = StatementType::Decrement;
      }
   };

   struct StatementIf : public Statement
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

   struct StatementWhile : public Statement
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

   struct StatementFor : public Statement
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

   struct StatementBreak : public Statement
   {
      StatementBreak()
      {
         mType = StatementType::Break;
      }
   };

   struct StatementContinue : public Statement
   {
      StatementContinue()
      {
         mType = StatementType::Continue;
      }
   };

   struct StatementVoidFunctionCall : public Statement
   {
      Symbol mFunctionName;
      CflatSTLVector<Expression*> mArguments;

      StatementVoidFunctionCall(const Symbol& pFunctionName)
         : mFunctionName(pFunctionName)
      {
         mType = StatementType::VoidFunctionCall;
      }

      virtual ~StatementVoidFunctionCall()
      {
         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };

   struct StatementVoidMethodCall : public Statement
   {
      ExpressionMemberAccess* mMemberAccess;
      CflatSTLVector<Expression*> mArguments;

      StatementVoidMethodCall(ExpressionMemberAccess* pMemberAccess)
         : mMemberAccess(pMemberAccess)
      {
         mType = StatementType::VoidMethodCall;
      }

      virtual ~StatementVoidMethodCall()
      {
         if(mMemberAccess)
         {
            CflatInvokeDtor(ExpressionMemberAccess, mMemberAccess);
            CflatFree(mMemberAccess);
         }

         for(size_t i = 0u; i < mArguments.size(); i++)
         {
            CflatInvokeDtor(Expression, mArguments[i]);
            CflatFree(mArguments[i]);
         }
      }
   };

   struct StatementReturn : public Statement
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
      "variable redefinition ('%s')",
      "no default constructor defined for the '%s' type",
      "invalid member access operator ('%s' is a pointer)",
      "invalid member access operator ('%s' is not a pointer)",
      "invalid operator for the '%s' type",
      "no member named '%s'",
      "'%s' must be an integer value"
   };
   const size_t kCompileErrorStringsCount = sizeof(kCompileErrorStrings) / sizeof(const char*);

   const char* kRuntimeErrorStrings[] = 
   {
      "null pointer access ('%s')",
      "invalid array index ('%s')",
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
   "namespace", "new", "operator", "private", "protected", "public",
   "reinterpret_cast", "return", "sizeof", "static", "static_cast",
   "struct", "switch", "this", "true", "typedef", "union", "unsigned",
   "using", "virtual", "void", "while"
};
const size_t kCflatKeywordsCount = sizeof(kCflatKeywords) / sizeof(const char*);

Environment::Environment()
{
   static_assert(kCompileErrorStringsCount == (size_t)Environment::CompileError::Count,
      "Missing compile error strings");
   static_assert(kRuntimeErrorStringsCount == (size_t)Environment::RuntimeError::Count,
      "Missing runtime error strings");

   registerBuiltInTypes();
   registerStandardFunctions();
}

Environment::~Environment()
{
   for(FunctionsRegistry::iterator it = mRegisteredFunctions.begin(); it != mRegisteredFunctions.end(); it++)
   {
      CflatSTLVector<Function*>& functions = it->second;

      for(size_t i = 0u; i < functions.size(); i++)
      {
         Function* function = functions[i];
         CflatInvokeDtor(Function, function);
         CflatFree(function);
      }
   }

   mRegisteredFunctions.clear();

   for(TypesRegistry::iterator it = mRegisteredTypes.begin(); it != mRegisteredTypes.end(); it++)
   {
      Type* type = it->second;
      CflatInvokeDtor(Type, type);
      CflatFree(type);
   }

   mRegisteredTypes.clear();
}

uint32_t Environment::hash(const char* pString)
{
   const uint32_t OffsetBasis = 2166136261u;
   const uint32_t FNVPrime = 16777619u;

   uint32_t charIndex = 0u;
   uint32_t hash = OffsetBasis;

   while(pString[charIndex] != '\0')
   {
      hash ^= pString[charIndex++];
      hash *= FNVPrime;
   }

   return hash;
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

void Environment::registerStandardFunctions()
{
   CflatRegisterFunctionReturnParams1(this, size_t,,, strlen, const char*,,);
}

Type* Environment::getType(uint32_t pNameHash)
{
   TypesRegistry::const_iterator it = mRegisteredTypes.find(pNameHash);
   return it != mRegisteredTypes.end() ? it->second : nullptr;
}

Function* Environment::getFunction(uint32_t pNameHash)
{
   FunctionsRegistry::iterator it = mRegisteredFunctions.find(pNameHash);
   return it != mRegisteredFunctions.end() ? it->second.at(0) : nullptr;
}

CflatSTLVector<Function*>* Environment::getFunctions(uint32_t pNameHash)
{
   FunctionsRegistry::iterator it = mRegisteredFunctions.find(pNameHash);
   return it != mRegisteredFunctions.end() ? &it->second : nullptr;
}

TypeUsage Environment::parseTypeUsage(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   TypeUsage typeUsage;
   char baseTypeName[128];

   pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);

   while(tokens[tokenIndex + 1].mLength == 2u && strncmp(tokens[tokenIndex + 1].mStart, "::", 2u) == 0)
   {
      tokenIndex += 2u;
      pContext.mStringBuffer.append("::");
      pContext.mStringBuffer.append(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
   }

   strcpy(baseTypeName, pContext.mStringBuffer.c_str());
   Type* type = getType(baseTypeName);

   if(!type)
   {
      for(size_t i = 0u; i < pContext.mUsingNamespaces.size(); i++)
      {
         pContext.mStringBuffer.assign(pContext.mUsingNamespaces[i]);
         pContext.mStringBuffer.append("::");
         pContext.mStringBuffer.append(baseTypeName);
         type = getType(pContext.mStringBuffer.c_str());

         if(type)
            break;
      }
   }

   if(type)
   {
      pContext.mStringBuffer.clear();

      const bool isConst = tokenIndex > 0u && strncmp(tokens[tokenIndex - 1u].mStart, "const", 5u) == 0;
      const bool isPointer = *tokens[tokenIndex + 1u].mStart == '*';
      const bool isReference = *tokens[tokenIndex + 1u].mStart == '&';

      if(isConst)
      {
         pContext.mStringBuffer.append("const ");
      }

      pContext.mStringBuffer.append(type->mName);

      if(isPointer)
      {
         pContext.mStringBuffer.append("*");
         tokenIndex++;
      }
      else if(isReference)
      {
         pContext.mStringBuffer.append("&");
         tokenIndex++;
      }

      typeUsage = getTypeUsage(pContext.mStringBuffer.c_str());
   }

   return typeUsage;
}

void Environment::throwCompileError(ParsingContext& pContext, CompileError pError,
   const char* pArg1, const char* pArg2)
{
   const Token& token = pContext.mTokens[pContext.mTokenIndex];

   char errorMsg[256];
   sprintf(errorMsg, kCompileErrorStrings[(int)pError], pArg1, pArg2);

   pContext.mErrorMessage.assign("[Compile Error] Line ");
   pContext.mErrorMessage.append(std::to_string(token.mLine));
   pContext.mErrorMessage.append(": ");
   pContext.mErrorMessage.append(errorMsg);
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

      preprocessedCode.push_back(pCode[cursor++]);
   }

   if(preprocessedCode.back() != '\n')
   {
      preprocessedCode.push_back('\n');
   }

   preprocessedCode.shrink_to_fit();
}

void Environment::tokenize(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;

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

         if(strncmp(token.mStart, kCflatKeywords[i], keywordLength) == 0)
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
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Expression* expression = nullptr;

   const size_t tokensCount = pTokenLastIndex - pContext.mTokenIndex + 1u;

   if(tokensCount == 1u)
   {
      if(token.mType == TokenType::Number)
      {
         TypeUsage typeUsage;
         Value* value = (Value*)CflatMalloc(sizeof(Value));

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
               CflatInvokeCtor(Value, value)(typeUsage, &number);
            }
            // double
            else
            {
               typeUsage.mType = getType("double");
               const double number = strtod(numberStr, nullptr);
               CflatInvokeCtor(Value, value)(typeUsage, &number);
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
               CflatInvokeCtor(Value, value)(typeUsage, &number);
            }
            // signed
            else
            {
               typeUsage.mType = getType("int");
               const int number = atoi(numberStr);
               CflatInvokeCtor(Value, value)(typeUsage, &number);
            }
         }

         expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
         CflatInvokeCtor(ExpressionValue, expression)(*value);

         CflatInvokeDtor(Value, value);
         CflatFree(value);
      }
      else if(token.mType == TokenType::String)
      {
         pContext.mStringBuffer.assign(token.mStart + 1, token.mLength - 1u);
         pContext.mStringBuffer[token.mLength - 2u] = '\0';

         const char* string =
            mLiteralStringsPool.push(pContext.mStringBuffer.c_str(), token.mLength - 1u);

         TypeUsage typeUsage = getTypeUsage("const char*");
         Value value(typeUsage, &string);

         expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
         CflatInvokeCtor(ExpressionValue, expression)(value);
      }
      else if(token.mType == TokenType::Identifier)
      {
         // variable access
         pContext.mStringBuffer.assign(token.mStart, token.mLength);
         Symbol identifier(pContext.mStringBuffer.c_str());

         Instance* instance = retrieveInstance(pContext, identifier.mName.c_str());

         if(instance)
         {
            expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
            CflatInvokeCtor(ExpressionVariableAccess, expression)(identifier);  
         }
         else
         {
            throwCompileError(pContext, CompileError::UndefinedVariable, identifier.mName.c_str());
         }
      }
   }
   else
   {
      size_t operatorTokenIndex = 0u;
      uint32_t parenthesisLevel = tokens[tokenIndex].mStart[0] == '(' ? 1u : 0u;

      for(size_t i = tokenIndex + 1u; i < pTokenLastIndex; i++)
      {
         if(tokens[i].mType == TokenType::Operator && parenthesisLevel == 0u)
         {
            operatorTokenIndex = i;
            break;
         }

         if(tokens[i].mStart[0] == '(')
         {
            parenthesisLevel++;
         }
         else if(tokens[i].mStart[0] == ')')
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

         if(typeUsage.mType->mCategory != TypeCategory::BuiltIn)
         {
            pContext.mStringBuffer.assign("operator");
            pContext.mStringBuffer.append(operatorStr);
            Method* operatorMethod = findMethod(typeUsage.mType, pContext.mStringBuffer.c_str());

            if(!operatorMethod)
            {
               const char* typeName = typeUsage.mType->mName.c_str();
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
      else if(token.mType == TokenType::Identifier)
      {
         const Token& nextToken = tokens[tokenIndex + 1u];

         // function call
         if(nextToken.mStart[0] == '(')
         {
            pContext.mStringBuffer.assign(token.mStart, token.mLength);
            Symbol identifier(pContext.mStringBuffer.c_str());

            ExpressionReturnFunctionCall* castedExpression = 
               (ExpressionReturnFunctionCall*)CflatMalloc(sizeof(ExpressionReturnFunctionCall));
            CflatInvokeCtor(ExpressionReturnFunctionCall, castedExpression)(identifier);
            expression = castedExpression;

            tokenIndex++;
            parseFunctionCallArguments(pContext, castedExpression->mArguments);
         }
         // member access
         else if(nextToken.mStart[0] == '.' || strncmp(nextToken.mStart, "->", 2u) == 0)
         {
            ExpressionMemberAccess* castedExpression =
               (ExpressionMemberAccess*)CflatMalloc(sizeof(ExpressionMemberAccess));
            CflatInvokeCtor(ExpressionMemberAccess, castedExpression)();
            expression = castedExpression;

            parseMemberAccessSymbols(pContext, castedExpression->mSymbols);
         }
      }
      else if(token.mType == TokenType::Operator)
      {
         // address of
         if(token.mStart[0] == '&')
         {
            tokenIndex++;

            expression = (ExpressionAddressOf*)CflatMalloc(sizeof(ExpressionAddressOf));
            CflatInvokeCtor(ExpressionAddressOf, expression)
               (parseExpression(pContext, findClosureTokenIndex(pContext, ' ', ';') - 1u));
            tokenIndex++;
         }
      }
   }

   return expression;
}

size_t Environment::findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
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
         Instance* instance = retrieveInstance(pContext, expression->mVariableName.mName.c_str());
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
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Pointer);
      }
      break;
   case ExpressionType::ReturnFunctionCall:
      {
         ExpressionReturnFunctionCall* expression = static_cast<ExpressionReturnFunctionCall*>(pExpression);
         Function* function = getFunction(expression->mFunctionName.mName.c_str());
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
   CflatSTLVector<Token>& tokens = pContext.mTokens;
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
         // usign namespace
         if(strncmp(token.mStart, "using", 5u) == 0)
         {
            tokenIndex++;
            const Token& nextToken = tokens[tokenIndex];

            if(strncmp(nextToken.mStart, "namespace", 9u) == 0)
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

               pContext.mUsingNamespaces.push_back(pContext.mStringBuffer);
            }

            break;
         }
         // if
         else if(strncmp(token.mStart, "if", 2u) == 0)
         {
            tokenIndex++;
            statement = parseStatementIf(pContext);
         }
         // while
         else if(strncmp(token.mStart, "while", 5u) == 0)
         {
            tokenIndex++;
            statement = parseStatementWhile(pContext);
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
            const Symbol identifier(pContext.mStringBuffer.c_str());

            tokenIndex++;
            const Token& nextToken = tokens[tokenIndex];

            if(nextToken.mType != TokenType::Operator && nextToken.mType != TokenType::Punctuation)
            {
               pContext.mStringBuffer.assign(token.mStart, token.mLength);
               throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
               return nullptr;
            }

            // variable/const declaration
            if(nextToken.mStart[0] == '=' || nextToken.mStart[0] == ';')
            {
               Instance* existingInstance = retrieveInstance(pContext, identifier.mName.c_str());

               if(!existingInstance)
               {
                  Expression* initialValue = nullptr;

                  if(nextToken.mStart[0] == '=')
                  {
                     tokenIndex++;
                     initialValue = parseExpression(pContext, findClosureTokenIndex(pContext, ' ', ';') - 1u);
                  }
                  else if(typeUsage.mType->mCategory != TypeCategory::BuiltIn && !typeUsage.isPointer())
                  {
                     Struct* type = static_cast<Struct*>(typeUsage.mType);
                     Method* defaultCtor = type->getDefaultConstructor();

                     if(!defaultCtor)
                     {
                        throwCompileError(pContext, CompileError::NoDefaultConstructor, type->mName.c_str());
                        break;
                     }
                  }

                  registerInstance(pContext, typeUsage, identifier.mName.c_str());

                  statement = (StatementVariableDeclaration*)CflatMalloc(sizeof(StatementVariableDeclaration));
                  CflatInvokeCtor(StatementVariableDeclaration, statement)
                     (typeUsage, identifier, initialValue);
               }
               else
               {
                  throwCompileError(pContext, CompileError::VariableRedefinition, identifier.mName.c_str());
               }
            }
            // function declaration
            else if(nextToken.mStart[0] == '(')
            {
               tokenIndex--;
               statement = parseStatementFunctionDeclaration(pContext);
            }

            break;
         }
         // assignment/variable access/function call
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
                     statement = parseStatementVoidFunctionCall(pContext);
                  }
                  // member access
                  else
                  {
                     Expression* memberAccess =
                        parseExpression(pContext, findClosureTokenIndex(pContext, ' ', ';') - 1u);

                     if(memberAccess)
                     {
                        // method call
                        if(tokens[tokenIndex].mStart[0] == '(')
                        {
                           statement = parseStatementVoidMethodCall(pContext, memberAccess);
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
                     // increment
                     if(strncmp(nextToken.mStart, "++", 2u) == 0)
                     {
                        if(isInteger(*instance->mTypeUsage.mType))
                        {
                           statement = (StatementIncrement*)CflatMalloc(sizeof(StatementIncrement));
                           CflatInvokeCtor(StatementIncrement, statement)(variableName);
                           tokenIndex += 2u;
                        }
                        else
                        {
                           throwCompileError(pContext, CompileError::NonIntegerValue, variableName);
                        }
                     }
                     // decrement
                     else if(strncmp(nextToken.mStart, "--", 2u) == 0)
                     {
                        if(isInteger(*instance->mTypeUsage.mType))
                        {
                           statement = (StatementDecrement*)CflatMalloc(sizeof(StatementDecrement));
                           CflatInvokeCtor(StatementDecrement, statement)(variableName);
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
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   if(token.mStart[0] != '{')
      return nullptr;

   StatementBlock* block = (StatementBlock*)CflatMalloc(sizeof(StatementBlock));
   CflatInvokeCtor(StatementBlock, block)();

   incrementScopeLevel(pContext);

   while(tokens[tokenIndex].mStart[0] != '}')
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

StatementFunctionDeclaration* Environment::parseStatementFunctionDeclaration(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   const Token& previousToken = tokens[tokenIndex - 1u];

   pContext.mStringBuffer.assign(previousToken.mStart, previousToken.mLength);
   TypeUsage returnType = getTypeUsage(pContext.mStringBuffer.c_str());

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   const Symbol functionName(pContext.mStringBuffer.c_str());

   StatementFunctionDeclaration* statement =
      (StatementFunctionDeclaration*)CflatMalloc(sizeof(StatementFunctionDeclaration));
   CflatInvokeCtor(StatementFunctionDeclaration, statement)(returnType, functionName);

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
      Symbol parameterName(pContext.mStringBuffer.c_str());
      statement->mParameterNames.push_back(parameterName);
      tokenIndex++;

      Instance* parameterInstance =
         registerInstance(pContext, parameterType, parameterName.mName.c_str());
      parameterInstance->mScopeLevel++;
   }

   statement->mBody = parseStatementBlock(pContext);

   return statement;
}

StatementAssignment* Environment::parseStatementAssignment(ParsingContext& pContext, size_t pOperatorTokenIndex)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   Expression* leftValue = parseExpression(pContext, pOperatorTokenIndex - 1u);

   const Token& operatorToken = pContext.mTokens[pOperatorTokenIndex];
   CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

   tokenIndex = pOperatorTokenIndex + 1u;
   Expression* rightValue = parseExpression(pContext, findClosureTokenIndex(pContext, ' ', ';') - 1u);

   StatementAssignment* statement = (StatementAssignment*)CflatMalloc(sizeof(StatementAssignment));
   CflatInvokeCtor(StatementAssignment, statement)(leftValue, rightValue, operatorStr.c_str());

   return statement;
}

StatementIf* Environment::parseStatementIf(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      return nullptr;
   }

   tokenIndex++;
   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, '(', ')');
   Expression* condition = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* ifStatement = parseStatement(pContext);
   tokenIndex++;

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

StatementWhile* Environment::parseStatementWhile(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
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

StatementFor* Environment::parseStatementFor(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mStart[0] != '(')
   {
      return nullptr;
   }

   incrementScopeLevel(pContext);

   tokenIndex++;
   const size_t initializationClosureTokenIndex = findClosureTokenIndex(pContext, ' ', ';');
   Statement* initialization = parseStatement(pContext);
   tokenIndex = initializationClosureTokenIndex + 1u;

   const size_t conditionClosureTokenIndex = findClosureTokenIndex(pContext, ' ', ';');
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
   CflatSTLVector<Token>& tokens = pContext.mTokens;
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
   CflatSTLVector<Token>& tokens = pContext.mTokens;
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

StatementVoidFunctionCall* Environment::parseStatementVoidFunctionCall(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   Symbol functionName(pContext.mStringBuffer.c_str());
   tokenIndex++;

   StatementVoidFunctionCall* statement =
      (StatementVoidFunctionCall*)CflatMalloc(sizeof(StatementVoidFunctionCall));
   CflatInvokeCtor(StatementVoidFunctionCall, statement)(functionName);

   parseFunctionCallArguments(pContext, statement->mArguments);

   return statement;
}

StatementVoidMethodCall* Environment::parseStatementVoidMethodCall(
   ParsingContext& pContext, Expression* pMemberAccess)
{
   CflatAssert(pMemberAccess->getType() == ExpressionType::MemberAccess);
   ExpressionMemberAccess* memberAccess = static_cast<ExpressionMemberAccess*>(pMemberAccess);

   StatementVoidMethodCall* statement =
      (StatementVoidMethodCall*)CflatMalloc(sizeof(StatementVoidMethodCall));
   CflatInvokeCtor(StatementVoidMethodCall, statement)(memberAccess);

   parseFunctionCallArguments(pContext, statement->mArguments);

   return statement;
}

StatementReturn* Environment::parseStatementReturn(ParsingContext& pContext)
{
   Expression* expression = parseExpression(pContext, findClosureTokenIndex(pContext, ' ', ';') - 1u);

   StatementReturn* statement = (StatementReturn*)CflatMalloc(sizeof(StatementReturn));
   CflatInvokeCtor(StatementReturn, statement)(expression);

   return statement;
}

bool Environment::parseFunctionCallArguments(ParsingContext& pContext, CflatSTLVector<Expression*>& pArguments)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   while(tokens[tokenIndex++].mStart[0] != ')')
   {
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, ' ', ')');
      const size_t separatorTokenIndex = findClosureTokenIndex(pContext, ' ', ',');

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

bool Environment::parseMemberAccessSymbols(ParsingContext& pContext, CflatSTLVector<Symbol>& pSymbols)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   TypeUsage typeUsage;
   bool anyRemainingMemberAccessSymbols = true;

   while(anyRemainingMemberAccessSymbols)
   {
      const bool memberAccess = tokens[tokenIndex + 1u].mStart[0] == '.';
      const bool ptrMemberAccess = !memberAccess && strncmp(tokens[tokenIndex + 1u].mStart, "->", 2u) == 0;

      anyRemainingMemberAccessSymbols = memberAccess || ptrMemberAccess;

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      pSymbols.push_back(pContext.mStringBuffer.c_str());

      if(pSymbols.size() == 1u)
      {
         Instance* instance = retrieveInstance(pContext, pSymbols.back().mName.c_str());
         typeUsage = instance->mValue.mTypeUsage;
      }
      else if(tokens[tokenIndex + 1u].mStart[0] != '(')
      {
         const char* memberName = pSymbols.back().mName.c_str();
         Struct* type = static_cast<Struct*>(typeUsage.mType);
         Member* member = nullptr;

         for(size_t j = 0u; j < type->mMembers.size(); j++)
         {
            if(strcmp(type->mMembers[j].mName.c_str(), memberName) == 0)
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
            throwCompileError(pContext, CompileError::MissingMember, memberName);
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
            throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorPtr, pSymbols.back().mName.c_str());
            return false;
         }
      }
      else
      {
         if(ptrMemberAccess)
         {
            throwCompileError(pContext, CompileError::InvalidMemberAccessOperatorNonPtr, pSymbols.back().mName.c_str());
            return false;
         }
      }

      tokenIndex++;

      if(anyRemainingMemberAccessSymbols)
      {
         tokenIndex++;
      }
   }

   return true;
}

Environment::Instance* Environment::registerInstance(Context& pContext, const TypeUsage& pTypeUsage, const char* pName)
{
   Instance instance;
   instance.mTypeUsage = pTypeUsage;
   instance.mValue.init(pTypeUsage);
   instance.mNameHash = hash(pName);
   instance.mScopeLevel = pContext.mScopeLevel;

   pContext.mInstances.push_back(instance);

   return &pContext.mInstances.back();
}

Environment::Instance* Environment::retrieveInstance(Context& pContext, const char* pName)
{
   const uint32_t nameHash = hash(pName);
   Instance* instance = nullptr;

   for(int i = (int)pContext.mInstances.size() - 1; i >= 0; i--)
   {
      if(pContext.mInstances[i].mNameHash == nameHash)
      {
         instance = &pContext.mInstances[i];
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
   pContext.mScopeLevel--;

   while(!pContext.mInstances.empty() && pContext.mInstances.back().mScopeLevel > pContext.mScopeLevel)
   {
      pContext.mInstances.pop_back();
   }
}

void Environment::throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg)
{
   char errorMsg[256];
   sprintf(errorMsg, kRuntimeErrorStrings[(int)pError], pArg);

   pContext.mErrorMessage.assign("[Runtime Error] Line ");
   pContext.mErrorMessage.append(std::to_string(pContext.mCurrentLine));
   pContext.mErrorMessage.append(": ");
   pContext.mErrorMessage.append(errorMsg);
}

void Environment::getValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue)
{
   switch(pExpression->getType())
   {
   case ExpressionType::Value:
      {
         ExpressionValue* expression = static_cast<ExpressionValue*>(pExpression);

         pOutValue->init(expression->mValue.mTypeUsage);
         pOutValue->set(expression->mValue.mValueBuffer);
      }
      break;
   case ExpressionType::VariableAccess:
      {
         ExpressionVariableAccess* expression = static_cast<ExpressionVariableAccess*>(pExpression);
         Instance* instance = retrieveInstance(pContext, expression->mVariableName.mName.c_str());

         pOutValue->init(instance->mValue.mTypeUsage);
         pOutValue->set(instance->mValue.mValueBuffer);
      }
      break;
   case ExpressionType::BinaryOperation:
      {
         ExpressionBinaryOperation* expression = static_cast<ExpressionBinaryOperation*>(pExpression);

         Value leftValue;
         getValue(pContext, expression->mLeft, &leftValue);
         Value rightValue;
         getValue(pContext, expression->mRight, &rightValue);

         applyBinaryOperator(pContext, leftValue, rightValue, expression->mOperator, pOutValue);
      }
      break;
   case ExpressionType::Parenthesized:
      {
         ExpressionParenthesized* expression = static_cast<ExpressionParenthesized*>(pExpression);
         getValue(pContext, expression->mExpression, pOutValue);
      }
      break;
   case ExpressionType::AddressOf:
      {
         ExpressionAddressOf* expression = static_cast<ExpressionAddressOf*>(pExpression);

         if(expression->mExpression->getType() == ExpressionType::VariableAccess)
         {
            ExpressionVariableAccess* variableAccess =
               static_cast<ExpressionVariableAccess*>(expression->mExpression);
            Instance* instance = retrieveInstance(pContext, variableAccess->mVariableName.mName.c_str());
            getAddressOfValue(pContext, &instance->mValue, pOutValue);
         }
      }
      break;
   case ExpressionType::ReturnFunctionCall:
      {
         ExpressionReturnFunctionCall* expression = static_cast<ExpressionReturnFunctionCall*>(pExpression);
         Function* function = getFunction(expression->mFunctionName.mName.c_str());

         CflatSTLVector<Value> argumentValues;
         getArgumentValues(pContext, expression->mArguments, argumentValues);

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
   default:
      break;
   }
}

void Environment::getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue)
{
   if(pExpression->getType() == ExpressionType::VariableAccess)
   {
      ExpressionVariableAccess* variableAccess = static_cast<ExpressionVariableAccess*>(pExpression);
      Instance* instance = retrieveInstance(pContext, variableAccess->mVariableName.mName.c_str());
      pOutValue->mTypeUsage = instance->mValue.mTypeUsage;
      pOutValue->mValueBuffer = instance->mValue.mValueBuffer;
   }
   else if(pExpression->getType() == ExpressionType::MemberAccess)
   {
      ExpressionMemberAccess* memberAccess = static_cast<ExpressionMemberAccess*>(pExpression);
      Instance* instance = retrieveInstance(pContext, memberAccess->mSymbols[0].mName.c_str());
      pOutValue->mTypeUsage = instance->mValue.mTypeUsage;
      pOutValue->mValueBuffer = instance->mValue.mValueBuffer;

      for(size_t i = 1u; i < memberAccess->mSymbols.size(); i++)
      {
         const char* memberName = memberAccess->mSymbols[i].mName.c_str();
         Struct* type = static_cast<Struct*>(pOutValue->mTypeUsage.mType);
         Member* member = nullptr;

         for(size_t j = 0u; j < type->mMembers.size(); j++)
         {
            if(strcmp(type->mMembers[j].mName.c_str(), memberName) == 0)
            {
               member = &type->mMembers[j];
               break;
            }
         }

         // the symbol is a member
         if(member)
         {
            char* instanceDataPtr = pOutValue->mTypeUsage.isPointer()
               ? CflatRetrieveValue(pOutValue, char*,,)
               : pOutValue->mValueBuffer;

            pOutValue->mTypeUsage = member->mTypeUsage;
            pOutValue->mValueBuffer = instanceDataPtr + member->mOffset;
         }
         // the symbol is a method
         else
         {
            break;
         }
      }
   }
}

void Environment::getAddressOfValue(ExecutionContext& pContext, Value* pInstanceDataValue, Value* pOutValue)
{
   pContext.mStringBuffer.assign(pInstanceDataValue->mTypeUsage.mType->mName);
   pContext.mStringBuffer.append("*");

   TypeUsage pointerTypeUsage = getTypeUsage(pContext.mStringBuffer.c_str());

   pOutValue->init(pointerTypeUsage);
   pOutValue->set(&pInstanceDataValue->mValueBuffer);
}

void Environment::getArgumentValues(ExecutionContext& pContext,
   const CflatSTLVector<Expression*>& pExpressions, CflatSTLVector<Value>& pValues)
{
   pValues.resize(pExpressions.size());

   for(size_t i = 0u; i < pExpressions.size(); i++)
   {
      getValue(pContext, pExpressions[i], &pValues[i]);
   }
}

void Environment::applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
   const char* pOperator, Value* pOutValue)
{
   Type* leftType = pLeft.mTypeUsage.mType;

   if(leftType->mCategory == TypeCategory::BuiltIn)
   {
      const bool integerValues = isInteger(*leftType);

      const int64_t leftValueAsInteger = getValueAsInteger(pLeft);
      const int64_t rightValueAsInteger = getValueAsInteger(pRight);
      const double leftValueAsDecimal = getValueAsDecimal(pLeft);
      const double rightValueAsDecimal = getValueAsDecimal(pRight);

      if(strcmp(pOperator, "==") == 0)
      {
         const bool result = leftValueAsInteger == rightValueAsInteger;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "!=") == 0)
      {
         const bool result = leftValueAsInteger != rightValueAsInteger;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "<") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger < rightValueAsInteger
            : leftValueAsDecimal < rightValueAsDecimal;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, ">") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger > rightValueAsInteger
            : leftValueAsDecimal > rightValueAsDecimal;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "<=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger <= rightValueAsInteger
            : leftValueAsDecimal <= rightValueAsDecimal;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, ">=") == 0)
      {
         const bool result = integerValues
            ? leftValueAsInteger >= rightValueAsInteger
            : leftValueAsDecimal >= rightValueAsDecimal;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "&&") == 0)
      {
         const bool result = leftValueAsInteger && rightValueAsInteger;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "||") == 0)
      {
         const bool result = leftValueAsInteger || rightValueAsInteger;

         pOutValue->init(getTypeUsage("bool"));
         pOutValue->set(&result);
      }
      else if(strcmp(pOperator, "+") == 0)
      {
         pOutValue->init(pLeft.mTypeUsage);

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
         pOutValue->init(pLeft.mTypeUsage);

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
         pOutValue->init(pLeft.mTypeUsage);

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
         pOutValue->init(pLeft.mTypeUsage);

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

      Method* operatorMethod = findMethod(leftType, pContext.mStringBuffer.c_str());
      CflatAssert(operatorMethod);

      Value thisPtrValue;
      getAddressOfValue(pContext, &const_cast<Cflat::Value&>(pLeft), &thisPtrValue);

      pOutValue->init(operatorMethod->mReturnTypeUsage);

      CflatSTLVector<Value> args;
      args.push_back(pRight);

      operatorMethod->execute(thisPtrValue, args, pOutValue);
   }
}

void Environment::performAssignment(ExecutionContext& pContext, Value* pValue,
   const char* pOperator, Value* pInstanceDataValue)
{
   if(strcmp(pOperator, "=") == 0)
   {
      memcpy(pInstanceDataValue->mValueBuffer, pValue->mValueBuffer, pValue->mTypeUsage.getSize());
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

bool Environment::isInteger(const Type& pType)
{
   return pType.mCategory == TypeCategory::BuiltIn &&
      (strncmp(pType.mName.c_str(), "int", 3u) == 0 ||
       strncmp(pType.mName.c_str(), "uint", 4u) == 0 ||
       strcmp(pType.mName.c_str(), "char") == 0 ||
       strcmp(pType.mName.c_str(), "bool") == 0);
}

bool Environment::isDecimal(const Type& pType)
{
   return pType.mCategory == TypeCategory::BuiltIn &&
      (strcmp(pType.mName.c_str(), "float") == 0 ||
       strcmp(pType.mName.c_str(), "double") == 0);
}

int64_t Environment::getValueAsInteger(const Value& pValue)
{
   int64_t value = 0u;

   if(pValue.mTypeUsage.mType->mSize == 4u)
   {
      value = (int64_t)CflatRetrieveValue(&pValue, int32_t,,);
   }
   else if(pValue.mTypeUsage.mType->mSize == 8u)
   {
      value = CflatRetrieveValue(&pValue, int64_t,,);
   }
   else if(pValue.mTypeUsage.mType->mSize == 2u)
   {
      value = (int64_t)CflatRetrieveValue(&pValue, int16_t,,);
   }
   else if(pValue.mTypeUsage.mType->mSize == 1u)
   {
      value = (int64_t)CflatRetrieveValue(&pValue, int8_t,,);
   }

   return value;
}

double Environment::getValueAsDecimal(const Value& pValue)
{
   double value = 0.0;

   if(pValue.mTypeUsage.mType->mSize == 4u)
   {
      value = (double)CflatRetrieveValue(&pValue, float,,);
   }
   else if(pValue.mTypeUsage.mType->mSize == 8u)
   {
      value = CflatRetrieveValue(&pValue, double,,);
   }

   return value;
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

Method* Environment::findMethod(Type* pType, const char* pMethodName)
{
   CflatAssert(pType->mCategory != TypeCategory::BuiltIn);

   Method* method = nullptr;
   Struct* type = static_cast<Struct*>(pType);

   for(size_t i = 0u; i < type->mMethods.size(); i++)
   {
      if(strcmp(type->mMethods[i].mName.c_str(), pMethodName) == 0)
      {
         method = &type->mMethods[i];
         break;
      }
   }

   return method;
}

void Environment::execute(ExecutionContext& pContext, Statement* pStatement)
{
   pContext.mCurrentLine = pStatement->mLine;

   switch(pStatement->getType())
   {
   case StatementType::Block:
      {
         incrementScopeLevel(pContext);

         StatementBlock* statement = static_cast<StatementBlock*>(pStatement);

         for(size_t i = 0u; i < statement->mStatements.size(); i++)
         {
            execute(pContext, statement->mStatements[i]);

            if(pContext.mJumpStatement != JumpStatement::None)
            {
               break;
            }
         }

         decrementScopeLevel(pContext);
      }
      break;
   case StatementType::UsingDirective:
      {
      }
      break;
   case StatementType::NamespaceDeclaration:
      {
      }
      break;
   case StatementType::VariableDeclaration:
      {
         StatementVariableDeclaration* statement = static_cast<StatementVariableDeclaration*>(pStatement);
         Instance* instance =
            registerInstance(pContext, statement->mTypeUsage, statement->mVariableName.mName.c_str());

         // if there is an assignment in the declaration, set the value
         if(statement->mInitialValue)
         {
            getValue(pContext, statement->mInitialValue, &instance->mValue);
         }
         // otherwise, call the default constructor if the type is a struct or a class
         else if(instance->mTypeUsage.mType->mCategory != TypeCategory::BuiltIn &&
            !instance->mTypeUsage.isPointer())
         {
            instance->mValue.mTypeUsage = instance->mTypeUsage;
            Value thisPtr;
            getAddressOfValue(pContext, &instance->mValue, &thisPtr);

            Struct* type = static_cast<Struct*>(instance->mTypeUsage.mType);
            Method* defaultCtor = type->getDefaultConstructor();
            CflatSTLVector<Value> args;
            defaultCtor->execute(thisPtr, args, nullptr);
         }
      }
      break;
   case StatementType::FunctionDeclaration:
      {
         StatementFunctionDeclaration* statement = static_cast<StatementFunctionDeclaration*>(pStatement);
         Function* function = registerFunction(statement->mFunctionName.mName.c_str());
         function->mReturnTypeUsage = statement->mReturnType;

         for(size_t i = 0u; i < statement->mParameterTypes.size(); i++)
         {
            function->mParameters.push_back(statement->mParameterTypes[i]);
         }

         if(statement->mBody)
         {
            function->execute =
               [this, &pContext, function, statement](CflatSTLVector<Value>& pArguments, Value* pOutReturnValue)
            {
               CflatAssert(function->mParameters.size() == pArguments.size());
               
               for(size_t i = 0u; i < pArguments.size(); i++)
               {
                  const TypeUsage parameterType = statement->mParameterTypes[i];
                  const char* parameterName = statement->mParameterNames[i].mName.c_str();
                  Instance* argumentInstance = registerInstance(pContext, parameterType, parameterName);
                  argumentInstance->mScopeLevel++;
                  argumentInstance->mValue.set(pArguments[i].mValueBuffer);
               }

               execute(pContext, statement->mBody);

               if(pOutReturnValue)
               {
                  pOutReturnValue->init(pContext.mReturnValue.mTypeUsage);
                  pOutReturnValue->set(pContext.mReturnValue.mValueBuffer);
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
         getValue(pContext, statement->mRightValue, &rightValue);

         performAssignment(pContext, &rightValue, statement->mOperator, &instanceDataValue);
         instanceDataValue.mValueBuffer = nullptr;
      }
      break;
   case StatementType::Increment:
      {
         StatementIncrement* statement = static_cast<StatementIncrement*>(pStatement);
         Instance* instance = retrieveInstance(pContext, statement->mVariableName.mName.c_str());
         CflatAssert(instance);
         setValueAsInteger(getValueAsInteger(instance->mValue) + 1u, &instance->mValue);
      }
      break;
   case StatementType::Decrement:
      {
         StatementDecrement* statement = static_cast<StatementDecrement*>(pStatement);
         Instance* instance = retrieveInstance(pContext, statement->mVariableName.mName.c_str());
         CflatAssert(instance);
         setValueAsInteger(getValueAsInteger(instance->mValue) - 1u, &instance->mValue);
      }
      break;
   case StatementType::If:
      {
         StatementIf* statement = static_cast<StatementIf*>(pStatement);

         Value conditionValue;
         getValue(pContext, statement->mCondition, &conditionValue);
         const bool conditionMet = CflatRetrieveValue(&conditionValue, bool,,);

         if(conditionMet)
         {
            execute(pContext, statement->mIfStatement);
         }
         else if(statement->mElseStatement)
         {
            execute(pContext, statement->mElseStatement);
         }
      }
      break;
   case StatementType::While:
      {
         StatementWhile* statement = static_cast<StatementWhile*>(pStatement);

         Value conditionValue;
         getValue(pContext, statement->mCondition, &conditionValue);
         bool conditionMet = CflatRetrieveValue(&conditionValue, bool,,);

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

            getValue(pContext, statement->mCondition, &conditionValue);
            conditionMet = CflatRetrieveValue(&conditionValue, bool,,);
         }
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

         const bool defaultConditionValue = true;
         Value conditionValue(getTypeUsage("bool"), &defaultConditionValue);
         bool conditionMet = defaultConditionValue;

         if(statement->mCondition)
         {
            getValue(pContext, statement->mCondition, &conditionValue);
            conditionMet = CflatRetrieveValue(&conditionValue, bool,,);
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
               getValue(pContext, statement->mCondition, &conditionValue);
               conditionMet = CflatRetrieveValue(&conditionValue, bool,,);
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
            getValue(pContext, statement->mExpression, &pContext.mReturnValue);
         }

         pContext.mJumpStatement = JumpStatement::Return;
      }
      break;
   case StatementType::VoidFunctionCall:
      {
         StatementVoidFunctionCall* statement = static_cast<StatementVoidFunctionCall*>(pStatement);
         Function* function = getFunction(statement->mFunctionName.mName.c_str());

         CflatSTLVector<Value> argumentValues;
         getArgumentValues(pContext, statement->mArguments, argumentValues);

         function->execute(argumentValues, nullptr);
      }
      break;
   case StatementType::VoidMethodCall:
      {
         StatementVoidMethodCall* statement = static_cast<StatementVoidMethodCall*>(pStatement);

         Value instanceDataValue;
         getInstanceDataValue(pContext, statement->mMemberAccess, &instanceDataValue);

         const char* methodName = statement->mMemberAccess->mSymbols.back().mName.c_str();
         Method* method = findMethod(instanceDataValue.mTypeUsage.mType, methodName);
         CflatAssert(method);

         Value thisPtr;

         if(instanceDataValue.mTypeUsage.isPointer())
         {
            thisPtr.init(instanceDataValue.mTypeUsage);
            thisPtr.set(instanceDataValue.mValueBuffer);
         }
         else
         {
            getAddressOfValue(pContext, &instanceDataValue, &thisPtr);
         }

         pContext.mReturnValue.init(method->mReturnTypeUsage);

         CflatSTLVector<Value> argumentValues;
         getArgumentValues(pContext, statement->mArguments, argumentValues);

         method->execute(thisPtr, argumentValues, &pContext.mReturnValue);
         instanceDataValue.mValueBuffer = nullptr;
      }
      break;
   default:
      break;
   }
}

Type* Environment::getType(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   return getType(nameHash);
}

TypeUsage Environment::getTypeUsage(const char* pTypeName)
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
      CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Pointer);
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

Function* Environment::registerFunction(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   Function* function = (Function*)CflatMalloc(sizeof(Function));
   CflatInvokeCtor(Function, function)(pName);
   FunctionsRegistry::iterator it = mRegisteredFunctions.find(nameHash);

   if(it == mRegisteredFunctions.end())
   {
      CflatSTLVector<Function*> functions;
      functions.push_back(function);
      mRegisteredFunctions[nameHash] = functions;
   }
   else
   {
      it->second.push_back(function);
   }

   return function;
}

Function* Environment::getFunction(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   return getFunction(nameHash);
}

CflatSTLVector<Function*>* Environment::getFunctions(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   return getFunctions(nameHash);
}

void Environment::setVariable(const TypeUsage& pTypeUsage, const char* pName, const Value& pValue)
{
   Instance* instance = retrieveInstance(mExecutionContext, pName);

   if(!instance)
   {
      instance = registerInstance(mExecutionContext, pTypeUsage, pName);
   }

   instance->mValue.init(pTypeUsage);
   instance->mValue.set(pValue.mValueBuffer);
}

Value* Environment::getVariable(const char* pName)
{
   Instance* instance = retrieveInstance(mExecutionContext, pName);
   return instance ? &instance->mValue : nullptr;
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

   ParsingContext parsingContext;

   preprocess(parsingContext, pCode);
   tokenize(parsingContext);
   parse(parsingContext, program);

   if(!parsingContext.mErrorMessage.empty())
   {
      mErrorMessage.assign(parsingContext.mErrorMessage);
      return false;
   }
   
   // make sure that there is enough space in the array of instances to avoid
   // memory reallocations, which would potentially invalidate cached pointers
   if(parsingContext.mInstances.capacity() > mExecutionContext.mInstances.capacity())
   {
      mExecutionContext.mInstances.reserve(parsingContext.mInstances.capacity());
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

const char* Environment::getErrorMessage()
{
   return mErrorMessage.empty() ? nullptr : mErrorMessage.c_str();
}

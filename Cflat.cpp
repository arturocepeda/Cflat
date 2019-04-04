
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
      For,
      While,
      VoidFunctionCall,
      VoidMethodCall,
      Break,
      Return
   };

   struct Statement
   {
   protected:
      StatementType mType;

      Statement()
      {
      }

   public:
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
}


//
//  Environment
//
using namespace Cflat;

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

void Environment::throwCompileError(ParsingContext& pContext, const char* pErrorMsg, uint16_t pLineNumber)
{
   pContext.mErrorMessage.assign("Line ");
   pContext.mErrorMessage.append(std::to_string(pLineNumber));
   pContext.mErrorMessage.append(": ");
   pContext.mErrorMessage.append(pErrorMsg);
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
         if(token.mStart[0] == kCflatPunctuation[i][0])
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
   mLiteralStringsPool.reset();
   pContext.mInstances.clear();

   size_t& tokenIndex = pContext.mTokenIndex;

   for(tokenIndex = 0u; tokenIndex < pContext.mTokens.size(); tokenIndex++)
   {
      Statement* statement = parseStatement(pContext);

      if(statement)
      {
         pProgram.push_back(statement);
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

         expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
         CflatInvokeCtor(ExpressionVariableAccess, expression)(identifier);
      }
   }
   else
   {
      size_t operatorTokenIndex = 0u;
      uint32_t parenthesisLevel = 0u;

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
         
         const Token& operatorToken = pContext.mTokens[operatorTokenIndex];
         CflatSTLString operatorStr(operatorToken.mStart, operatorToken.mLength);

         tokenIndex = operatorTokenIndex + 1u;
         Expression* right = parseExpression(pContext, pTokenLastIndex);
         tokenIndex++;

         expression = (ExpressionBinaryOperation*)CflatMalloc(sizeof(ExpressionBinaryOperation));
         CflatInvokeCtor(ExpressionBinaryOperation, expression)(left, right, operatorStr.c_str());
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

   return closureTokenIndex;
}

Statement* Environment::parseStatement(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Statement* statement = nullptr;

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
         // for
         else if(strncmp(token.mStart, "for", 3u) == 0)
         {
            //TODO
         }
         // while
         else if(strncmp(token.mStart, "while", 5u) == 0)
         {
            //TODO
         }
         // function declaration
         else if(strncmp(token.mStart, "void", 4u) == 0)
         {
            tokenIndex++;
            statement = parseStatementFunctionDeclaration(pContext);
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
               throwCompileError(pContext, "unexpected symbol", nextToken.mLine);
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
                        pContext.mStringBuffer.assign("no default constructor defined for the '");
                        pContext.mStringBuffer.append(type->mName.c_str());
                        pContext.mStringBuffer.append("' type");
                        throwCompileError(pContext, pContext.mStringBuffer.c_str(), token.mLine);
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
                  pContext.mStringBuffer.assign("variable redefinition (");
                  pContext.mStringBuffer.append(identifier.mName.c_str());
                  pContext.mStringBuffer.append(")");
                  throwCompileError(pContext, pContext.mStringBuffer.c_str(), token.mLine);
               }
            }
            // function declaration
            else if(nextToken.mStart[0] == '(')
            {
               statement = parseStatementFunctionDeclaration(pContext);
            }

            break;
         }
         // variable access/function call
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
                     // assignment
                     else
                     {
                        //TODO
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
                     statement = (StatementIncrement*)CflatMalloc(sizeof(StatementIncrement));
                     CflatInvokeCtor(StatementIncrement, statement)(variableName);
                     tokenIndex++;
                  }
                  // decrement
                  else if(strncmp(nextToken.mStart, "--", 2u) == 0)
                  {
                     statement = (StatementDecrement*)CflatMalloc(sizeof(StatementDecrement));
                     CflatInvokeCtor(StatementDecrement, statement)(variableName);
                     tokenIndex++;
                  }
               }
               else
               {
                  pContext.mStringBuffer.assign("undefined variable (");
                  pContext.mStringBuffer.append(token.mStart, token.mLength);
                  pContext.mStringBuffer.append(")");
                  throwCompileError(pContext, pContext.mStringBuffer.c_str(), token.mLine);
               }
            }
            else
            {
               pContext.mStringBuffer.assign("unexpected symbol after '");
               pContext.mStringBuffer.append(token.mStart, token.mLength);
               pContext.mStringBuffer.append("'");
               throwCompileError(pContext, pContext.mStringBuffer.c_str(), nextToken.mLine);
            }
         }
      }
      break;
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

   while(tokens[tokenIndex].mStart[0] != '}')
   {
      tokenIndex++;
      Statement* statement = parseStatement(pContext);

      if(statement)
      {
         block->mStatements.push_back(statement);
      }
   }

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
      TypeUsage parameterType = parseTypeUsage(pContext);
      statement->mParameterTypes.push_back(parameterType);
      tokenIndex++;

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      Symbol parameterName(pContext.mStringBuffer.c_str());
      statement->mParameterNames.push_back(parameterName);
      tokenIndex++;
   }

   statement->mBody = parseStatementBlock(pContext);

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
   Expression* expression = parseExpression(pContext, conditionClosureTokenIndex - 1u);
   tokenIndex = conditionClosureTokenIndex + 1u;

   Statement* ifStatement = parseStatement(pContext);
   tokenIndex++;

   if(ifStatement->getType() != StatementType::Block)
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
   CflatInvokeCtor(StatementIf, statement)(expression, ifStatement, elseStatement);

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
      const bool methodCall = tokens[tokenIndex + 1u].mStart[0] == '(';

      if(methodCall)
      {
         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         pSymbols.push_back(pContext.mStringBuffer.c_str());
         tokenIndex++;
         break;
      }

      const bool memberAccess = tokens[tokenIndex + 1u].mStart[0] == '.';
      const bool ptrMemberAccess = !memberAccess && strncmp(tokens[tokenIndex + 1u].mStart, "->", 2u) == 0;

      anyRemainingMemberAccessSymbols = memberAccess || ptrMemberAccess;

      if(anyRemainingMemberAccessSymbols)
      {
         pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
         pSymbols.push_back(pContext.mStringBuffer.c_str());

         if(pSymbols.size() == 1u)
         {
            Instance* instance = retrieveInstance(pContext, pSymbols.back().mName.c_str());
            typeUsage = instance->mValue.mTypeUsage;
         }
         else
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

            CflatAssert(member);

            typeUsage = member->mTypeUsage;
         }

         if(typeUsage.isPointer())
         {
            if(!ptrMemberAccess)
            {
               pContext.mStringBuffer.assign("invalid member access operator ('");
               pContext.mStringBuffer.append(pSymbols.back().mName.c_str());
               pContext.mStringBuffer.append("' is a pointer)");
               throwCompileError(pContext, pContext.mStringBuffer.c_str(), tokens[tokenIndex].mLine);

               return false;
            }
         }
         else
         {
            if(ptrMemberAccess)
            {
               pContext.mStringBuffer.assign("invalid member access operator ('");
               pContext.mStringBuffer.append(pSymbols.back().mName.c_str());
               pContext.mStringBuffer.append("' is not a pointer)");
               throwCompileError(pContext, pContext.mStringBuffer.c_str(), tokens[tokenIndex].mLine);

               return false;
            }
         }

         tokenIndex += 2u;
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

void Environment::throwRuntimeError(ExecutionContext& pContext, const char* pErrorMsg)
{
   pContext.mErrorMessage.assign("Runtime Error: ");
   pContext.mErrorMessage.append(pErrorMsg);
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

         applyBinaryOperator(pContext, &leftValue, &rightValue, expression->mOperator, pOutValue);
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

void Environment::applyBinaryOperator(ExecutionContext& pContext, Value* pLeft, Value* pRight,
   const char* pOperator, Value* pOutValue)
{
   if(pOperator[0] == '<')
   {
      const uint64_t leftValue = getValueAsInteger(pLeft);
      const uint64_t rightValue = getValueAsInteger(pRight);
      const bool result = leftValue < rightValue;

      pOutValue->init(getTypeUsage("bool"));
      pOutValue->set(&result);
   }
}

void Environment::execute(ExecutionContext& pContext, Program& pProgram)
{
   for(size_t i = 0u; i < pProgram.size(); i++)
   {
      execute(pContext, pProgram[i]);

      if(!pContext.mErrorMessage.empty())
         break;
   }
}

uint64_t Environment::getValueAsInteger(Value* pValue)
{
   uint64_t value = 0u;

   if(pValue->mTypeUsage.mType->mSize == 4u)
   {
      value = (uint64_t)CflatRetrieveValue(pValue, uint32_t,,);
   }
   else if(pValue->mTypeUsage.mType->mSize == 8u)
   {
      value = CflatRetrieveValue(pValue, uint64_t,,);
   }
   else if(pValue->mTypeUsage.mType->mSize == 2u)
   {
      value = (uint64_t)CflatRetrieveValue(pValue, uint16_t,,);
   }
   else if(pValue->mTypeUsage.mType->mSize == 1u)
   {
      value = (uint64_t)CflatRetrieveValue(pValue, uint8_t,,);
   }

   return value;
}

bool Environment::integerValueAdd(Context& pContext, Value* pValue, int pQuantity)
{
   const size_t typeSize = pValue->mTypeUsage.mType->mSize;

   if(typeSize == 4u)
   {
      int32_t value = CflatRetrieveValue(pValue, int32_t,,);
      value += (int32_t)pQuantity;
      pValue->set(&value);
   }
   else if(typeSize == 8u)
   {
      int64_t value = CflatRetrieveValue(pValue, int64_t,,);
      value += (int64_t)pQuantity;
      pValue->set(&value);
   }
   else if(typeSize == 2u)
   {
      int16_t value = CflatRetrieveValue(pValue, int16_t,,);
      value += (int16_t)pQuantity;
      pValue->set(&value);
   }
   else if(typeSize == 1u)
   {
      int8_t value = CflatRetrieveValue(pValue, int8_t,,);
      value += (int8_t)pQuantity;
      pValue->set(&value);
   }
   else
   {
      return false;
   }

   return true;
}

void Environment::execute(ExecutionContext& pContext, Statement* pStatement)
{
   switch(pStatement->getType())
   {
   case StatementType::Block:
      {
         incrementScopeLevel(pContext);

         StatementBlock* statement = static_cast<StatementBlock*>(pStatement);

         for(size_t i = 0u; i < statement->mStatements.size(); i++)
         {
            execute(pContext, statement->mStatements[i]);
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
               }

               execute(pContext, statement->mBody);

               if(pOutReturnValue)
               {
                  pOutReturnValue->init(pContext.mReturnValue.mTypeUsage);
                  pOutReturnValue->set(pContext.mReturnValue.mValueBuffer);
               }
            };
         }
      }
      break;
   case StatementType::Assignment:
      {
      }
      break;
   case StatementType::Increment:
      {
         StatementIncrement* statement = static_cast<StatementIncrement*>(pStatement);
         Instance* instance = retrieveInstance(pContext, statement->mVariableName.mName.c_str());
         CflatAssert(instance);
         integerValueAdd(pContext, &instance->mValue, 1);
      }
      break;
   case StatementType::Decrement:
      {
         StatementDecrement* statement = static_cast<StatementDecrement*>(pStatement);
         Instance* instance = retrieveInstance(pContext, statement->mVariableName.mName.c_str());
         CflatAssert(instance);
         integerValueAdd(pContext, &instance->mValue, -1);
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
   case StatementType::For:
      {
      }
      break;
   case StatementType::While:
      {
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
         Instance* instance = retrieveInstance(pContext, statement->mMemberAccess->mSymbols[0].mName.c_str());
         instanceDataValue.mTypeUsage = instance->mValue.mTypeUsage;
         instanceDataValue.mValueBuffer = instance->mValue.mValueBuffer;

         for(size_t i = 1u; i < statement->mMemberAccess->mSymbols.size() - 1u; i++)
         {
            const char* memberName = statement->mMemberAccess->mSymbols[i].mName.c_str();
            Struct* type = static_cast<Struct*>(instanceDataValue.mTypeUsage.mType);
            Member* member = nullptr;

            for(size_t j = 0u; j < type->mMembers.size(); j++)
            {
               if(strcmp(type->mMembers[j].mName.c_str(), memberName) == 0)
               {
                  member = &type->mMembers[j];
                  break;
               }
            }

            CflatAssert(member);

            char* instanceDataPtr = instanceDataValue.mTypeUsage.isPointer()
               ? CflatRetrieveValue(&instanceDataValue, char*,,)
               : instanceDataValue.mValueBuffer;

            instanceDataValue.mTypeUsage = member->mTypeUsage;
            instanceDataValue.mValueBuffer = instanceDataPtr + member->mOffset;
         }

         const char* methodName = statement->mMemberAccess->mSymbols.back().mName.c_str();
         Struct* type = static_cast<Struct*>(instanceDataValue.mTypeUsage.mType);
         Method* method = nullptr;
         
         for(size_t i = 0u; i < type->mMethods.size(); i++)
         {
            if(strcmp(type->mMethods[i].mName.c_str(), methodName) == 0)
            {
               method = &type->mMethods[i];
               break;
            }
         }

         CflatAssert(method);

         Value thisPtr;
         getAddressOfValue(pContext, &instanceDataValue, &thisPtr);

         CflatSTLVector<Value> argumentValues;
         getArgumentValues(pContext, statement->mArguments, argumentValues);

         method->execute(thisPtr, argumentValues, &pContext.mReturnValue);
         instanceDataValue.mValueBuffer = nullptr;
      }
      break;
   case StatementType::Break:
      {
      }
      break;
   case StatementType::Return:
      {
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

void Environment::load(const char* pCode)
{
   ParsingContext parsingContext;
   Program program;

   preprocess(parsingContext, pCode);
   tokenize(parsingContext);
   parse(parsingContext, program);

   if(!parsingContext.mErrorMessage.empty())
      return;
   
   // make sure that there is enough space in the array of instances to avoid
   // memory reallocations, which would potentially invalidate cached pointers
   mExecutionContext.mInstances.reserve(parsingContext.mInstances.capacity());

   execute(mExecutionContext, program);
}


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
      UnaryOperation,
      BinaryOperation,
      Conditional,
      ReturnFunctionCall
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

   struct ExpressionReturnFunctionCall : public Expression
   {
      Symbol mFunctionName;
      CflatSTLVector<Expression*> mArguments;

      ExpressionReturnFunctionCall(const Symbol& pFunctionName)
         : mFunctionName(pFunctionName)
      {
         mType = ExpressionType::ReturnFunctionCall;
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
}


//
//  Environment
//
using namespace Cflat;

const char* kCflatPunctuation[] = 
{
   ".", ",", ":", ";", "->", "(", ")", "{", "}", "[", "]", "<", ">", "::"
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

const char* Environment::findClosure(const char* pCode, char pOpeningChar, char pClosureChar)
{
   CflatAssert(pCode);
   CflatAssert(*pCode == pOpeningChar);

   uint32_t scopeLevel = 0u;
   const char* cursor = pCode;

   while(*cursor != '\0')
   {
      if(*cursor == pOpeningChar)
      {
         scopeLevel++;
      }
      else if(*cursor == pClosureChar)
      {
         scopeLevel--;

         if(scopeLevel == 0u)
         {
            return cursor;
         }
      }

      cursor++;
   }

   return nullptr;
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

   char baseTypeName[64];
   strncpy(baseTypeName, tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
   baseTypeName[tokens[tokenIndex].mLength] = '\0';
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
         if(strncmp(token.mStart, kCflatPunctuation[i], 1u) == 0)
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
         if(strncmp(token.mStart, kCflatOperators[i], 1u) == 0)
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
         pProgram.push_back(statement);
      }

      if(!pContext.mErrorMessage.empty())
         break;
   }
}

Expression* Environment::parseExpression(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Expression* expression = nullptr;

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
      pContext.mStringBuffer.assign(token.mStart, token.mLength);

      TypeUsage typeUsage;
      typeUsage.mType = getType("char");
      typeUsage.mArraySize = (uint16_t)(token.mLength - 2u);

      const char* string = pContext.mStringBuffer.c_str() + 1;
      Value value(typeUsage, string);

      expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
      CflatInvokeCtor(ExpressionValue, expression)(value);
   }
   else if(token.mType == TokenType::Identifier)
   {
      pContext.mStringBuffer.assign(token.mStart, token.mLength);
      Symbol identifier(pContext.mStringBuffer.c_str());

      const Token& nextToken = tokens[tokenIndex + 1u];

      // function call
      if(nextToken.mStart[0] == '(')
      {
         ExpressionReturnFunctionCall* castedExpression = 
            (ExpressionReturnFunctionCall*)CflatMalloc(sizeof(ExpressionReturnFunctionCall));
         CflatInvokeCtor(ExpressionReturnFunctionCall, castedExpression)(identifier);
         expression = castedExpression;

         tokenIndex++;

         while(tokens[tokenIndex++].mStart[0] != ')')
         {
            Expression* argument = parseExpression(pContext);

            if(argument)
            {
               castedExpression->mArguments.push_back(argument);
            }
         }
      }
      // variable access
      else
      {
         expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
         CflatInvokeCtor(ExpressionVariableAccess, expression)(identifier);
      }
   }

   return expression;
}

Statement* Environment::parseStatement(ParsingContext& pContext)
{
   CflatSTLVector<Token>& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];
   Statement* statement = nullptr;

   switch(token.mType)
   {
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
            if(strncmp(nextToken.mStart, "=", 1u) == 0 || strncmp(nextToken.mStart, ";", 1u) == 0)
            {
               Instance* existingInstance = retrieveInstance(pContext, identifier.mName.c_str());

               if(!existingInstance)
               {
                  Expression* initialValue = nullptr;

                  if(strncmp(nextToken.mStart, "=", 1u) == 0)
                  {
                     tokenIndex++;
                     initialValue = parseExpression(pContext);
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
            else if(strncmp(nextToken.mStart, "(", 1u) == 0)
            {
               statement = parseStatementFunctionDeclaration(pContext);
            }

            break;
         }
         // variable access/function call
         else
         {
            tokenIndex++;
            const Token& nextToken = tokens[tokenIndex];

            if(nextToken.mType == TokenType::Punctuation)
            {
               // function call
               if(strncmp(nextToken.mStart, "(", 1u) == 0)
               {
                  //TODO
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
                  }
                  // decrement
                  else if(strncmp(nextToken.mStart, "--", 2u) == 0)
                  {
                     statement = (StatementDecrement*)CflatMalloc(sizeof(StatementDecrement));
                     CflatInvokeCtor(StatementDecrement, statement)(variableName);
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

   while(tokens[tokenIndex++].mStart[0] != '}')
   {
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

Environment::Instance* Environment::registerInstance(Context& pContext, const TypeUsage& pTypeUsage, const char* pName)
{
   Instance instance;
   instance.mTypeUsage = pTypeUsage;
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
   default:
      break;
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

bool Environment::integerValueAdd(Context& pContext, Value* pValue, int pQuantity)
{
   const size_t typeSize = pValue->mTypeUsage.mType->getSize();

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

         if(statement->mInitialValue)
         {
            getValue(pContext, statement->mInitialValue, &instance->mValue);
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
   
   execute(mExecutionContext, program);
}

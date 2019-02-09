
////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.10
//  C++ based Scripting Language
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

using namespace Cflat;

//
//  Environment
//
#define CflatRegisterBuiltInType(pType) \
   { \
      BuiltInType* type = new BuiltInType(#pType); \
      type->mSize = sizeof(pType); \
      registerType(type); \
   }

Environment::Environment()
{
   registerBuiltInTypes();
   registerStandardFunctions();
}

Environment::~Environment()
{
   for(size_t i = 0u; i < mRegisteredTypes.size(); i++)
   {
      delete mRegisteredTypes[i];
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
   CflatRegisterBuiltInType(int);
   CflatRegisterBuiltInType(char);
   CflatRegisterBuiltInType(float);
   CflatRegisterBuiltInType(uint32_t);
   CflatRegisterBuiltInType(size_t);
}

void Environment::registerStandardFunctions()
{
   {
      Function* function = new Function("strlen");
      CflatAddParameterConstPtr(function, char);
      CflatDefineExecutionReturnParams1(function, strlen, size_t, const char*);
      registerFunction(function);
   }
}

void Environment::registerType(Type* pType)
{
   CflatAssert(pType);
   const uint32_t nameHash = hash(pType->mName.c_str());
   CflatAssert(mRegisteredTypes.find(nameHash) == mRegisteredTypes.end());
   mRegisteredTypes[nameHash] = pType;
}

Type* Environment::getType(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   TypesRegistry::const_iterator it = mRegisteredTypes.find(nameHash);
   return it != mRegisteredTypes.end() ? it->second : nullptr;
}

void Environment::registerFunction(Function* pFunction)
{
   CflatAssert(pFunction);
   const uint32_t nameHash = hash(pFunction->mName.c_str());
   CflatAssert(mRegisteredFunctions.find(nameHash) == mRegisteredFunctions.end());
   mRegisteredFunctions[nameHash] = pFunction;
}

Function* Environment::getFunction(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   FunctionsRegistry::const_iterator it = mRegisteredFunctions.find(nameHash);
   return it != mRegisteredFunctions.end() ? it->second : nullptr;
}

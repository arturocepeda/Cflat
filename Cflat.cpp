
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
   { CflatRegisterBuiltInType(int, this); }
   { CflatRegisterBuiltInType(uint32_t, this); }
   { CflatRegisterBuiltInType(size_t, this); }
   { CflatRegisterBuiltInType(char, this); }
   { CflatRegisterBuiltInType(bool, this); }
   { CflatRegisterBuiltInType(uint8_t, this); }
   { CflatRegisterBuiltInType(short, this); }
   { CflatRegisterBuiltInType(uint16_t, this); }
   { CflatRegisterBuiltInType(float, this); }
   { CflatRegisterBuiltInType(double, this); }
}

void Environment::registerStandardFunctions()
{
   {
      CflatRegisterFunction(strlen, this);
      CflatFunctionDefineReturnType(strlen, size_t);
      CflatFunctionAddParameterConstPtr(strlen, char);
      CflatFunctionDefineExecutionReturnParams1(strlen, size_t, const char*);
   }
}

Type* Environment::getType(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   TypesRegistry::const_iterator it = mRegisteredTypes.find(nameHash);
   return it != mRegisteredTypes.end() ? it->second : nullptr;
}

Function* Environment::registerFunction(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   Function* function = new Function(pName);
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
   FunctionsRegistry::iterator it = mRegisteredFunctions.find(nameHash);
   return it != mRegisteredFunctions.end() ? it->second.at(0) : nullptr;
}

CflatSTLVector<Function*>* Environment::getFunctions(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   FunctionsRegistry::iterator it = mRegisteredFunctions.find(nameHash);
   return it != mRegisteredFunctions.end() ? &it->second : nullptr;
}

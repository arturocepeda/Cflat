
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
   { CflatRegisterBuiltInType(this, int); }
   { CflatRegisterBuiltInType(this, uint32_t); }
   { CflatRegisterBuiltInType(this, size_t); }
   { CflatRegisterBuiltInType(this, char); }
   { CflatRegisterBuiltInType(this, bool); }
   { CflatRegisterBuiltInType(this, uint8_t); }
   { CflatRegisterBuiltInType(this, short); }
   { CflatRegisterBuiltInType(this, uint16_t); }
   { CflatRegisterBuiltInType(this, float); }
   { CflatRegisterBuiltInType(this, double); }
}

void Environment::registerStandardFunctions()
{
   {
      CflatRegisterFunction(this, strlen);
      CflatFunctionDefineReturnParams1(this, size_t,,, strlen, const char*,,);
   }
}

Type* Environment::getType(const char* pName)
{
   const uint32_t nameHash = hash(pName);
   TypesRegistry::const_iterator it = mRegisteredTypes.find(nameHash);
   return it != mRegisteredTypes.end() ? it->second : nullptr;
}

TypeRef Environment::getTypeRef(const char* pTypeName)
{
   TypeRef typeRef;

   const size_t typeNameLength = strlen(pTypeName);
   const char* baseTypeNameStart = pTypeName;
   const char* baseTypeNameEnd = pTypeName + typeNameLength - 1u;

   // is it const?
   const char* typeNameConst = strstr(pTypeName, "const");

   if(typeNameConst)
   {
      typeRef.mFlags |= (uint8_t)TypeRefFlags::Const;
      baseTypeNameStart = typeNameConst + 6u;
   }

   // is it a pointer?
   const char* typeNamePtr = strchr(baseTypeNameStart, '*');

   if(typeNamePtr)
   {
      typeRef.mFlags |= (uint8_t)TypeRefFlags::Pointer;
      baseTypeNameEnd = typeNamePtr - 1u;
   }
   else
   {
      // is it a reference?
      const char* typeNameRef = strchr(baseTypeNameStart, '&');

      if(typeNameRef)
      {
         typeRef.mFlags |= (uint8_t)TypeRefFlags::Reference;
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

   typeRef.mType = getType(baseTypeName);
   CflatAssert(typeRef.mType);

   return typeRef;
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

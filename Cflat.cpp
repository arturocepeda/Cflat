
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

#include "Cflat.h"
#include <cmath>

#include "Internal/CflatGlobalFunctions.inl"
#include "Internal/CflatExpressions.inl"
#include "Internal/CflatStatements.inl"
#include "Internal/CflatErrorMessages.inl"


//
//  Memory
//
using namespace Cflat;

Memory::mallocFunction Memory::smmalloc = ::malloc;
Memory::freeFunction Memory::smfree = ::free;

void Memory::setFunctions(Memory::mallocFunction pmalloc, Memory::freeFunction pfree)
{
   smmalloc = pmalloc;
   smfree = pfree;
}

Memory::mallocFunction Memory::malloc()
{
   return smmalloc;
}
Memory::freeFunction Memory::free()
{
   return smfree;
}


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

bool Type::isVoid() const
{
   static const Identifier kvoid("void");

   return mIdentifier == kvoid;
}

bool Type::isDecimal() const
{
   static const Identifier kfloat("float");
   static const Identifier kfloat32_t("float32_t");
   static const Identifier kdouble("double");

   return mCategory == TypeCategory::BuiltIn &&
      (mIdentifier == kfloat || mIdentifier == kfloat32_t || mIdentifier == kdouble);
}

bool Type::isInteger() const
{
   return (mCategory == TypeCategory::BuiltIn && !isDecimal()) ||
      mCategory == TypeCategory::Enum ||
      mCategory == TypeCategory::EnumClass;
}

bool Type::compatibleWith(const Type& pOther) const
{
   return this == &pOther || (isInteger() && pOther.isInteger());
}


//
//  TypeUsage
//
const CflatArgsVector(TypeUsage)& TypeUsage::kEmptyList()
{
   static CflatArgsVector(TypeUsage) emptyList;
   return emptyList;
}

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
      return sizeof(void*) * mArraySize;
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

bool TypeUsage::isConstPointer() const
{
   return CflatHasFlag(mFlags, TypeUsageFlags::ConstPointer);
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
const CflatArgsVector(Value)& Value::kEmptyList()
{
   static CflatArgsVector(Value) emptyList;
   return emptyList;
}

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
   , mFlags(0u)
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
   , mFlags(0u)
   , execute(nullptr)
{
}

Method::~Method()
{
   execute = nullptr;
}


//
//  MethodUsage
//
MethodUsage::MethodUsage()
   : mMethod(nullptr)
   , mOffset(0u)
{
}


//
//  Instance
//
Instance::Instance()
   : mScopeLevel(0u)
   , mFlags(0u)
{
}

Instance::Instance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
   : mTypeUsage(pTypeUsage)
   , mIdentifier(pIdentifier)
   , mScopeLevel(0u)
   , mFlags(0u)
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

Type* TypesHolder::getType(const Identifier& pIdentifier) const
{
   TypesRegistry::const_iterator it = mTypes.find(pIdentifier.mHash);
   return it != mTypes.end() ? it->second : nullptr;
}

Type* TypesHolder::getType(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
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

const TypeAlias* TypesHolder::getTypeAlias(const Identifier& pIdentifier) const
{
   TypeAliasesRegistry::const_iterator it = mTypeAliases.find(pIdentifier.mHash);
   return it != mTypeAliases.end() ? &it->second : nullptr;
}

bool TypesHolder::deregisterType(Type* pType)
{
   const Hash hash = pType->getHash();
   TypesRegistry::const_iterator it = mTypes.find(hash);

   if(it != mTypes.end())
   {
      CflatInvokeDtor(Type, it->second);
      CflatFree(it->second);

      mTypes.erase(it);

      return true;
   }

   return false;
}

void TypesHolder::getAllTypes(CflatSTLVector(Type*)* pOutTypes) const
{
   pOutTypes->reserve(pOutTypes->size() + mTypes.size());

   for(TypesRegistry::const_iterator it = mTypes.begin(); it != mTypes.end(); it++)
   {
      pOutTypes->push_back(it->second);
   }
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

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier) const
{
   FunctionsRegistry::const_iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? it->second.at(0) : nullptr;
}

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return getFunction(pIdentifier, pParameterTypes, pTemplateTypes, false);
}

Function* FunctionsHolder::getFunctionPerfectMatch(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return getFunction(pIdentifier, pParameterTypes, pTemplateTypes, true);
}

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   CflatArgsVector(TypeUsage) typeUsages;

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return getFunction(pIdentifier, typeUsages, pTemplateTypes, false);
}

CflatSTLVector(Function*)* FunctionsHolder::getFunctions(const Identifier& pIdentifier) const
{
   FunctionsRegistry::const_iterator it = mFunctions.find(pIdentifier.mHash);
   return it != mFunctions.end() ? const_cast<CflatSTLVector(Function*)*>(&it->second) : nullptr;
}

void FunctionsHolder::getAllFunctions(CflatSTLVector(Function*)* pOutFunctions) const
{
   size_t functionsCount = 0u;

   for(FunctionsRegistry::const_iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
   {
      const CflatSTLVector(Function*)& functions = it->second;
      functionsCount += functions.size();
   }

   if(functionsCount > 0u)
   {
      pOutFunctions->reserve(pOutFunctions->size() + functionsCount);

      for(FunctionsRegistry::const_iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
      {
         const CflatSTLVector(Function*)& functions = it->second;

         for(size_t i = 0u; i < functions.size(); i++)
         {
            pOutFunctions->push_back(functions[i]);
         }
      }
   }
}

size_t FunctionsHolder::getFunctionsCount() const
{
   size_t functionsCount = 0u;

   for(FunctionsRegistry::const_iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
   {
      const CflatSTLVector(Function*)& functions = it->second;
      functionsCount += functions.size();
   }

   return functionsCount;
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

Function* FunctionsHolder::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes,
   bool pRequirePerfectMatch) const
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

     if(!pRequirePerfectMatch)
      {
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

         // third pass: look for a variadic function
         if(!function)
         {
            for(size_t i = 0u; i < functions->size(); i++)
            {
               Function* functionOverload = functions->at(i);

               if(CflatHasFlag(functionOverload->mFlags, FunctionFlags::Variadic) &&
                  functionOverload->mParameters.size() <= pParameterTypes.size() &&
                  functionOverload->mTemplateTypes == pTemplateTypes)
               {
                  bool parametersMatch = true;

                  for(size_t j = 0u; j < functionOverload->mParameters.size(); j++)
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
   }

   return function;
}

bool FunctionsHolder::deregisterFunctions(const Identifier& pIdentifier)
{
   FunctionsRegistry::iterator it = mFunctions.find(pIdentifier.mHash);

   if(it != mFunctions.end())
   {
      CflatSTLVector(Function*)& functions = it->second;

      for(size_t i = 0u; i < functions.size(); i++)
      {
         CflatInvokeDtor(Function, functions[i]);
         CflatFree(functions[i]);
      }

      functions.clear();
      mFunctions.erase(it);

      return true;
   }

   return false;
}


//
//  InstancesHolder
//
InstancesHolder::InstancesHolder()
{
}

InstancesHolder::~InstancesHolder()
{
   releaseInstances(0u, true);
}

Instance* InstancesHolder::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   Instance* instance = retrieveInstance(pIdentifier);

   if(!instance)
   {
      instance = registerInstance(pTypeUsage, pIdentifier);
   }

   instance->mValue = pValue;

   return instance;
}

Value* InstancesHolder::getVariable(const Identifier& pIdentifier) const
{
   Instance* instance = retrieveInstance(pIdentifier);
   return instance ? &instance->mValue : nullptr;
}

Instance* InstancesHolder::registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   mInstances.emplace_back(pTypeUsage, pIdentifier);
   return &mInstances.back();
}

Instance* InstancesHolder::retrieveInstance(const Identifier& pIdentifier) const
{
   Instance* instance = nullptr;

   for(int i = (int)mInstances.size() - 1; i >= 0; i--)
   {
      if(mInstances[i].mIdentifier == pIdentifier)
      {
         instance = const_cast<Instance*>(&mInstances[i]);
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
            Struct* structOrClassType = static_cast<Struct*>(instanceType);
            Method* dtor = structOrClassType->getDestructor();

            if(dtor)
            {
               TypeUsage thisPtrTypeUsage;
               thisPtrTypeUsage.mType = instanceType;
               thisPtrTypeUsage.mPointerLevel = 1u;

               Value thisPtrValue;
               thisPtrValue.initExternal(thisPtrTypeUsage);
               thisPtrValue.set(&instance.mValue.mValueBuffer);

               CflatArgsVector(Value) args;
               dtor->execute(thisPtrValue, args, nullptr);
            }
         }
      }

      mInstances.pop_back();
   }
}

void InstancesHolder::getAllInstances(CflatSTLVector(Instance*)* pOutInstances) const
{
   pOutInstances->reserve(pOutInstances->size() + mInstances.size());

   for(size_t i = 0u; i < mInstances.size(); i++)
   {
      pOutInstances->push_back(const_cast<Instance*>(&mInstances[i]));
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
   , mAlignment(0u)
   , mCachedMethodIndexDefaultConstructor(kInvalidCachedMethodIndex)
   , mCachedMethodIndexCopyConstructor(kInvalidCachedMethodIndex)
   , mCachedMethodIndexDestructor(kInvalidCachedMethodIndex)
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
      if(mBaseTypes[i].mType == pBaseType ||
         static_cast<Struct*>(mBaseTypes[i].mType)->derivedFrom(pBaseType))
      {
         return true;
      }
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

Type* Struct::getType(const Identifier& pIdentifier) const
{
   return mTypesHolder.getType(pIdentifier);
}

Type* Struct::getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return mTypesHolder.getType(pIdentifier, pTemplateTypes);
}

void Struct::registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   mTypesHolder.registerTypeAlias(pIdentifier, pTypeUsage);
}

const TypeAlias* Struct::getTypeAlias(const Identifier& pIdentifier) const
{
   return mTypesHolder.getTypeAlias(pIdentifier);
}

Function* Struct::registerStaticMethod(const Identifier& pIdentifier)
{
   return mFunctionsHolder.registerFunction(pIdentifier);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier) const
{
   return mFunctionsHolder.getFunction(pIdentifier);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes,
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return mFunctionsHolder.getFunction(pIdentifier, pParameterTypes, pTemplateTypes);
}

Function* Struct::getStaticMethod(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments,
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return mFunctionsHolder.getFunction(pIdentifier, pArguments, pTemplateTypes);
}

CflatSTLVector(Function*)* Struct::getStaticMethods(const Identifier& pIdentifier) const
{
   return mFunctionsHolder.getFunctions(pIdentifier);
}

void Struct::setStaticMember(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   mInstancesHolder.setVariable(pTypeUsage, pIdentifier, pValue);
}

Value* Struct::getStaticMember(const Identifier& pIdentifier) const
{
   return mInstancesHolder.getVariable(pIdentifier);
}

Instance* Struct::getStaticMemberInstance(const Identifier& pIdentifier) const
{
   return mInstancesHolder.retrieveInstance(pIdentifier);
}

Member* Struct::findMember(const Identifier& pIdentifier) const
{
   for(size_t i = 0u; i < mMembers.size(); i++)
   {
      if(mMembers[i].mIdentifier == pIdentifier)
      {
         return const_cast<Member*>(&mMembers[i]);
      }
   }

   Member* member = nullptr;

   for(size_t i = 0u; i < mBaseTypes.size(); ++i)
   {
      CflatAssert(mBaseTypes[i].mType->mCategory == TypeCategory::StructOrClass);
      Struct* baseType = static_cast<Struct*>(mBaseTypes[i].mType);
      member = baseType->findMember(pIdentifier);

      if(member)
      {
         break;
      }
   }

   return member;
}

Method* Struct::getDefaultConstructor() const
{
   if(mCachedMethodIndexDefaultConstructor != kInvalidCachedMethodIndex)
   {
      CflatAssert((size_t)mCachedMethodIndexDefaultConstructor < mMethods.size());
      return const_cast<Method*>(&mMethods[mCachedMethodIndexDefaultConstructor]);
   }

   return nullptr;
}

Method* Struct::getCopyConstructor() const
{
   if(mCachedMethodIndexCopyConstructor != kInvalidCachedMethodIndex)
   {
      CflatAssert((size_t)mCachedMethodIndexCopyConstructor < mMethods.size());
      return const_cast<Method*>(&mMethods[mCachedMethodIndexCopyConstructor]);
   }

   return nullptr;
}

Method* Struct::getDestructor() const
{
   if(mCachedMethodIndexDestructor != kInvalidCachedMethodIndex)
   {
      CflatAssert((size_t)mCachedMethodIndexDestructor < mMethods.size());
      return const_cast<Method*>(&mMethods[mCachedMethodIndexDestructor]);
   }

   return nullptr;
}

Method* Struct::findConstructor(const CflatArgsVector(TypeUsage)& pParameterTypes) const
{
   const Identifier emptyId;
   return findMethod(emptyId, pParameterTypes);
}

Method* Struct::findConstructor(const CflatArgsVector(Value)& pArguments) const
{
   const Identifier emptyId;
   return findMethod(emptyId, pArguments);
}

Method* Struct::findMethod(const Identifier& pIdentifier) const
{
   for(size_t i = 0u; i < mMethods.size(); i++)
   {
      if(mMethods[i].mIdentifier == pIdentifier)
      {
         return const_cast<Method*>(&mMethods[i]);
      }
   }

   Method* method = nullptr;

   for(size_t i = 0u; i < mBaseTypes.size(); ++i)
   {
      CflatAssert(mBaseTypes[i].mType->mCategory == TypeCategory::StructOrClass);
      Struct* baseType = static_cast<Struct*>(mBaseTypes[i].mType);
      method = baseType->findMethod(pIdentifier);

      if(method)
      {
         break;
      }
   }

   return method;
}

Method* Struct::findMethod(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   const MethodUsage methodUsage = findMethodUsage(pIdentifier, 0u, pParameterTypes, pTemplateTypes);
   return methodUsage.mMethod;
}

Method* Struct::findMethod(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   CflatArgsVector(TypeUsage) typeUsages;

   for(size_t i = 0u; i < pArguments.size(); i++)
   {
      typeUsages.push_back(pArguments[i].mTypeUsage);
   }

   return findMethod(pIdentifier, typeUsages);
}

Function* Struct::findStaticMethod(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   Function* staticMethod = getStaticMethod(pIdentifier, pParameterTypes, pTemplateTypes);

   if(!staticMethod)
   {
      for(size_t i = 0u; i < mBaseTypes.size(); i++)
      {
         CflatAssert(mBaseTypes[i].mType->mCategory == TypeCategory::StructOrClass);
         Struct* baseType = static_cast<Struct*>(mBaseTypes[i].mType);
         staticMethod = baseType->findStaticMethod(pIdentifier, pParameterTypes, pTemplateTypes);

         if(staticMethod)
         {
            break;
         }
      }
   }

   return staticMethod;
}

MethodUsage Struct::findMethodUsage(const Identifier& pIdentifier, size_t pOffset,
   const CflatArgsVector(TypeUsage)& pParameterTypes, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   MethodUsage methodUsage;

   // first pass: look for a perfect argument match
   for(size_t i = 0u; i < mMethods.size(); i++)
   {
      if(mMethods[i].mIdentifier == pIdentifier &&         
         mMethods[i].mParameters.size() == pParameterTypes.size() &&
         mMethods[i].mTemplateTypes == pTemplateTypes)
      {
         bool parametersMatch = true;

         for(size_t j = 0u; j < pParameterTypes.size(); j++)
         {
            const TypeHelper::Compatibility compatibility =
               TypeHelper::getCompatibility(mMethods[i].mParameters[j], pParameterTypes[j]);

            if(compatibility != TypeHelper::Compatibility::PerfectMatch)
            {
               parametersMatch = false;
               break;
            }
         }

         if(parametersMatch)
         {
            methodUsage.mMethod = const_cast<Method*>(&mMethods[i]);
            break;
         }
      }
   }

   // second pass: look for a compatible argument match
   if(!methodUsage.mMethod)
   {
      for(size_t i = 0u; i < mMethods.size(); i++)
      {
         if(mMethods[i].mIdentifier == pIdentifier &&
            mMethods[i].mParameters.size() == pParameterTypes.size() &&
            mMethods[i].mTemplateTypes == pTemplateTypes)
         {
            bool parametersMatch = true;

            for(size_t j = 0u; j < pParameterTypes.size(); j++)
            {
               const TypeHelper::Compatibility compatibility =
                  TypeHelper::getCompatibility(mMethods[i].mParameters[j], pParameterTypes[j]);

               if(compatibility == TypeHelper::Compatibility::Incompatible)
               {
                  parametersMatch = false;
                  break;
               }
            }

            if(parametersMatch)
            {
               methodUsage.mMethod = const_cast<Method*>(&mMethods[i]);
               break;
            }
         }
      }
   }

   if(!methodUsage.mMethod)
   {
      for(size_t i = 0u; i < mBaseTypes.size(); ++i)
      {
         CflatAssert(mBaseTypes[i].mType->mCategory == TypeCategory::StructOrClass);
         Struct* baseType = static_cast<Struct*>(mBaseTypes[i].mType);

         const size_t totalOffset = pOffset + (size_t)mBaseTypes[i].mOffset;
         methodUsage =
            baseType->findMethodUsage(pIdentifier, totalOffset, pParameterTypes, pTemplateTypes);

         if(methodUsage.mMethod)
         {
            methodUsage.mOffset = totalOffset;
            break;
         }
      }
   }

   return methodUsage;
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
TypeHelper::CustomPerfectMatchesRegistry TypeHelper::smCustomPerfectMatchesRegistry;

void TypeHelper::registerCustomPerfectMatch(Type* pTypeA, Type* pTypeB)
{
   CflatAssert(pTypeA && pTypeB);

   CustomPerfectMatchesRegistry::iterator it =
      smCustomPerfectMatchesRegistry.find(pTypeA->mIdentifier.mHash);

   if(it == smCustomPerfectMatchesRegistry.end())
   {
      CflatSTLSet(Hash) typeSet;
      typeSet.insert(pTypeB->mIdentifier.mHash);
      smCustomPerfectMatchesRegistry.emplace(pTypeA->mIdentifier.mHash, typeSet);
   }
   else
   {
      it->second.insert(pTypeB->mIdentifier.mHash);
   }

   it = smCustomPerfectMatchesRegistry.find(pTypeB->mIdentifier.mHash);

   if(it == smCustomPerfectMatchesRegistry.end())
   {
      CflatSTLSet(Hash) typeSet;
      typeSet.insert(pTypeA->mIdentifier.mHash);
      smCustomPerfectMatchesRegistry.emplace(pTypeB->mIdentifier.mHash, typeSet);
   }
   else
   {
      it->second.insert(pTypeA->mIdentifier.mHash);
   }
}

void TypeHelper::releaseCustomPerfectMatchesRegistry()
{
   smCustomPerfectMatchesRegistry.clear();
}

TypeHelper::Compatibility TypeHelper::getCompatibility(
   const TypeUsage& pParameter, const TypeUsage& pArgument, uint32_t pRecursionDepth)
{
   if(pParameter == pArgument)
   {
      return Compatibility::PerfectMatch;
   }

   if(pParameter.isReference() && !pParameter.isConst() && pArgument.isConst())
   {
      return Compatibility::Incompatible;
   }

   if(pParameter.mType == pArgument.mType ||
      isCustomPerfectMatch(pParameter.mType, pArgument.mType))
   {
      if(pParameter.mPointerLevel == pArgument.mPointerLevel &&
         pParameter.getSize() == pArgument.getSize())
      {
         return Compatibility::PerfectMatch;
      }

      if(pParameter.mPointerLevel == (pArgument.mPointerLevel + 1u) &&
         !pParameter.isArray() &&
         pArgument.isArray())
      {
         return Compatibility::PerfectMatch;
      }
   }

   if(pArgument.compatibleWith(pParameter))
   {
      return Compatibility::ImplicitCastableInteger;
   }

   if(pArgument.mType->mCategory == TypeCategory::BuiltIn &&
      !pArgument.isPointer() &&
      pParameter.mType->mCategory == TypeCategory::BuiltIn &&
      !pParameter.isPointer())
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

   if(pParameter.mType->isVoid() && pParameter.isPointer() && pArgument.isPointer())
   {
      return Compatibility::ImplicitCastableInteger;
   }

   if(pParameter.isPointer() && pArgument.mType->isVoid() && pArgument.isPointer())
   {
      return Compatibility::ImplicitCastableInteger;
   }

   if(pParameter.mType->mCategory == TypeCategory::StructOrClass && !pParameter.isPointer())
   {
      static const Hash kInitializerListIdentifierHash = hash("initializer_list");

      Struct* parameterType = static_cast<Struct*>(pParameter.mType);

      if(pParameter.mType->mIdentifier.mHash == kInitializerListIdentifierHash &&
         pArgument.isArray())
      {
         CflatAssert(!parameterType->mTemplateTypes.empty());

         if(parameterType->mTemplateTypes[0].mType == pArgument.mType)
         {
            return Compatibility::ImplicitConstructable;
         }
      }

      if(pRecursionDepth == 0u)
      {
         for(size_t i = 0u; i < parameterType->mMethods.size(); i++)
         {
            const Method& method = parameterType->mMethods[i];

            if(method.mIdentifier.mNameLength == 0u && !method.mParameters.empty())
            {
               const Compatibility ctorCompatibility =
                  getCompatibility(method.mParameters[0], pArgument, pRecursionDepth + 1u);

               if(ctorCompatibility != Compatibility::Incompatible)
               {
                  return Compatibility::ImplicitConstructable;
               }
            }
         }
      }
   }

   return Compatibility::Incompatible;
}

size_t TypeHelper::calculateAlignment(const TypeUsage& pTypeUsage)
{
   if(pTypeUsage.isPointer())
   {
      return alignof(void*);
   }

   size_t alignment = pTypeUsage.mType->mSize;

   if(pTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
   {
      const Struct* structOrClassType = static_cast<const Struct*>(pTypeUsage.mType);

      if(structOrClassType->mAlignment != 0u)
      {
         alignment = (size_t)structOrClassType->mAlignment;
      }
      else
      {
         alignment = 1u;

         for(size_t i = 0u; i < structOrClassType->mMembers.size(); i++)
         {
            const size_t memberTypeAlignment =
               calculateAlignment(structOrClassType->mMembers[i].mTypeUsage);

            if(memberTypeAlignment > alignment)
            {
               alignment = memberTypeAlignment;
            }
         }
      }
   }

   return alignment;
}

bool TypeHelper::isCustomPerfectMatch(Type* pTypeA, Type* pTypeB)
{
   CflatAssert(pTypeA && pTypeB);

   CustomPerfectMatchesRegistry::const_iterator it =
      smCustomPerfectMatchesRegistry.find(pTypeA->mIdentifier.mHash);
   return it != smCustomPerfectMatchesRegistry.end() &&
      it->second.find(pTypeB->mIdentifier.mHash) != it->second.end();
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
      if(*cursor == '"' || (*cursor == 'L' && *(cursor + 1) == '"'))
      {
         const bool wide = *cursor == 'L';

         if(wide)
         {
            cursor++;
         }

         do
         {
            cursor++;

            if(*cursor == '\n')
            {
               break;
            }
         }
         while(!(*cursor == '"' && *(cursor - 1) != '\\'));

         cursor++;
         token.mLength = cursor - token.mStart;
         token.mType = wide ? TokenType::WideString : TokenType::String;
         pTokens.push_back(token);
         continue;
      }

      // character
      if(*cursor == '\'' || (*cursor == 'L' && *(cursor + 1) == '\''))
      {
         const bool wide = *cursor == 'L';

         if(wide)
         {
            cursor++;
         }

         do
         {
            cursor++;

            if(*cursor == '\n')
            {
               break;
            }
         }
         while(!(*cursor == '\'' && *(cursor - 1) != '\\'));

         cursor++;
         token.mLength = cursor - token.mStart;
         token.mType = wide ? TokenType::WideCharacter : TokenType::Character;
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
            while(isdigit(*cursor) || *cursor == '.' || *cursor == 'f' || *cursor == 'u'
               || *cursor == 'e' || *cursor == '-');
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
            !isValidIdentifierCharacter(token.mStart[keywordLength]))
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
      while(isValidIdentifierCharacter(*cursor));

      token.mLength = cursor - token.mStart;
      token.mType = TokenType::Identifier;
      pTokens.push_back(token);
   }
}

bool Tokenizer::isValidIdentifierCharacter(char pCharacter)
{
   return isalnum(pCharacter) || pCharacter == '_';
}

bool Tokenizer::isValidIdentifierBeginningCharacter(char pCharacter)
{
   return !isdigit(pCharacter) && (isalpha(pCharacter) || pCharacter == '_');
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
{
   if(pParent && pParent->getParent())
   {
      char buffer[kDefaultLocalStringBufferSize];
      snprintf(buffer, sizeof(buffer), "%s::%s", mParent->mFullIdentifier.mName, mIdentifier.mName);
      mFullIdentifier = Identifier(buffer);
   }
}

Namespace::~Namespace()
{
   mInstancesHolder.releaseInstances(0u, true);

   for(NamespacesRegistry::iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      Namespace* ns = it->second;
      CflatInvokeDtor(Namespace, ns);
      CflatFree(ns);
   }
}

const Identifier& Namespace::getIdentifier() const
{
   return mIdentifier;
}

const Identifier& Namespace::getFullIdentifier() const
{
   return mFullIdentifier;
}

Namespace* Namespace::getParent() const
{
   return mParent;
}

Namespace* Namespace::getChild(Hash pNameHash) const
{
   NamespacesRegistry::const_iterator it = mNamespaces.find(pNameHash);
   return it != mNamespaces.end() ? it->second : nullptr;
}

Namespace* Namespace::getNamespace(const Identifier& pName) const
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

Type* Namespace::getType(const Identifier& pIdentifier, bool pExtendSearchToParent) const
{
   return getType(pIdentifier, TypeUsage::kEmptyList(), pExtendSearchToParent);
}

Type* Namespace::getType(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pTemplateTypes, bool pExtendSearchToParent) const
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

const TypeAlias* Namespace::getTypeAlias(const Identifier& pIdentifier) const
{
   return mTypesHolder.getTypeAlias(pIdentifier);
}

bool Namespace::deregisterType(Type* pType)
{
   return mTypesHolder.deregisterType(pType);
}

TypeUsage Namespace::getTypeUsage(const char* pTypeName) const
{
   return mEnvironment->getTypeUsage(pTypeName, const_cast<Namespace*>(this));
}

Function* Namespace::getFunction(const Identifier& pIdentifier, bool pExtendSearchToParent) const
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
   bool pExtendSearchToParent) const
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

Function* Namespace::getFunctionPerfectMatch(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes,
   const CflatArgsVector(TypeUsage)& pTemplateTypes,
   bool pExtendSearchToParent) const
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
         return ns->getFunctionPerfectMatch(functionIdentifier, pParameterTypes, pTemplateTypes);
      }

      Function* function = nullptr;

      if(pExtendSearchToParent && mParent)
      {
         function = 
            mParent->getFunctionPerfectMatch(pIdentifier, pParameterTypes, pTemplateTypes, true);
      }

      return function;
   }

   Function* function =
      mFunctionsHolder.getFunctionPerfectMatch(pIdentifier, pParameterTypes, pTemplateTypes);

   if(!function && pExtendSearchToParent && mParent)
   {
      function =
         mParent->getFunctionPerfectMatch(pIdentifier, pParameterTypes, pTemplateTypes, true);
   }

   return function;
}

Function* Namespace::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments,
   const CflatArgsVector(TypeUsage)& pTemplateTypes,
   bool pExtendSearchToParent) const
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
   bool pExtendSearchToParent) const
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

bool Namespace::deregisterFunctions(const Identifier& pIdentifier)
{
   return mFunctionsHolder.deregisterFunctions(pIdentifier);
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

Instance* Namespace::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
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

   return instance;
}

Value* Namespace::getVariable(const Identifier& pIdentifier, bool pExtendSearchToParent) const
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

Instance* Namespace::retrieveInstance(const Identifier& pIdentifier, bool pExtendSearchToParent) const
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

void Namespace::getAllNamespaces(CflatSTLVector(Namespace*)* pOutNamespaces, bool pRecursively) const
{
   pOutNamespaces->reserve(pOutNamespaces->size() + mNamespaces.size());

   for(NamespacesRegistry::const_iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
   {
      pOutNamespaces->push_back(it->second);

      if(pRecursively)
      {
         it->second->getAllNamespaces(pOutNamespaces, true);
      }
   }
}

void Namespace::getAllTypes(CflatSTLVector(Type*)* pOutTypes, bool pRecursively) const
{
   mTypesHolder.getAllTypes(pOutTypes);

   if(pRecursively)
   {
      for(NamespacesRegistry::const_iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
      {
         it->second->getAllTypes(pOutTypes, true);
      }
   }
}

void Namespace::getAllInstances(CflatSTLVector(Instance*)* pOutInstances, bool pRecursively) const
{
   mInstancesHolder.getAllInstances(pOutInstances);

   if(pRecursively)
   {
      for(NamespacesRegistry::const_iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
      {
         it->second->getAllInstances(pOutInstances, true);
      }
   }
}

void Namespace::getAllFunctions(CflatSTLVector(Function*)* pOutFunctions, bool pRecursively) const
{
   mFunctionsHolder.getAllFunctions(pOutFunctions);

   if(pRecursively)
   {
      for(NamespacesRegistry::const_iterator it = mNamespaces.begin(); it != mNamespaces.end(); it++)
      {
         it->second->getAllFunctions(pOutFunctions, true);
      }
   }
}


//
//  Context
//
Context::Context(ContextType pType, Namespace* pGlobalNamespace)
   : mType(pType)
   , mProgram(nullptr)
   , mBlockLevel(0u)
   , mScopeLevel(0u)
{
   mNamespaceStack.push_back(pGlobalNamespace);
}


//
//  ParsingContext
//
ParsingContext::ParsingContext(Namespace* pGlobalNamespace)
   : Context(ContextType::Parsing, pGlobalNamespace)
   , mTokenIndex(0u)
   , mCurrentFunction(nullptr)
   , mLocalNamespaceGlobalIndex(0u)
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
   : mSettings(0u)
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

   mTypeUsageVoid = getTypeUsage("void");
   mTypeUsageSizeT = getTypeUsage("size_t");
   mTypeUsageBool = getTypeUsage("bool");
   mTypeUsageCString = getTypeUsage("const char*");
   mTypeUsageWideString = getTypeUsage("const wchar_t*");
   mTypeUsageCharacter = getTypeUsage("const char");
   mTypeUsageWideCharacter = getTypeUsage("const wchar_t");
   mTypeUsageVoidPtr = getTypeUsage("void*");
}

Environment::~Environment()
{
   mGlobalNamespace.releaseInstances(0, true);

   for(ProgramsRegistry::iterator it = mPrograms.begin(); it != mPrograms.end(); it++)
   {
      CflatInvokeDtor(Program, it->second);
      CflatFree(it->second);
   }
}

void Environment::addSetting(Settings pSetting)
{
   CflatSetFlag(mSettings, pSetting);
}

void Environment::removeSetting(Settings pSetting)
{
   CflatResetFlag(mSettings, pSetting);
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

      if(currentChar != ' ')
      {
         if(currentParameterIndex < 0)
         {
            macro.mName.push_back(currentChar);
         }
         else
         {
            parameters[currentParameterIndex].push_back(currentChar);
         }
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
   CflatRegisterBuiltInType(this, uint32_t);
   CflatRegisterBuiltInType(this, size_t);
   CflatRegisterBuiltInType(this, char);
   CflatRegisterBuiltInType(this, bool);
   CflatRegisterBuiltInType(this, uint8_t);
   CflatRegisterBuiltInType(this, short);
   CflatRegisterBuiltInType(this, wchar_t);
   CflatRegisterBuiltInType(this, uint16_t);
   CflatRegisterBuiltInType(this, int64_t);
   CflatRegisterBuiltInType(this, uint64_t);
   CflatRegisterBuiltInType(this, float);
   CflatRegisterBuiltInType(this, double);

   CflatRegisterBuiltInTypedef(this, int32_t, int);
   CflatRegisterBuiltInTypedef(this, int8_t, char);
   CflatRegisterBuiltInTypedef(this, int16_t, short);
}

TypeUsage Environment::parseTypeUsage(ParsingContext& pContext, size_t pTokenLastIndex) const
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
   bool anyInvalidTemplateType = false;

   if(tokenIndex < (tokens.size() - 1u) && tokens[tokenIndex + 1u].mStart[0] == '<')
   {
      tokenIndex += 2u;
      const size_t closureTokenIndex = findClosureTokenIndex(pContext, '<', '>');

      if((pTokenLastIndex == 0u || closureTokenIndex <= pTokenLastIndex) &&
         isTemplate(pContext, tokenIndex - 1u, closureTokenIndex))
      {
         while(tokenIndex < closureTokenIndex)
         {
            const TypeUsage templateTypeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

            if(!templateTypeUsage.mType)
            {
               anyInvalidTemplateType = true;
               break;
            }

            templateTypes.push_back(templateTypeUsage);
            tokenIndex++;
         }

         tokenIndex = closureTokenIndex;
      }
   }

   if(!anyInvalidTemplateType)
   {   
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
         for(int i = (int)pContext.mLocalNamespaceStack.size() - 1; i >= 0; i--)
         {
            Namespace* ns = pContext.mLocalNamespaceStack[i].mNamespace;
            typeUsage.mType = ns->getType(baseTypeIdentifier, templateTypes);

            if(typeUsage.mType)
            {
               break;
            }
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
   }

   if(typeUsage.mType)
   {
      while(tokenIndex < (tokens.size() - 1u) && *tokens[tokenIndex + 1u].mStart == '*')
      {
         typeUsage.mPointerLevel++;
         tokenIndex++;
      }

      if(typeUsage.mPointerLevel > 0u && typeUsage.isConst())
      {
         CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Const);
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::ConstPointer);
      }

      if(tokenIndex < (tokens.size() - 1u) && *tokens[tokenIndex + 1u].mStart == '&')
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);
         tokenIndex++;
      }

      tokenIndex++;

      if(tokenIndex < tokens.size() &&
         typeUsage.mPointerLevel > 0u &&
         tokens[tokenIndex].mType == TokenType::Keyword &&
         strncmp(tokens[tokenIndex].mStart, "const", 5u) == 0)
      {
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Const);
         tokenIndex++;
      }
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
   snprintf(errorMsg, sizeof(errorMsg), kPreprocessorErrorStrings[(int)pError], pArg);

   const char* code = pContext.mProgram->mCode.c_str();
   uint16_t line = 1u;

   for(size_t i = 0u; i < pCursor; i++)
   {
      if(code[i] == '\n')
      {
         line++;
      }
   }

   char lineAsString[kSmallLocalStringBufferSize];
   snprintf(lineAsString, sizeof(lineAsString), "%d", line);

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
   snprintf(errorMsg, sizeof(errorMsg), kCompileErrorStrings[(int)pError], pArg1, pArg2);

   char lineAsString[kSmallLocalStringBufferSize];
   snprintf(lineAsString, sizeof(lineAsString), "%d", token.mLine);

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
      // string literal
      if(pCode[cursor] == '"')
      {
         do
         {
            preprocessedCode.push_back(pCode[cursor]);
            cursor++;
         }
         while(pCode[cursor] != '"' && pCode[cursor] != '\0');
      }
      // line comment
      else if(strncmp(pCode + cursor, "//", 2u) == 0)
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
               if(pCode[cursor] == '(')
               {
                  while(pCode[cursor] != ')')
                  {
                     macroDefinition[macroCursor++] = pCode[cursor++];
                  }
               }
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

      // perform macro replacement
      if(Tokenizer::isValidIdentifierBeginningCharacter(pCode[cursor]))
      {
         for(size_t i = 0u; i < mMacros.size(); i++)
         {
            Macro& macro = mMacros[i];

            if(strncmp(pCode + cursor, macro.mName.c_str(), macro.mName.length()) == 0 &&
               !Tokenizer::isValidIdentifierCharacter(pCode[cursor + macro.mName.length()]))
            {
               cursor += macro.mName.length();

               if(macro.mParametersCount > 0u)
               {
                  while(pCode[cursor] == ' ' || pCode[cursor] == '\n')
                  {
                     cursor++;
                  }
               }

               // parse arguments
               CflatSTLVector(CflatSTLString) arguments;

               if(pCode[cursor] == '(')
               {
                  int parenthesisLevel = 1;

                  arguments.emplace_back();
                  cursor++;

                  while(parenthesisLevel > 0 && pCode[cursor] != '\0')
                  {
                     if (pCode[cursor] == '(')
                     {
                        parenthesisLevel++;
                     }
                     else if (pCode[cursor] == ')')
                     {
                        parenthesisLevel--;
                        if (parenthesisLevel == 0)
                        {
                           break;
                        }
                     }

                     if (pCode[cursor] == '"')
                     {
                        do
                        {
                           arguments.back().push_back(pCode[cursor]);
                           cursor++;
                        }
                        while(!(pCode[cursor] == '"' && pCode[cursor + 1] != '\\'));

                        arguments.back().push_back(pCode[cursor]);
                        cursor++;
                     }
                     else if(pCode[cursor] == ',')
                     {
                        arguments.emplace_back();
                        cursor++;

                        while(pCode[cursor] == ' ' || pCode[cursor] == '\n')
                        {
                           cursor++;
                        }
                     }
                     else
                     {
                        arguments.back().push_back(pCode[cursor]);
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
      }

      // skip carriage return characters
      while(pCode[cursor] == '\r')
      {
         cursor++;
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

void Environment::tokenize(ParsingContext& pContext) const
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

   if(tokensCount != 0u)
   {
      if(tokensCount == 1u)
      {
         expression = parseExpressionSingleToken(pContext);
      }
      else
      {
         expression = parseExpressionMultipleTokens(pContext, pContext.mTokenIndex, pTokenLastIndex);
      }
   }

   if(!pNullAllowed && !expression)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
   }

   if(expression && !mErrorMessage.empty())
   {
      CflatInvokeDtor(Expression, expression);
      CflatFree(expression);
      expression = nullptr;
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

            char* endPtr = nullptr;
            const float number = strtof(numberStr, &endPtr);

            if(endPtr != (numberStr + token.mLength - 1))
            {
               throwCompileError(pContext, CompileError::InvalidNumericValue, numberStr);
               return nullptr;
            }

            value.initOnStack(typeUsage, &mExecutionContext.mStack);
            value.set(&number);
         }
         // double
         else
         {
            typeUsage.mType = mTypeDouble;

            char* endPtr = nullptr;
            const double number = strtod(numberStr, &endPtr);

            if(endPtr != (numberStr + token.mLength))
            {
               throwCompileError(pContext, CompileError::InvalidNumericValue, numberStr);
               return nullptr;
            }

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
            throwCompileError(pContext, CompileError::InvalidNumericValue, numberStr);
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
      // variable access / enum value
      pContext.mStringBuffer.assign(token.mStart, token.mLength);
      const Identifier identifier(pContext.mStringBuffer.c_str());

      Instance* instance = retrieveInstance(pContext, identifier);

      if(instance && instance->mTypeUsage.mType)
      {
         if(CflatHasFlag(instance->mFlags, InstanceFlags::EnumValue))
         {
            expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
            CflatInvokeCtor(ExpressionValue, expression)(instance->mValue);
         }
         else
         {
            expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
            CflatInvokeCtor(ExpressionVariableAccess, expression)(identifier, instance->mTypeUsage);
         }
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
         TypeUsage typeUsage = mTypeUsageVoidPtr;
         CflatSetFlag(typeUsage.mFlags, TypeUsageFlags::Const);

         expression = (ExpressionNullPointer*)CflatMalloc(sizeof(ExpressionNullPointer));
         CflatInvokeCtor(ExpressionNullPointer, expression)(typeUsage);
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
   else if(token.mType == TokenType::String || token.mType == TokenType::WideString)
   {
      expression = parseExpressionLiteralString(pContext, token.mType);
   }
   else if(token.mType == TokenType::Character || token.mType == TokenType::WideCharacter)
   {
      expression = parseExpressionLiteralCharacter(pContext, token.mType);
   }

   return expression;
}

Expression* Environment::parseExpressionMultipleTokens(ParsingContext& pContext,
   size_t pTokenFirstIndex, size_t pTokenLastIndex)
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
   uint32_t curlyBracketLevel = tokens[pTokenLastIndex].mStart[0] == '}' ? 1u : 0u;
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
         else if(tokens[i].mStart[0] == '}')
         {
            curlyBracketLevel++;
            continue;
         }
         else if(tokens[i].mStart[0] == '{')
         {
            curlyBracketLevel--;
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

      if(parenthesisLevel == 0u &&
         squareBracketLevel == 0u &&
         curlyBracketLevel == 0u &&
         templateLevel == 0u)
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
   TypeUsage cStyleCastTypeUsage;

   if(tokens[tokenIndex].mStart[0] == '(')
   {
      const size_t cachedTokenIndex = tokenIndex;
      tokenIndex++;

      cStyleCastTypeUsage = parseTypeUsage(pContext, pTokenLastIndex - 1u);
      isCStyleCast = cStyleCastTypeUsage.mType != nullptr && tokens[tokenIndex].mStart[0] == ')';

      if(!isCStyleCast)
      {
         tokenIndex = cachedTokenIndex;
      }
   }

   // C-style cast
   if(isCStyleCast)
   {
      tokenIndex++;
      Expression* expressionToCast = parseExpression(pContext, pTokenLastIndex);

      expression = (ExpressionCast*)CflatMalloc(sizeof(ExpressionCast));
      CflatInvokeCtor(ExpressionCast, expression)
         (CastType::CStyle, cStyleCastTypeUsage, expressionToCast);

      const TypeUsage& sourceTypeUsage = getTypeUsage(expressionToCast);

      if(!isCastAllowed(CastType::CStyle, sourceTypeUsage, cStyleCastTypeUsage))
      {
         throwCompileError(pContext, CompileError::InvalidCast);
      }
   }
   // assignment
   else if(assignmentOperatorTokenIndex > 0u)
   {
      Expression* left = parseExpression(pContext, assignmentOperatorTokenIndex - 1u);

      if(left)
      {
         const TypeUsage& leftTypeUsage = getTypeUsage(left);

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
      const size_t elseTokenIndex = findSeparationTokenIndex(pContext, ':', pTokenLastIndex);

      if(elseTokenIndex > 0u)
      {
         Expression* condition = parseExpression(pContext, conditionalTokenIndex - 1u);
         tokenIndex = conditionalTokenIndex + 1u;

         Expression* ifExpression = parseExpression(pContext, elseTokenIndex - 1u);
         tokenIndex = elseTokenIndex + 1u;

         Expression* elseExpression = parseExpression(pContext, pTokenLastIndex);
         tokenIndex = pTokenLastIndex + 1u;

         expression = (ExpressionConditional*)CflatMalloc(sizeof(ExpressionConditional));
         CflatInvokeCtor(ExpressionConditional, expression)(condition, ifExpression, elseExpression);
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
         const TypeUsage& leftTypeUsage = getTypeUsage(left);

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
               const TypeUsage& rightTypeUsage = getTypeUsage(right);

               if(rightTypeUsage.mType)
               {
                  CflatArgsVector(TypeUsage) args;
                  args.push_back(rightTypeUsage);

                  pContext.mStringBuffer.assign("operator");
                  pContext.mStringBuffer.append(operatorStr);
                  const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());

                  Struct* leftType = static_cast<Struct*>(leftTypeUsage.mType);
                  Method* operatorMethod = leftType->findMethod(operatorIdentifier, args);

                  if(operatorMethod)
                  {
                     overloadedOperatorTypeUsage = operatorMethod->mReturnTypeUsage;

                     if(!isMethodCallAllowed(operatorMethod, leftTypeUsage))
                     {
                        throwCompileError(pContext, CompileError::CannotCallNonConstMethod);
                        operatorIsValid = false;
                     }
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
                           CflatSTLString typeFullName;
                           getTypeFullName(leftTypeUsage.mType, &typeFullName);

                           throwCompileError(pContext, CompileError::InvalidOperator,
                              typeFullName.c_str(), operatorStr.c_str());
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
               TypeUsage typeUsage;

               if(overloadedOperatorTypeUsage.mType)
               {
                  typeUsage = overloadedOperatorTypeUsage;
               }
               else
               {
                  bool logicalOperator = false;

                  for(size_t i = 0u; i < kCflatLogicalOperatorsCount; i++)
                  {
                     if(strcmp(operatorStr.c_str(), kCflatLogicalOperators[i]) == 0)
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
                     const TypeUsage& rightTypeUsage = getTypeUsage(right);

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

               expression =
                  (ExpressionBinaryOperation*)CflatMalloc(sizeof(ExpressionBinaryOperation));
               CflatInvokeCtor(ExpressionBinaryOperation, expression)
                  (left, right, operatorStr.c_str(), typeUsage);
            }
            else
            {
               CflatInvokeDtor(Expression, left);
               CflatFree(left);
               CflatInvokeDtor(Expression, right);
               CflatFree(right);
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
   else if(tokens[pTokenFirstIndex].mType == TokenType::Operator ||
      tokens[pTokenLastIndex].mType == TokenType::Operator)
   {
      size_t operatorTokenIndex;
      size_t operandExpressionTokenFirstIndex;
      size_t operandExpressionTokenLastIndex;
      bool postOperator;

      if(tokens[pTokenLastIndex].mType == TokenType::Operator)
      {
         operatorTokenIndex = pTokenLastIndex;
         operandExpressionTokenFirstIndex = pTokenFirstIndex;
         operandExpressionTokenLastIndex = pTokenLastIndex - 1u;
         postOperator = true;
      }
      else
      {
         operatorTokenIndex = pTokenFirstIndex;
         operandExpressionTokenFirstIndex = pTokenFirstIndex + 1u;
         operandExpressionTokenLastIndex = pTokenLastIndex;
         postOperator = false;
      }

      pContext.mTokenIndex = operandExpressionTokenFirstIndex;
      Expression* operandExpression = parseExpression(pContext, operandExpressionTokenLastIndex);

      if(operandExpression)
      {
         const Token& operatorToken = tokens[operatorTokenIndex];

         char operatorStr[kSmallLocalStringBufferSize];
         strncpy(operatorStr, operatorToken.mStart, operatorToken.mLength);
         operatorStr[operatorToken.mLength] = '\0';

         expression =
            parseExpressionUnaryOperator(pContext, operandExpression, operatorStr, postOperator);
      }
   }
   // member access
   else if(memberAccessTokenIndex > 0u)
   {
      if(tokens[memberAccessTokenIndex + 1u].mType == TokenType::Identifier)
      {
         Expression* memberOwner = parseExpression(pContext, memberAccessTokenIndex - 1u);
         tokenIndex = memberAccessTokenIndex + 1u;

         if(memberOwner &&
            memberOwner->getTypeUsage().mType &&
            memberOwner->getTypeUsage().mType->mCategory == TypeCategory::StructOrClass)
         {
            pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
            const Identifier memberIdentifier(pContext.mStringBuffer.c_str());

            const bool isMemberAccess = tokens[memberAccessTokenIndex].mStart[0] == '.';
            const bool isPtrMemberAccess =
               !isMemberAccess && strncmp(tokens[memberAccessTokenIndex].mStart, "->", 2u) == 0;

            bool memberAccessIsValid = true;

            const TypeUsage& ownerTypeUsage = getTypeUsage(memberOwner);
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
                  Member* member = type->findMember(memberIdentifier);

                  if(member)
                  {
                     memberTypeUsage = member->mTypeUsage;
                  }
                  else
                  {
                     CflatSTLString typeFullName;
                     getTypeFullName(type, &typeFullName);

                     throwCompileError(pContext, CompileError::MissingMember,
                        memberIdentifier.mName, typeFullName.c_str());
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
                     Method* method =
                        static_cast<ExpressionMethodCall*>(expression)->mMethodUsage.mMethod;

                     if(method)
                     {
                        CflatInvokeCtor(ExpressionMemberAccess, memberAccess)
                           (memberOwner, memberIdentifier, method->mReturnTypeUsage);

                        if(!isMethodCallAllowed(method, ownerTypeUsage))
                        {
                           throwCompileError(pContext, CompileError::CannotCallNonConstMethod);
                        }
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

      ExpressionArrayInitialization* arrayInitialization =
         (ExpressionArrayInitialization*)CflatMalloc(sizeof(ExpressionArrayInitialization));
      CflatInvokeCtor(ExpressionArrayInitialization, arrayInitialization)();
      expression = arrayInitialization;

      const size_t closureIndex = findClosureTokenIndex(pContext, '{', '}', pTokenLastIndex);

      while(tokenIndex < closureIndex)
      {
         const size_t separatorIndex = findSeparationTokenIndex(pContext, ',', closureIndex);
         const size_t lastArrayValueIndex = separatorIndex > 0u
            ? separatorIndex - 1u
            : closureIndex - 1u;

         Expression* arrayValueExpression = parseExpression(pContext, lastArrayValueIndex);
         arrayInitialization->mValues.push_back(arrayValueExpression);

         if(arrayInitialization->mElementTypeUsage.mType)
         {
            const TypeHelper::Compatibility compatibility = TypeHelper::getCompatibility(
               arrayValueExpression->getTypeUsage(), arrayInitialization->mElementTypeUsage);

            if(compatibility == TypeHelper::Compatibility::Incompatible)
            {
               throwCompileError(pContext, Environment::CompileError::NonHomogeneousTypeList);
               break;
            }
         }
         else
         {
            arrayInitialization->mElementTypeUsage = arrayValueExpression->getTypeUsage();
         }

         tokenIndex = lastArrayValueIndex + 2u;
      }

      arrayInitialization->assignTypeUsage();
   }
   // array element access / operator[]
   else if(tokens[pTokenLastIndex].mStart[0] == ']')
   {
      const size_t openingIndex = findOpeningTokenIndex(pContext, '[', ']', pTokenLastIndex);

      if(pTokenLastIndex > (openingIndex + 1u))
      {
         Expression* arrayAccess = parseExpression(pContext, openingIndex - 1u);

         if(arrayAccess)
         {
            tokenIndex = openingIndex + 1u;
            Expression* arrayElementIndex = parseExpression(pContext, pTokenLastIndex - 1u);

            TypeUsage typeUsage = getTypeUsage(arrayAccess);

            if(typeUsage.isArray() || typeUsage.isPointer())
            {
               if(typeUsage.isArray())
               {
                  CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Array);
                  typeUsage.mArraySize = 1u;
               }
               else
               {
                  typeUsage.mPointerLevel--;
               }

               expression =
                  (ExpressionArrayElementAccess*)CflatMalloc(sizeof(ExpressionArrayElementAccess));
               CflatInvokeCtor(ExpressionArrayElementAccess, expression)
                  (arrayAccess, arrayElementIndex, typeUsage);
            }
            else if(typeUsage.mType->mCategory == TypeCategory::StructOrClass)
            {
               Struct* type = static_cast<Struct*>(typeUsage.mType);
               const Identifier operatorMethodID("operator[]");
               Method* operatorMethod = type->findMethod(operatorMethodID);

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
                  methodCall->mMethodUsage.mMethod = operatorMethod;
                  methodCall->assignTypeUsage(mTypeUsageVoid);
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

      TypeUsage templateTypeUsage;
         
      if(!isFunctionCall && isTemplate(pContext, pTokenLastIndex))
      {
         const size_t templateClosureIndex =
            findClosureTokenIndex(pContext, '<', '>', pTokenLastIndex);
         isFunctionCall =
            templateClosureIndex > 0u && tokens[templateClosureIndex + 1u].mStart[0] == '(';

         if(isFunctionCall)
         {
            pContext.mTokenIndex = pTokenFirstIndex;
            templateTypeUsage = parseTypeUsage(pContext, templateClosureIndex);

            if(!templateTypeUsage.mType)
            {
               pContext.mTokenIndex++;
            }
         }
      }

      // function call / object construction
      if(isFunctionCall)
      {
         Type* type = templateTypeUsage.mType
            ? templateTypeUsage.mType
            : findType(pContext, fullIdentifier);
         expression = type
            ? parseExpressionObjectConstruction(pContext, type)
            : parseExpressionFunctionCall(pContext, fullIdentifier);
      }
      // aggregate initialization
      else if(tokens[tokenIndex].mStart[0] == '{')
      {
         Type* type = findType(pContext, fullIdentifier);

         if(type)
         {
            expression = parseExpressionAggregateInitialization(pContext, type, pTokenLastIndex);
         }
      }
      // variable access with namespace / static member access
      else
      {
         Instance* variableInstance = retrieveInstance(pContext, fullIdentifier);
         Instance* enumInstance = nullptr;

         if(!variableInstance)
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

               if(type)
               {
                  if(type->mCategory == TypeCategory::StructOrClass)
                  {
                     variableInstance =
                        static_cast<Struct*>(type)->mInstancesHolder.retrieveInstance(memberIdentifier);

                     if(!variableInstance)
                     {
                        CflatSTLString typeFullName;
                        getTypeFullName(type, &typeFullName);

                        throwCompileError(pContext, Environment::CompileError::MissingStaticMember,
                           memberIdentifier.mName, typeFullName.c_str());
                     }
                  }
                  else if(type->mCategory == TypeCategory::Enum)
                  {
                     enumInstance =
                        static_cast<Enum*>(type)->mInstancesHolder.retrieveInstance(memberIdentifier);
                  }
                  else if(type->mCategory == TypeCategory::EnumClass)
                  {
                     enumInstance =
                        static_cast<EnumClass*>(type)->mInstancesHolder.retrieveInstance(memberIdentifier);
                  }
               }
            }
         }

         if(variableInstance)
         {
            expression = (ExpressionVariableAccess*)CflatMalloc(sizeof(ExpressionVariableAccess));
            CflatInvokeCtor(ExpressionVariableAccess, expression)
               (fullIdentifier, variableInstance->mTypeUsage);
         }
         else if(enumInstance)
         {
            expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
            CflatInvokeCtor(ExpressionValue, expression)(enumInstance->mValue);
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
            CflatInvokeCtor(ExpressionSizeOf, concreteExpression)(mTypeUsageSizeT);
            expression = concreteExpression;

            concreteExpression->mSizeOfTypeUsage = parseTypeUsage(pContext, closureTokenIndex - 1u);

            if(!concreteExpression->mSizeOfTypeUsage.mType)
            {
               concreteExpression->mSizeOfExpression = parseExpression(pContext, closureTokenIndex - 1u);
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
   else if(token.mType == TokenType::String || token.mType == TokenType::WideString)
   {
      expression = parseExpressionLiteralString(pContext, token.mType);
   }
   else if(token.mType == TokenType::Character || token.mType == TokenType::WideCharacter)
   {
      expression = parseExpressionLiteralCharacter(pContext, token.mType);
   }

   return expression;
}

Expression* Environment::parseExpressionLiteralString(ParsingContext& pContext, TokenType pTokenType)
{
   pContext.mStringBuffer.clear();

   const size_t firstIndex = pTokenType == TokenType::String ? 1u : 2u;

   do
   {
      const Token& token = pContext.mTokens[pContext.mTokenIndex];

      for(size_t i = firstIndex; i < (token.mLength - 1u); i++)
      {
         const char currentChar = *(token.mStart + i);

         if(currentChar == '\\')
         {
            const char escapeChar = *(token.mStart + i + 1u);

            if(escapeChar == 'n')
            {
               pContext.mStringBuffer.push_back('\n');
            }
            else if(escapeChar == '\\')
            {
               pContext.mStringBuffer.push_back('\\');
            }
            else if(escapeChar == 't')
            {
               pContext.mStringBuffer.push_back('\t');
            }
            else if(escapeChar == 'r')
            {
               pContext.mStringBuffer.push_back('\r');
            }
            else if(escapeChar == '"')
            {
               pContext.mStringBuffer.push_back('\"');
            }
            else if(escapeChar == '\'')
            {
               pContext.mStringBuffer.push_back('\'');
            }
            else if(escapeChar == '0')
            {
               pContext.mStringBuffer.push_back('\0');
            }
            else
            {
               pContext.mStringBuffer.clear();
               pContext.mStringBuffer.push_back(escapeChar);
               
               throwCompileError(pContext, CompileError::InvalidEscapeSequence,
                  pContext.mStringBuffer.c_str());
               
               return nullptr;
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
      pContext.mTokens[pContext.mTokenIndex].mType == pTokenType);

   pContext.mStringBuffer.push_back('\0');

   const Hash stringHash = hash(pContext.mStringBuffer.c_str());
   Value value;

   if(pTokenType == TokenType::String)
   {
      const char* string =
         mLiteralStringsPool.registerString(stringHash, pContext.mStringBuffer.c_str());

      value.initOnStack(mTypeUsageCString, &mExecutionContext.mStack);
      value.set(&string);
   }
   else
   {
      const wchar_t* string =
         mLiteralWideStringsPool.registerString(stringHash, pContext.mStringBuffer.c_str());

      value.initOnStack(mTypeUsageWideString, &mExecutionContext.mStack);
      value.set(&string);
   }

   ExpressionValue* expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
   CflatInvokeCtor(ExpressionValue, expression)(value);

   return expression;
}

Expression* Environment::parseExpressionLiteralCharacter(ParsingContext& pContext, TokenType pTokenType)
{
   const Token& token = pContext.mTokens[pContext.mTokenIndex];
   const size_t expectedTokenSize = pTokenType == TokenType::Character ? 3u : 4u;

   if(token.mLength != expectedTokenSize)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   Value value;

   if(pTokenType == TokenType::Character)
   {
      const char character = token.mStart[1];

      value.initOnStack(mTypeUsageCharacter, &mExecutionContext.mStack);
      value.set(&character);
   }
   else
   {
      const wchar_t character = (wchar_t)token.mStart[2];

      value.initOnStack(mTypeUsageWideCharacter, &mExecutionContext.mStack);
      value.set(&character);
   }

   ExpressionValue* expression = (ExpressionValue*)CflatMalloc(sizeof(ExpressionValue));
   CflatInvokeCtor(ExpressionValue, expression)(value);

   return expression;
}

Expression* Environment::parseExpressionUnaryOperator(ParsingContext& pContext, Expression* pOperand,
   const char* pOperator, bool pPostOperator)
{
   Expression* expression = nullptr;
   bool validOperation = true;

   const TypeUsage& operandTypeUsage = getTypeUsage(pOperand);

   Method* operatorMethod = nullptr;
   Function* operatorFunction = nullptr;
   TypeUsage overloadedOperatorTypeUsage;

   if(operandTypeUsage.mType &&
      operandTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
      !operandTypeUsage.isPointer())
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      Struct* operandType = static_cast<Struct*>(operandTypeUsage.mType);
      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      operatorMethod = operandType->findMethod(operatorIdentifier, TypeUsage::kEmptyList());
      
      if(operatorMethod)
      {
         if(isMethodCallAllowed(operatorMethod, operandTypeUsage))
         {
            overloadedOperatorTypeUsage = operatorMethod->mReturnTypeUsage;
         }
         else
         {
            validOperation = false;
            throwCompileError(pContext, CompileError::CannotCallNonConstMethod);
         }
      }
      else
      {
         CflatArgsVector(TypeUsage) parameterTypes;
         parameterTypes.push_back(operandTypeUsage);

         operatorFunction =
            operandTypeUsage.mType->mNamespace->getFunction(operatorIdentifier, parameterTypes);

         if(!operatorFunction)
         {
            operatorFunction = findFunction(pContext, operatorIdentifier, parameterTypes);
         }

         if(operatorFunction)
         {
            overloadedOperatorTypeUsage = operatorFunction->mReturnTypeUsage;
         }
      }
   }

   if(!operatorMethod && !operatorFunction)
   {
      const bool isIncrementOrDecrement =
         strncmp(pOperator, "++", 2u) == 0 ||
         strncmp(pOperator, "--", 2u) == 0;

      if(isIncrementOrDecrement)
      {
         if(operandTypeUsage.isConst())
         {
            throwCompileError(pContext, Environment::CompileError::CannotModifyConstExpression);
            validOperation = false;
         }
      }
   }

   if(validOperation)
   {
      TypeUsage typeUsage;

      if(overloadedOperatorTypeUsage.mType)
      {
         typeUsage = overloadedOperatorTypeUsage;
      }
      else
      {
         typeUsage = pOperator[0] == '!'
            ? mTypeUsageBool
            : getTypeUsage(pOperand);
         CflatResetFlag(typeUsage.mFlags, TypeUsageFlags::Reference);

         if(pOperator[0] == '&')
         {
            typeUsage.mPointerLevel++;
         }
         else if(pOperator[0] == '*')
         {
            typeUsage.mPointerLevel--;
         }
      }

      expression = (ExpressionUnaryOperation*)CflatMalloc(sizeof(ExpressionUnaryOperation));
      CflatInvokeCtor(ExpressionUnaryOperation, expression)
         (pOperand, pOperator, pPostOperator, typeUsage);
   }

   return expression;
}

Expression* Environment::parseExpressionCast(ParsingContext& pContext, CastType pCastType,
   size_t pTokenLastIndex)
{
   if(pCastType == CastType::Dynamic && CflatHasFlag(mSettings, Settings::DisallowDynamicCast))
   {
      throwCompileError(pContext, CompileError::DynamicCastNotAllowed);
      return nullptr;
   }

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
               const size_t closureTokenIndex =
                  findClosureTokenIndex(pContext, '(', ')', pTokenLastIndex);

               if(closureTokenIndex > 0u)
               {
                  tokenIndex++;

                  Expression* expressionToCast =
                     parseExpression(pContext, closureTokenIndex - 1u);

                  if(expressionToCast)
                  {
                     const TypeUsage& sourceTypeUsage = getTypeUsage(expressionToCast);

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
      const TypeUsage& typeUsage = getTypeUsage(expression->mArguments[i]);
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
            Struct* castType = static_cast<Struct*>(type);
            expression->mFunction =
               castType->findStaticMethod(staticMethodIdentifier, argumentTypes, templateTypes);

            if(!expression->mFunction)
            {
               CflatSTLString typeFullName;
               getTypeFullName(type, &typeFullName);

               throwCompileError(pContext, CompileError::MissingStaticMethod,
                  staticMethodIdentifier.mName, typeFullName.c_str());
            }
         }
      }
   }

   if(expression->mFunction)
   {
      expression->assignTypeUsage(mTypeUsageVoid);
   }
   else
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
   const TypeUsage& methodOwnerTypeUsage = getTypeUsage(memberAccess->mMemberOwner);
   CflatAssert(methodOwnerTypeUsage.mType);
   CflatAssert(methodOwnerTypeUsage.mType->mCategory == TypeCategory::StructOrClass);

   Struct* methodOwnerType = static_cast<Struct*>(methodOwnerTypeUsage.mType);
   CflatArgsVector(TypeUsage) argumentTypes;

   for(size_t i = 0u; i < expression->mArguments.size(); i++)
   {
      const TypeUsage& typeUsage = getTypeUsage(expression->mArguments[i]);
      argumentTypes.push_back(typeUsage);
   }

   CflatArgsVector(TypeUsage) templateTypes;
   toArgsVector(expression->mTemplateTypes, templateTypes);

   const Identifier& methodId = memberAccess->mMemberIdentifier;
   expression->mMethodUsage =
      methodOwnerType->findMethodUsage(methodId, 0u, argumentTypes, templateTypes);

   if(expression->mMethodUsage.mMethod)
   {
      expression->assignTypeUsage(mTypeUsageVoid);
   }
   else
   {
      CflatSTLString typeFullName;
      getTypeFullName(methodOwnerType, &typeFullName);

      throwCompileError(pContext, CompileError::MissingMethod, methodId.mName, typeFullName.c_str());
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
      const TypeUsage& typeUsage = getTypeUsage(expression->mArguments[i]);
      argumentTypes.push_back(typeUsage);
   }

   if(pType->mCategory == TypeCategory::StructOrClass)
   {
      Struct* type = static_cast<Struct*>(pType);
      expression->mConstructor = type->findConstructor(argumentTypes);
   }

   if(!expression->mConstructor)
   {
      CflatSTLString typeFullName;
      getTypeFullName(pType, &typeFullName);

      throwCompileError(pContext, CompileError::MissingConstructor, typeFullName.c_str());
   }

   return expression;
}

Expression* Environment::parseExpressionAggregateInitialization(ParsingContext& pContext, Type* pType,
   size_t pTokenLastIndex)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);
   Struct* type = static_cast<Struct*>(pType);

   ExpressionAggregateInitialization* expression =
      (ExpressionAggregateInitialization*)CflatMalloc(sizeof(ExpressionAggregateInitialization));
   CflatInvokeCtor(ExpressionAggregateInitialization, expression)(pType);

   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   tokenIndex++;

   size_t typeMemberIndex = 0u;
   const size_t closureIndex = findClosureTokenIndex(pContext, '{', '}', pTokenLastIndex);

   while(tokenIndex < closureIndex)
   {
      if(typeMemberIndex == type->mMembers.size())
      {
         throwCompileError(pContext, Environment::CompileError::TooManyArgumentsInAggregate);
         break;
      }

      const size_t separatorIndex = findSeparationTokenIndex(pContext, ',', closureIndex);
      const size_t lastValueIndex = separatorIndex > 0u
         ? separatorIndex - 1u
         : closureIndex - 1u;

      Expression* valueExpression = parseExpression(pContext, lastValueIndex);
      expression->mValues.push_back(valueExpression);

      if(valueExpression)
      {
         const TypeHelper::Compatibility compatibility = TypeHelper::getCompatibility(
            valueExpression->getTypeUsage(), type->mMembers[typeMemberIndex].mTypeUsage);

         if(compatibility == TypeHelper::Compatibility::Incompatible)
         {
            throwCompileError(pContext, Environment::CompileError::MismatchingTypeInAggregate,
               type->mMembers[typeMemberIndex].mIdentifier.mName);
            break;
         }
      }
      else
      {
         break;
      }

      tokenIndex = lastValueIndex + 2u;
      typeMemberIndex++;
   }

   return expression;
}

size_t Environment::findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
   size_t pTokenIndexLimit) const
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
   size_t pClosureIndex) const
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
   size_t pClosureIndex) const
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

      if(tokens[i].mStart[0] == '(' || tokens[i].mStart[0] == '{')
      {
         scopeLevel++;
      }
      else if(tokens[i].mStart[0] == ')' || tokens[i].mStart[0] == '}')
      {
         scopeLevel--;
      }
   }

   return separationTokenIndex;
}

uint8_t Environment::getBinaryOperatorPrecedence(ParsingContext& pContext, size_t pTokenIndex) const
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

bool Environment::isTemplate(ParsingContext& pContext, size_t pOpeningTokenIndex, size_t pClosureTokenIndex) const
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
         if(tokens[i].mStart[0] == '<' && tokens[i].mLength == 1u)
         {
            const size_t cachedTokenIndex = pContext.mTokenIndex;
            pContext.mTokenIndex = i;
            const size_t innerTemplateClosureTokenIndex =
               findClosureTokenIndex(pContext, '<', '>', pClosureTokenIndex - 1u);
            pContext.mTokenIndex = cachedTokenIndex;

            if(isTemplate(pContext, i, innerTemplateClosureTokenIndex))
            {
               i = innerTemplateClosureTokenIndex;
               continue;
            }
            else
            {
               return false;
            }
         }

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

bool Environment::isTemplate(ParsingContext& pContext, size_t pTokenLastIndex) const
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;

   if(tokens[tokenIndex].mLength != 1u || tokens[tokenIndex].mStart[0] != '<')
      return false;

   const size_t templateClosureTokenIndex =
      findClosureTokenIndex(pContext, '<', '>', pTokenLastIndex);

   return isTemplate(pContext, tokenIndex, templateClosureTokenIndex);
}

bool Environment::isCastAllowed(CastType pCastType, const TypeUsage& pFrom, const TypeUsage& pTo) const
{
   if(!pFrom.mType || !pTo.mType)
   {
      return false;
   }

   if(pTo == pFrom)
   {
      return true;
   }

   if(pFrom.isPointer() && pTo.isPointer() && (pFrom == mTypeUsageVoidPtr || pTo == mTypeUsageVoidPtr))
   {
      return true;
   }

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
      else if(pFrom.mType->isInteger() && pTo.mType->isInteger())
      {
         castAllowed = true;
      }
      else if(pFrom.mType->isInteger() && pTo.mType->isDecimal())
      {
         castAllowed = true;
      }
      else if(pFrom.mType->isDecimal() && pTo.mType->isInteger())
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

bool Environment::isMethodCallAllowed(Method* pMethod, const TypeUsage& pOwnerTypeUsage) const
{
   if(!CflatHasFlag(pMethod->mFlags, MethodFlags::Const))
   {
      if((pOwnerTypeUsage.isPointer() && pOwnerTypeUsage.isConstPointer()) ||
         (!pOwnerTypeUsage.isPointer() && pOwnerTypeUsage.isConst()))
      {
         return false;
      }
   }

   return true;
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

      if(token.mLength == 6u && strncmp(token.mStart, "static", 6u) == 0)
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

         if (!pContext.mStringBuffer.empty() && !Tokenizer::isValidIdentifierBeginningCharacter(*pContext.mStringBuffer.c_str()))
         {
            throwCompileError(pContext, CompileError::UnexpectedSymbol, pContext.mStringBuffer.c_str());
            return nullptr;
         }

         bool isFunctionDeclaration = tokens[tokenIndex].mStart[0] == '(';

         if(isFunctionDeclaration)
         {
             size_t savedTokenIndex = tokenIndex;
             // Check if the next token after the opening parenthesis is either a
             // typeusage or a closing parenthesis. This would be indicating that
             // we're dealing with a function declaration and not a variable
             // declaration.
             tokenIndex++;

             if ((savedTokenIndex + 1u) < tokens.size() &&
                 tokens[savedTokenIndex + 1].mStart[0] != ')')
             {
                 // The next token after the opening parenthesis is not a  closing parenthesis

                 // Checks if the next token after the opening parenthesis is a type description which
                 // would indicate that it is a function declaration
                 TypeUsage parameterTypeUsage = parseTypeUsage(pContext, 0u);
                 if (!parameterTypeUsage.mType)
                 {
                     // Would indicate object construction/variable declaration
                     isFunctionDeclaration = false;
                 }
             }

             tokenIndex = savedTokenIndex;
         }

         // function declaration
         if(isFunctionDeclaration)
         {
            tokenIndex--;
            statement = parseStatementFunctionDeclaration(pContext, typeUsage, staticDeclaration);
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
   if(pStatic &&
      CflatHasFlag(mSettings, Settings::DisallowStaticPointers) &&
      pTypeUsage.isPointer() &&
      pTypeUsage != mTypeUsageCString &&
      pTypeUsage != mTypeUsageWideString)
   {
      throwCompileError(pContext, CompileError::StaticPointersNotAllowed);
      return nullptr;
   }

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
               const TypeUsage& initialValueTypeUsage = getTypeUsage(initialValueExpression);

               const bool autoConst = pTypeUsage.isConst();
               const bool autoReference = pTypeUsage.isReference();

               pTypeUsage = initialValueTypeUsage;

               if(autoConst)
               {
                  CflatSetFlag(pTypeUsage.mFlags, TypeUsageFlags::Const);
               }
               else
               {
                  bool resetConstFlag = true;

                  // Exception #1: keep 'const' if the initial value is the result of a function call
                  // whose return type is a const ref, and the variable has been declared as auto&
                  if(autoReference &&
                     initialValueTypeUsage.isConst() && initialValueTypeUsage.isReference())
                  {
                     resetConstFlag = false;
                  }
                  // Exception #2: keep 'const' for C-strings
                  else if(initialValueTypeUsage == mTypeUsageCString ||
                     initialValueTypeUsage == mTypeUsageWideString)
                  {
                     resetConstFlag = false;
                  }

                  if(resetConstFlag)
                  {
                     CflatResetFlag(pTypeUsage.mFlags, TypeUsageFlags::Const);
                  }
               }

               if(autoReference)
               {
                  CflatSetFlag(pTypeUsage.mFlags, TypeUsageFlags::Reference);
               }
               else
               {
                  CflatResetFlag(pTypeUsage.mFlags, TypeUsageFlags::Reference);
               }
            }
            // Assigning something to a const ref
            else if (initialValueExpression && pTypeUsage.isConst() && pTypeUsage.isReference())
            {
               const bool isFunctionCall =
                  initialValueExpression->getType() == ExpressionType::FunctionCall ||
                  initialValueExpression->getType() == ExpressionType::MethodCall;

               if (isFunctionCall)
               {
                  const TypeUsage& initialValueTypeUsage = getTypeUsage(initialValueExpression);

                  // If a value is being assigned to a const ref, treat it like a const value
                  if (!initialValueTypeUsage.isReference())
                  {
                     CflatResetFlag(pTypeUsage.mFlags, TypeUsageFlags::Reference);
                  }
               }
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
         Struct* type = static_cast<Struct*>(pTypeUsage.mType);

         if(token.mStart[0] == '(')
         {
            initialValueExpression = parseExpressionObjectConstruction(pContext, type);
         }
         else
         {
            const Identifier emptyId;
            Method* anyCtor = type->findMethod(emptyId);

            if(anyCtor)
            {
               Method* defaultCtor = type->getDefaultConstructor();

               if(!defaultCtor)
               {
                  CflatSTLString typeFullName;
                  getTypeFullName(type, &typeFullName);

                  throwCompileError(pContext, CompileError::NoDefaultConstructor,
                     typeFullName.c_str());
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

      pContext.mRegisteredInstances.emplace_back();
      ParsingContext::RegisteredInstance& registeredInstance = pContext.mRegisteredInstances.back();
      registeredInstance.mIdentifier = pIdentifier;
      registeredInstance.mNamespace = pContext.mNamespaceStack.back();
      registeredInstance.mScopeLevel = pContext.mScopeLevel;

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
            const TypeUsage& initialValueTypeUsage = getTypeUsage(initialValueExpression);

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
            if(pStatic && pTypeUsage.isConst() && pContext.mScopeLevel == 0u)
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
   const TypeUsage& pReturnType, bool pStatic)
{
   CflatSTLVector(Token)& tokens = pContext.mTokens;
   size_t& tokenIndex = pContext.mTokenIndex;
   const Token& token = tokens[tokenIndex];

   pContext.mStringBuffer.assign(token.mStart, token.mLength);
   const Identifier functionIdentifier(pContext.mStringBuffer.c_str());
   const size_t functionToken = tokenIndex;

   if(pReturnType.mType->mCategory == TypeCategory::StructOrClass &&
      !pReturnType.isPointer() &&
      !pReturnType.isReference())
   {
      Struct* returnType = static_cast<Struct*>(pReturnType.mType);
      Method* copyConstructor = returnType->getCopyConstructor();

      if(!copyConstructor)
      {
         CflatSTLString typeFullName;
         getTypeFullName(pReturnType.mType, &typeFullName);

         throwCompileError(pContext, CompileError::NoCopyConstructor, typeFullName.c_str());
         return nullptr;
      }
   }

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

      pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
      Identifier parameterIdentifier(pContext.mStringBuffer.c_str());

      for(size_t i = 0u; i < statement->mParameterIdentifiers.size(); i++)
      {
         if(parameterIdentifier == statement->mParameterIdentifiers[i])
         {
            pContext.mStringBuffer.assign(tokens[tokenIndex].mStart, tokens[tokenIndex].mLength);
            throwCompileError(pContext, Environment::CompileError::ParameterRedefinition,
               parameterIdentifier.mName);
            return statement;
         }
      }

      tokenIndex++;

      statement->mParameterTypes.push_back(parameterType);
      statement->mParameterIdentifiers.push_back(parameterIdentifier);

      pContext.mScopeLevel++;
      registerInstance(pContext, parameterType, parameterIdentifier);
      pContext.mScopeLevel--;
   }

   CflatArgsVector(TypeUsage) parameterTypes;
   toArgsVector(statement->mParameterTypes, parameterTypes);

   Namespace* ns = pContext.mNamespaceStack.back();
   Function* function = ns->getFunctionPerfectMatch(statement->mFunctionIdentifier, parameterTypes);

   if(!function)
   {
      function = ns->registerFunction(statement->mFunctionIdentifier);
      function->mProgram = pContext.mProgram;
      function->mLine = token.mLine;

      for(size_t i = 0u; i < statement->mParameterTypes.size(); i++)
      {
         function->mParameters.push_back(statement->mParameterTypes[i]);
         function->mParameterIdentifiers.push_back(statement->mParameterIdentifiers[i]);
      }
   }

   function->mReturnTypeUsage = statement->mReturnType;

   if(pStatic)
   {
      CflatSetFlag(function->mFlags, FunctionFlags::Static);
   }
   else
   {
      CflatResetFlag(function->mFlags, FunctionFlags::Static);
   }

   pContext.mCurrentFunction = function;

   if (tokens[tokenIndex].mStart[0] != ';')
   {
       statement->mBody = parseStatementBlock(pContext, true, true);
   }

   pContext.mCurrentFunction = nullptr;

   if(statement->mBody && pReturnType.mType != mTypeVoid)
   {
      bool allCodePathsReturn = containsReturnStatement(statement->mBody);

      if(!allCodePathsReturn)
      {
         for(int i = (int)statement->mBody->mStatements.size() - 1; i >= 0; i--)
         {
            if(statement->mBody->mStatements[i]->getType() == StatementType::If)
            {
               StatementIf* ifStatement =
                  static_cast<StatementIf*>(statement->mBody->mStatements[i]);
               Statement* elseStatement = ifStatement->mElseStatement;

               while(elseStatement)
               {
                  if(containsReturnStatement(elseStatement))
                  {
                     allCodePathsReturn = true;
                     break;
                  }

                  elseStatement = elseStatement->getType() == StatementType::If
                     ? static_cast<StatementIf*>(elseStatement)->mElseStatement
                     : nullptr;
               }

               if(allCodePathsReturn)
               {
                  allCodePathsReturn = containsReturnStatement(ifStatement->mIfStatement);
               }
            }
            else if(statement->mBody->mStatements[i]->getType() == StatementType::Switch)
            {
               StatementSwitch* switchStatement =
                  static_cast<StatementSwitch*>(statement->mBody->mStatements[i]);

               if(!switchStatement->mCaseSections.empty() &&
                  !switchStatement->mCaseSections.back().mExpression)
               {
                  const CflatSTLVector(Statement*)& defaultCaseStatements =
                     switchStatement->mCaseSections.back().mStatements;

                  for(int j = (int)defaultCaseStatements.size() - 1; j >= 0; j--)
                  {
                     if(containsReturnStatement(defaultCaseStatements[i]))
                     {
                        allCodePathsReturn = true;
                        break;
                     }
                  }
               }
            }

            if(allCodePathsReturn)
            {
               break;
            }
         }
      }

      if(!allCodePathsReturn)
      {
         tokenIndex = functionToken;
         throwCompileError(pContext, CompileError::MissingReturnStatement, functionIdentifier.mName);
      }
   }

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

   if(pContext.mScopeLevel > 0u)
   {
      CflatAssert(pContext.mCurrentFunction);
      ns = ns->requestNamespace(pContext.mCurrentFunction->mIdentifier);

      char buffer[kDefaultLocalStringBufferSize];
      snprintf(buffer, sizeof(buffer), "__local%u", pContext.mLocalNamespaceGlobalIndex);
      const Identifier internalNamespaceIdentifier(buffer);
      ns = ns->requestNamespace(internalNamespaceIdentifier);

      pContext.mLocalNamespaceStack.emplace_back();
      ParsingContext::LocalNamespace& localNamespace = pContext.mLocalNamespaceStack.back();
      localNamespace.mNamespace = ns;
      localNamespace.mScopeLevel = pContext.mScopeLevel;

      pContext.mLocalNamespaceGlobalIndex++;
   }

   Type* existingType = ns->getType(structIdentifier);

   if(existingType)
   {
      ns->deregisterType(existingType);
   }

   statement->mStruct = ns->registerType<Struct>(structIdentifier);

   size_t structSize = 0u;
   size_t structAlignment = 1u;

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

      const size_t typeUsageSize = typeUsage.getSize();
      const size_t typeUsageAlignment = TypeHelper::calculateAlignment(typeUsage);

      if(typeUsageAlignment > structAlignment)
      {
         structAlignment = typeUsageAlignment;
      }

      const size_t misalignment = structSize % typeUsageAlignment;

      if(misalignment > 0u)
      {
         const size_t padding = typeUsageAlignment - misalignment;
         structSize += padding;
      }

      const Identifier memberIdentifier(pContext.mStringBuffer.c_str());
      Member member(memberIdentifier);
      member.mTypeUsage = typeUsage;
      member.mOffset = (uint16_t)structSize;

      statement->mStruct->mMembers.push_back(member);
      structSize += typeUsageSize;
   }
   while(++tokenIndex < closureTokenIndex);

   const size_t misalignment = structSize % structAlignment;

   if(misalignment > 0u)
   {
      const size_t padding = structAlignment - misalignment;
      structSize += padding;
   }

   statement->mStruct->mSize = structSize;
   statement->mStruct->mAlignment = (uint8_t)structAlignment;

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
   const TypeUsage& collectionTypeUsage = getTypeUsage(collection);

   if(collectionTypeUsage.isArray() && !variableTypeUsage.isArray() &&
      collectionTypeUsage.mPointerLevel == variableTypeUsage.mPointerLevel)
   {
      if(variableTypeUsage.mType == mTypeAuto)
      {
         variableTypeUsage.mType = collectionTypeUsage.mType;
      }

      TypeUsage elementTypeUsage;
      elementTypeUsage.mType = collectionTypeUsage.mType;

      const TypeHelper::Compatibility compatibility =
         TypeHelper::getCompatibility(elementTypeUsage, variableTypeUsage);
      validStatement = compatibility == TypeHelper::Compatibility::PerfectMatch;
   }
   else if(collectionTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
   {
      Struct* collectionType = static_cast<Struct*>(collectionTypeUsage.mType);
      Method* beginMethod = collectionType->findMethod("begin", TypeUsage::kEmptyList());

      if(beginMethod)
      {
         Method* endMethod = collectionType->findMethod("end", TypeUsage::kEmptyList());

         if(endMethod && beginMethod->mReturnTypeUsage == endMethod->mReturnTypeUsage)
         {
            const TypeUsage iteratorTypeUsage = beginMethod->mReturnTypeUsage;

            if(iteratorTypeUsage.mType->mCategory == TypeCategory::StructOrClass && !iteratorTypeUsage.isPointer())
            {
               Struct* castIteratorType = static_cast<Struct*>(iteratorTypeUsage.mType);
               Method* indirectionOperatorMethod =
                  castIteratorType->findMethod("operator*", TypeUsage::kEmptyList());

               if(indirectionOperatorMethod)
               {
                  CflatArgsVector(TypeUsage) nonEqualOperatorParameterTypes;
                  nonEqualOperatorParameterTypes.push_back(iteratorTypeUsage);

                  validStatement = 
                     castIteratorType->findMethod("operator!=", nonEqualOperatorParameterTypes) &&
                     castIteratorType->findMethod("operator++", TypeUsage::kEmptyList());

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
   if(pContext.mScopeLevel == 0u || !pContext.mCurrentFunction)
   {
      throwCompileErrorUnexpectedSymbol(pContext);
      return nullptr;
   }

   const size_t closureTokenIndex = findClosureTokenIndex(pContext, 0, ';');

   if(closureTokenIndex == 0u)
   {
      throwCompileError(pContext, CompileError::Expected, ";");
      return nullptr;
   }

   const size_t expressionTokenIndex = pContext.mTokenIndex;
   Expression* expression = parseExpression(pContext, closureTokenIndex - 1u, true);

   if(pContext.mCurrentFunction->mReturnTypeUsage != mTypeUsageVoid)
   {
      if(expression)
      {
         const TypeUsage& expressionTypeUsage = getTypeUsage(expression);
         const TypeHelper::Compatibility compatibility =
            TypeHelper::getCompatibility(pContext.mCurrentFunction->mReturnTypeUsage, expressionTypeUsage);

         if(compatibility == TypeHelper::Compatibility::Incompatible)
         {
            throwCompileError(pContext, CompileError::IncompatibleReturnExpressionType,
               pContext.mCurrentFunction->mIdentifier.mName);
         }
      }
      else
      {
         throwCompileError(pContext, CompileError::MissingReturnExpression);
      }
   }
   else
   {
      if(expression)
      {
         throwCompileError(pContext, CompileError::VoidFunctionReturningValue);
      }
   }

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

const TypeUsage& Environment::getTypeUsage(Expression* pExpression) const
{
   static const TypeUsage kDefaultTypeUsage;
   return pExpression && mErrorMessage.empty() ? pExpression->getTypeUsage() : kDefaultTypeUsage;
}

Type* Environment::findType(const Context& pContext, const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
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
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
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
   const CflatArgsVector(TypeUsage)& pTemplateTypes) const
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

Instance* Environment::retrieveInstance(Context& pContext, const Identifier& pIdentifier) const
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

      while(!parsingContext.mLocalNamespaceStack.empty() &&
         parsingContext.mLocalNamespaceStack.back().mScopeLevel >= pContext.mScopeLevel)
      {
         parsingContext.mLocalNamespaceStack.pop_back();
      }
   }

   while(!pContext.mTypeAliases.empty() &&
      pContext.mTypeAliases.back().mScopeLevel >= pContext.mScopeLevel)
   {
      pContext.mTypeAliases.pop_back();
   }

   pContext.mLocalInstancesHolder.releaseInstances(pContext.mScopeLevel, isExecutionContext);

   pContext.mScopeLevel--;
}

void Environment::throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg)
{
   if(!mErrorMessage.empty())
      return;

   char errorMsg[kDefaultLocalStringBufferSize];
   snprintf(errorMsg, sizeof(errorMsg), kRuntimeErrorStrings[(int)pError], pArg);

   char lineAsString[kSmallLocalStringBufferSize];
   snprintf(lineAsString, sizeof(lineAsString), "%d", pContext.mCallStack.back().mLine);

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

         const TypeUsage& arrayTypeUsage = getTypeUsage(expression->mArray);
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
               const size_t arrayElementSize = arrayElementTypeUsage.getSize();
               const size_t offset = arrayElementSize * index;
               pOutValue->set(arrayValue.mValueBuffer + offset);
            }
            else
            {
               char buffer[kDefaultLocalStringBufferSize];
               snprintf(buffer, sizeof(buffer), "size %zu, index %zu", arraySize, index);
               throwRuntimeError(pContext, RuntimeError::InvalidArrayIndex, buffer);
            }
         }
         else
         {
            const size_t arrayElementTypeSize = arrayElementTypeUsage.getSize();
            const size_t ptrOffset = arrayElementTypeSize * index;
            const char* ptr = CflatValueAs(&arrayValue, char*) + ptrOffset;
            memcpy(pOutValue->mValueBuffer, ptr, arrayElementTypeSize);
         }
      }
      break;
   case ExpressionType::UnaryOperation:
      {
         ExpressionUnaryOperation* expression = static_cast<ExpressionUnaryOperation*>(pExpression);

         const TypeUsage& typeUsage = getTypeUsage(expression);
         assertValueInitialization(pContext, typeUsage, pOutValue);

         Value preValue;
         preValue.initExternal(getTypeUsage(expression->mExpression));
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

         const TypeUsage& typeUsage = getTypeUsage(expression);
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
   case ExpressionType::SizeOf:
      {
         ExpressionSizeOf* expression = static_cast<ExpressionSizeOf*>(pExpression);
         size_t size = 0u;
         
         if(expression->mSizeOfTypeUsage.mType)
         {
            size = expression->mSizeOfTypeUsage.getSize();
         }
         else if(expression->mSizeOfExpression)
         {
            Value value;
            value.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, expression->mSizeOfExpression, &value);
            size = value.mTypeUsage.getSize();
         }

         assertValueInitialization(pContext, mTypeUsageSizeT, pOutValue);
         pOutValue->set(&size);
      }
      break;
   case ExpressionType::Cast:
      {
         ExpressionCast* expression = static_cast<ExpressionCast*>(pExpression);

         assertValueInitialization(pContext, expression->getTypeUsage(), pOutValue);

         Value valueToCast;
         valueToCast.mValueInitializationHint = ValueInitializationHint::Stack;
         evaluateExpression(pContext, expression->mExpression, &valueToCast);

         const TypeUsage& targetTypeUsage = expression->getTypeUsage();

         if(valueToCast.mTypeUsage == mTypeUsageVoidPtr ||
            targetTypeUsage == mTypeUsageVoidPtr ||
            expression->mCastType == CastType::Reinterpret)
         {
            const void* ptr = CflatValueAs(&valueToCast, void*);
            pOutValue->set(&ptr);
         }
         else if(expression->mCastType == CastType::CStyle || expression->mCastType == CastType::Static)
         {
            performStaticCast(pContext, valueToCast, targetTypeUsage, pOutValue);
         }
         else if(expression->mCastType == CastType::Dynamic)
         {
            performInheritanceCast(pContext, valueToCast, targetTypeUsage, pOutValue);
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

         const TypeUsage& expressionTypeUsage = getTypeUsage(expression->mRightValue);
         Value expressionValue;
         expressionValue.initOnStack(expressionTypeUsage, &pContext.mStack);
         evaluateExpression(pContext, expression->mRightValue, &expressionValue);

         const TypeUsage& ownerTypeUsage = getTypeUsage(expression->mLeftValue);
         Value instanceDataValue;
         instanceDataValue.initExternal(ownerTypeUsage);
         getInstanceDataValue(pContext, expression->mLeftValue, &instanceDataValue);

         if(instanceDataValue.mValueBuffer)
         {
            performAssignment(pContext, expressionValue, expression->mOperator, &instanceDataValue);
            *pOutValue = instanceDataValue;
         }
      }
      break;
   case ExpressionType::FunctionCall:
      {
         ExpressionFunctionCall* expression = static_cast<ExpressionFunctionCall*>(pExpression);

         Function* function = expression->mFunction;
         CflatAssert(function);

         if(function->execute)
         {
            assertValueInitialization(pContext, function->mReturnTypeUsage, pOutValue);

            CflatArgsVector(Value) argumentValues;
            getArgumentValues(pContext, function->mParameters, expression->mArguments, argumentValues);

            if(mErrorMessage.empty())
            {
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
            }

            while(!argumentValues.empty())
            {
               argumentValues.pop_back();
            }
         }
         else
         {
            throwRuntimeError(pContext, RuntimeError::MissingFunctionImplementation,
               function->mIdentifier.mName);
         }
      }
      break;
   case ExpressionType::MethodCall:
      {
         ExpressionMethodCall* expression = static_cast<ExpressionMethodCall*>(pExpression);
         ExpressionMemberAccess* memberAccess =
            static_cast<ExpressionMemberAccess*>(expression->mMemberAccess);

         Method* method = expression->mMethodUsage.mMethod;
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
         getArgumentValues(pContext, method->mParameters, expression->mArguments, argumentValues);

         if(mErrorMessage.empty())
         {
            CflatArgsVector(Value) preparedArgumentValues;
            prepareArgumentsForFunctionCall(pContext, method->mParameters, argumentValues,
               preparedArgumentValues);

            {
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

               if(expression->mMethodUsage.mOffset > 0u)
               {
                  const char* offsetThisPtr =
                     CflatValueAs(&thisPtr, char*) + expression->mMethodUsage.mOffset;
                  memcpy(thisPtr.mValueBuffer, &offsetThisPtr, sizeof(char*));
               }

               method->execute(thisPtr, preparedArgumentValues, pOutValue);
            }

            while(!preparedArgumentValues.empty())
            {
               preparedArgumentValues.pop_back();
            }
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

         assertValueInitialization(pContext, expression->getTypeUsage(), pOutValue);

         const size_t arrayElementSize = expression->mElementTypeUsage.getSize();

         for(size_t i = 0u; i < expression->mValues.size(); i++)
         {
            Value arrayElementValue;
            arrayElementValue.initOnStack(expression->mElementTypeUsage, &pContext.mStack);
            evaluateExpression(pContext, expression->mValues[i], &arrayElementValue);

            const size_t offset = i * arrayElementSize;
            memcpy(pOutValue->mValueBuffer + offset, arrayElementValue.mValueBuffer, arrayElementSize);
         }
      }
      break;
   case ExpressionType::AggregateInitialization:
      {
         ExpressionAggregateInitialization* expression =
            static_cast<ExpressionAggregateInitialization*>(pExpression);

         assertValueInitialization(pContext, expression->getTypeUsage(), pOutValue);

         Struct* type = static_cast<Struct*>(expression->getTypeUsage().mType);
         CflatAssert(type->mMembers.size() >= expression->mValues.size());

         Value objectPtrValue;
         getAddressOfValue(pContext, *pOutValue, &objectPtrValue);
         void* objectPtr = CflatValueAs(&objectPtrValue, void*);

         for(size_t i = 0u; i < expression->mValues.size(); i++)
         {
            Value memberValue;
            memberValue.mValueInitializationHint = ValueInitializationHint::Stack;
            evaluateExpression(pContext, expression->mValues[i], &memberValue);

            char* objectMemberPtr = (char*)objectPtr + (size_t)type->mMembers[i].mOffset;
            const size_t objectMemberSize = type->mMembers[i].mTypeUsage.getSize();
            memcpy(objectMemberPtr, memberValue.mValueBuffer, objectMemberSize);
         }
      }
      break;
   case ExpressionType::ObjectConstruction:
      {
         ExpressionObjectConstruction* expression =
            static_cast<ExpressionObjectConstruction*>(pExpression);

         Method* ctor = expression->mConstructor;
         CflatAssert(ctor);

         assertValueInitialization(pContext, expression->getTypeUsage(), pOutValue);

         CflatArgsVector(Value) argumentValues;
         getArgumentValues(pContext, ctor->mParameters, expression->mArguments, argumentValues);

         CflatArgsVector(Value) preparedArgumentValues;
         prepareArgumentsForFunctionCall(pContext, ctor->mParameters, argumentValues,
            preparedArgumentValues);

         {
            Value thisPtr;
            thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
            getAddressOfValue(pContext, *pOutValue, &thisPtr);

            ctor->execute(thisPtr, preparedArgumentValues, nullptr);
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

      evaluateExpression(pContext, memberAccess->mMemberOwner, &memberAccess->mMemberOwnerValue);

      if(memberAccess->mMemberOwnerValue.mTypeUsage.isPointer() &&
         !CflatValueAs(&memberAccess->mMemberOwnerValue, void*))
      {
         throwRuntimeError(pContext, RuntimeError::NullPointerAccess,
            memberAccess->mMemberIdentifier.mName);
      }

      if(!mErrorMessage.empty())
      {
         return;
      }

      Struct* type = static_cast<Struct*>(memberAccess->mMemberOwnerValue.mTypeUsage.mType);
      member = type->findMember(memberAccess->mMemberIdentifier);

      if(member)
      {
         instanceDataPtr = memberAccess->mMemberOwnerValue.mTypeUsage.isPointer()
            ? CflatValueAs(&memberAccess->mMemberOwnerValue, char*)
            : memberAccess->mMemberOwnerValue.mValueBuffer;
      }

      if(member && instanceDataPtr)
      {
         TypeUsage referenceTypeUsage = member->mTypeUsage;
         CflatSetFlag(referenceTypeUsage.mFlags, TypeUsageFlags::Reference);

         assertValueInitialization(pContext, referenceTypeUsage, pOutValue);
         pOutValue->set(instanceDataPtr + member->mOffset);
      }
   }
   else if(pExpression->getType() == ExpressionType::ArrayElementAccess)
   {
      ExpressionArrayElementAccess* arrayElementAccess =
         static_cast<ExpressionArrayElementAccess*>(pExpression);

      const TypeUsage& arrayTypeUsage = getTypeUsage(arrayElementAccess->mArray);
      const size_t arraySize = (size_t)arrayTypeUsage.mArraySize;

      Value arrayIndexValue;
      arrayIndexValue.mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, arrayElementAccess->mArrayElementIndex, &arrayIndexValue);
      const size_t arrayIndex = (size_t)getValueAsInteger(arrayIndexValue);

      if(arrayIndex < arraySize)
      {
         TypeUsage arrayElementTypeUsage = arrayTypeUsage;
         CflatResetFlag(arrayElementTypeUsage.mFlags, TypeUsageFlags::Array);
         arrayElementTypeUsage.mArraySize = 1u;

         Value arrayDataValue;
         arrayDataValue.initExternal(arrayElementTypeUsage);
         getInstanceDataValue(pContext, arrayElementAccess->mArray, &arrayDataValue);

         pOutValue->mValueBuffer =
            arrayDataValue.mValueBuffer + (arrayIndex * arrayElementTypeUsage.getSize());
      }
      else
      {
         char buffer[kDefaultLocalStringBufferSize];
         snprintf(buffer, sizeof(buffer), "size %zu, index %zu", arraySize, arrayIndex);
         throwRuntimeError(pContext, Environment::RuntimeError::InvalidArrayIndex, buffer);
      }
   }
   else if(pExpression->getType() == ExpressionType::UnaryOperation &&
      static_cast<ExpressionUnaryOperation*>(pExpression)->mOperator[0] == '*')
   {
      ExpressionUnaryOperation* unaryOperation = static_cast<ExpressionUnaryOperation*>(pExpression);

      Value value;
      value.mValueInitializationHint = ValueInitializationHint::Stack;
      evaluateExpression(pContext, unaryOperation->mExpression, &value);
      CflatAssert(value.mTypeUsage.isPointer());

      const void* ptr = CflatValueAs(&value, void*);
      pOutValue->set(ptr);
   }
   else
   {
      assertValueInitialization(pContext, pExpression->getTypeUsage(), pOutValue);
      evaluateExpression(pContext, pExpression, pOutValue);
   }
}

void Environment::getAddressOfValue(ExecutionContext& pContext, const Value& pInstanceDataValue,
   Value* pOutValue)
{
   TypeUsage pointerTypeUsage = pInstanceDataValue.mTypeUsage;
   pointerTypeUsage.mPointerLevel++;
   pointerTypeUsage.mArraySize = 1u;

   assertValueInitialization(pContext, pointerTypeUsage, pOutValue);
   pOutValue->set(&pInstanceDataValue.mValueBuffer);
}

void Environment::getArgumentValues(ExecutionContext& pContext, const CflatSTLVector(TypeUsage)& pParameters,
   const CflatSTLVector(Expression*)& pExpressions, CflatArgsVector(Value)& pValues)
{
   pValues.resize(pExpressions.size());

   for(size_t i = 0u; i < pExpressions.size(); i++)
   {
      pValues[i].mValueInitializationHint = ValueInitializationHint::Stack;

      if(i < pParameters.size() && pParameters[i].isPointer())
      {
         // Set the pointer level for the value beforehand, to handle the special case where the
         // parameter expects a pointer and the argument is an array name
         pValues[i].mTypeUsage.mPointerLevel = pParameters[i].mPointerLevel;
      }

      evaluateExpression(pContext, pExpressions[i], &pValues[i]);
   }
}

void Environment::prepareArgumentsForFunctionCall(ExecutionContext& pContext,
   const CflatSTLVector(TypeUsage)& pParameters, const CflatArgsVector(Value)& pOriginalValues,
   CflatArgsVector(Value)& pPreparedValues)
{
   pPreparedValues.resize(pOriginalValues.size());

   for(size_t i = 0u; i < pOriginalValues.size(); i++)
   {
      const bool nonVariadicParameter = i < pParameters.size();
      const TypeUsage& valueTypeUsage = nonVariadicParameter
         ? pParameters[i]
         : pOriginalValues[i].mTypeUsage;
      const TypeHelper::Compatibility compatibility =
         TypeHelper::getCompatibility(valueTypeUsage, pOriginalValues[i].mTypeUsage);

      // pass by reference
      if(nonVariadicParameter &&
         pParameters[i].isReference() &&
         compatibility != TypeHelper::Compatibility::ImplicitConstructable)
      {
         pPreparedValues[i] = pOriginalValues[i];
         CflatSetFlag(pPreparedValues[i].mTypeUsage.mFlags, TypeUsageFlags::Reference);
      }
      // pass by value
      else
      {
         pPreparedValues[i].initOnStack(valueTypeUsage, &pContext.mStack);
         assignValue(pContext, pOriginalValues[i], &pPreparedValues[i], false, compatibility);
      }
   }
}

void Environment::applyUnaryOperator(ExecutionContext& pContext, const Value& pOperand,
   const char* pOperator, Value* pOutValue)
{
   Type* type = pOperand.mTypeUsage.mType;

   // overloaded operator
   if(type->mCategory == TypeCategory::StructOrClass && !pOperand.mTypeUsage.isPointer())
   {
      pContext.mStringBuffer.assign("operator");
      pContext.mStringBuffer.append(pOperator);

      CflatArgsVector(Value) argumentValues;

      Struct* castType = static_cast<Struct*>(type);
      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = castType->findMethod(operatorIdentifier);
      Function* operatorFunction = nullptr;
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pOperand, &thisPtrValue);

         operatorMethod->execute(thisPtrValue, argumentValues, pOutValue);
      }
      else
      {
         argumentValues.push_back(pOperand);

         operatorFunction = type->mNamespace->getFunction(operatorIdentifier, argumentValues);

         if(!operatorFunction)
         {
            operatorFunction = findFunction(pContext, operatorIdentifier, argumentValues);

            if(operatorFunction)
            {
               operatorFunction->execute(argumentValues, pOutValue);
            }
         }
      }

      if(operatorMethod || operatorFunction)
      {
         return;
      }
   }

   // address of
   if(pOperator[0] == '&')
   {
      getAddressOfValue(pContext, pOperand, pOutValue);
   }
   // integer built-in / pointer
   else if(type->isInteger() || pOperand.mTypeUsage.isPointer())
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
         else if(strcmp(pOperator, "++") == 0 || strcmp(pOperator, "--") == 0)
         {
            int64_t increment = 1;

            if(pOutValue->mTypeUsage.isPointer())
            {
               TypeUsage indirectionTypeUsage = pOutValue->mTypeUsage;
               indirectionTypeUsage.mPointerLevel--;
               increment = (int64_t)indirectionTypeUsage.getSize();
            }

            if(pOperator[0] == '-')
            {
               increment *= -1;
            }

            setValueAsInteger(valueAsInteger + increment, pOutValue);
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
}

void Environment::applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
   const char* pOperator, Value* pOutValue)
{
   if(!mErrorMessage.empty())
   {
      return;
   }

   Type* leftType = pLeft.mTypeUsage.mType;
   Type* rightType = pRight.mTypeUsage.mType;

   const bool leftIsNumericValue =
      leftType->mCategory == TypeCategory::BuiltIn ||
      leftType->mCategory == TypeCategory::Enum ||
      leftType->mCategory == TypeCategory::EnumClass ||
      pLeft.mTypeUsage.isPointer();
   const bool rightIsNumericValue =
      rightType->mCategory == TypeCategory::BuiltIn ||
      rightType->mCategory == TypeCategory::Enum ||
      rightType->mCategory == TypeCategory::EnumClass ||
      pRight.mTypeUsage.isPointer();

   if(leftIsNumericValue && rightIsNumericValue)
   {
      const bool integerLeftValue = leftType->isInteger() || pLeft.mTypeUsage.isPointer();
      const bool integerRightValue = rightType->isInteger() || pRight.mTypeUsage.isPointer();
      const bool integerValues = integerLeftValue && integerRightValue;

      int64_t leftValueAsInteger = getValueAsInteger(pLeft);
      int64_t rightValueAsInteger = getValueAsInteger(pRight);
      double leftValueAsDecimal = 0.0;
      double rightValueAsDecimal = 0.0;

      if(integerLeftValue)
      { 
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
               TypeUsage indirectionTypeUsage = pLeft.mTypeUsage;
               indirectionTypeUsage.mPointerLevel--;
               rightValueAsInteger *= (int64_t)indirectionTypeUsage.getSize();
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
               TypeUsage indirectionTypeUsage = pLeft.mTypeUsage;
               indirectionTypeUsage.mPointerLevel--;
               rightValueAsInteger *= (int64_t)indirectionTypeUsage.getSize();
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

      CflatArgsVector(Value) argumentValues;
      argumentValues.push_back(pRight);

      const Identifier operatorIdentifier(pContext.mStringBuffer.c_str());
      Method* operatorMethod = leftType->mCategory == TypeCategory::StructOrClass
         ? static_cast<Struct*>(leftType)->findMethod(operatorIdentifier, argumentValues)
         : nullptr;
      
      if(operatorMethod)
      {
         Value thisPtrValue;
         thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
         getAddressOfValue(pContext, pLeft, &thisPtrValue);

         CflatArgsVector(Value) preparedArgumentValues;
         prepareArgumentsForFunctionCall(pContext, operatorMethod->mParameters,
            argumentValues, preparedArgumentValues);

         operatorMethod->execute(thisPtrValue, preparedArgumentValues, pOutValue);

         while(!preparedArgumentValues.empty())
         {
            preparedArgumentValues.pop_back();
         }
      }
      else
      {
         argumentValues.insert(argumentValues.begin(), pLeft);

         Function* operatorFunction =
            leftType->mNamespace->getFunction(operatorIdentifier, argumentValues);

         if(!operatorFunction)
         {
            operatorFunction = findFunction(pContext, operatorIdentifier, argumentValues);
         }

         CflatAssert(operatorFunction);

         CflatArgsVector(Value) preparedArgumentValues;
         prepareArgumentsForFunctionCall(pContext, operatorFunction->mParameters,
            argumentValues, preparedArgumentValues);

         operatorFunction->execute(preparedArgumentValues, pOutValue);

         while(!preparedArgumentValues.empty())
         {
            preparedArgumentValues.pop_back();
         }
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

   if(pTargetTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
   {
      if(sourceTypeUsage.mType->mCategory == TypeCategory::StructOrClass)
      {
         performInheritanceCast(pContext, pValueToCast, pTargetTypeUsage, pOutValue);
      }
   }
   else
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
      else if(sourceTypeUsage.mType->isDecimal())
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

void Environment::performImplicitConstruction(ExecutionContext& pContext, Type* pCtorType,
   const Value& pCtorArg, Value* pObjectValue)
{
   static const Hash kInitializerListIdentifierHash = hash("initializer_list");

   CflatAssert(pCtorType->mCategory == TypeCategory::StructOrClass);
   Struct* ctorType = static_cast<Struct*>(pCtorType);

   CflatArgsVector(Value) ctorArgs;
   ctorArgs.push_back(pCtorArg);
   Method* ctor = ctorType->findConstructor(ctorArgs);
   CflatAssert(ctor);

   Value thisPtrValue;
   thisPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
   getAddressOfValue(pContext, *pObjectValue, &thisPtrValue);

   // Special case: array initializer list
   if(ctor->mParameters[0].mType->mIdentifier.mHash == kInitializerListIdentifierHash &&
      pCtorArg.mTypeUsage.isArray())
   {
      Type* initializerListType = ctor->mParameters[0].mType;
      TypeUsage initializerListTypeUsage;
      initializerListTypeUsage.mType = initializerListType;

      Value initializerListValue;
      initializerListValue.initOnStack(initializerListTypeUsage, &pContext.mStack);

      TypeUsage arrayElementConstPtr;
      arrayElementConstPtr.mType = pCtorArg.mTypeUsage.mType;
      arrayElementConstPtr.mPointerLevel = pCtorArg.mTypeUsage.mPointerLevel + 1u;
      CflatSetFlag(arrayElementConstPtr.mFlags, TypeUsageFlags::Const);

      void* arrayBeginPtr = pCtorArg.mValueBuffer;
      void* arrayEndPtr = (char*)arrayBeginPtr + pCtorArg.mTypeUsage.getSize();

      CflatArgsVector(Value) initializerListCtorArgs;
      initializerListCtorArgs.emplace_back();
      Value& initializerListCtorArgArrayBeginPtr = initializerListCtorArgs.back();
      initializerListCtorArgArrayBeginPtr.initOnStack(arrayElementConstPtr, &pContext.mStack);
      initializerListCtorArgArrayBeginPtr.set(&arrayBeginPtr);
      initializerListCtorArgs.emplace_back();
      Value& initializerListCtorArgArrayEndPtr = initializerListCtorArgs.back();
      initializerListCtorArgArrayEndPtr.initOnStack(arrayElementConstPtr, &pContext.mStack);
      initializerListCtorArgArrayEndPtr.set(&arrayEndPtr);

      Value initializerListPtrValue;
      initializerListPtrValue.mValueInitializationHint = ValueInitializationHint::Stack;
      getAddressOfValue(pContext, initializerListValue, &initializerListPtrValue);

      Method* initializerListCtor =
         static_cast<Struct*>(initializerListType)->findConstructor(initializerListCtorArgs);
      CflatAssert(initializerListCtor);

      Value unusedReturnValue;
      initializerListCtor->execute(initializerListPtrValue, initializerListCtorArgs, &unusedReturnValue);

      ctorArgs[0] = initializerListValue;
      ctor->execute(thisPtrValue, ctorArgs, &unusedReturnValue);
   }
   // General case
   else
   {
      Value unusedReturnValue;
      ctor->execute(thisPtrValue, ctorArgs, &unusedReturnValue);
   }
}

void Environment::assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
   bool pDeclaration)
{
   const TypeHelper::Compatibility compatibility =
      TypeHelper::getCompatibility(pTarget->mTypeUsage, pSource.mTypeUsage);
   assignValue(pContext, pSource, pTarget, pDeclaration, compatibility);
}

void Environment::assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
   bool pDeclaration, TypeHelper::Compatibility pCompatibility)
{
   const TypeUsage typeUsage = pTarget->mTypeUsage;

   if(pCompatibility == TypeHelper::Compatibility::ImplicitCastableInteger)
   {
      performIntegerCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(pCompatibility == TypeHelper::Compatibility::ImplicitCastableIntegerFloat)
   {
      performIntegerFloatCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(pCompatibility == TypeHelper::Compatibility::ImplicitCastableFloat)
   {
      performFloatCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(pCompatibility == TypeHelper::Compatibility::ImplicitCastableInheritance)
   {
      performInheritanceCast(pContext, pSource, typeUsage, pTarget);
   }
   else if(pCompatibility == TypeHelper::Compatibility::ImplicitConstructable)
   {
      performImplicitConstruction(pContext, typeUsage.mType, pSource, pTarget);
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
         Method* operatorMethod = type->findMethod(operatorIdentifier, args);

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

void Environment::getTypeFullName(Type* pType, CflatSTLString* pOutString)
{
   if(pType->mNamespace->getFullIdentifier().mHash != 0u)
   {
      pOutString->append(pType->mNamespace->getFullIdentifier().mName);
      pOutString->append("::");
   }

   pOutString->append(pType->mIdentifier.mName);

   if(pType->mCategory == TypeCategory::StructOrClass)
   {
      Struct* structOrClassType = static_cast<Struct*>(pType);
      
      if(!structOrClassType->mTemplateTypes.empty())
      {
         pOutString->append("<");

         for(size_t i = 0u; i < structOrClassType->mTemplateTypes.size(); i++)
         {
            if(i > 0u)
            {
               pOutString->append(", ");
            }

            getTypeFullName(structOrClassType->mTemplateTypes[i].mType, pOutString);
         }

         pOutString->append(">");
      }
   }
}

bool Environment::containsReturnStatement(Statement* pStatement)
{
   if(pStatement->getType() == StatementType::Return)
   {
      return true;
   }

   if(pStatement->getType() == StatementType::Block)
   {
      StatementBlock* blockStatement = static_cast<StatementBlock*>(pStatement);

      for(int i = (int)blockStatement->mStatements.size() - 1; i >= 0; i--)
      {
         if(containsReturnStatement(blockStatement->mStatements[i]))
         {
            return true;
         }
      }
   }

   return false;
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

bool Environment::tryCallDefaultConstructor(ExecutionContext& pContext, Instance* pInstance, Type* pType, size_t pOffset)
{
   CflatAssert(pType->mCategory == TypeCategory::StructOrClass);
   Struct* type = static_cast<Struct*>(pType);
   Method* defaultCtor = type->getDefaultConstructor();

   if(!defaultCtor)
   {
      return false;
   }

   Value thisPtr;
   thisPtr.mValueInitializationHint = ValueInitializationHint::Stack;
   getAddressOfValue(pContext, pInstance->mValue, &thisPtr);

   if(pOffset > 0)
   {
      const char* offsetThisPtr = CflatValueAs(&thisPtr, char*) + pOffset;
      memcpy(thisPtr.mValueBuffer, &offsetThisPtr, sizeof(char*));
   }

   CflatArgsVector(Value) args;
   defaultCtor->execute(thisPtr, args, nullptr);

   return true;
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

         const bool isLocalStaticVariable = statement->mStatic && pContext.mScopeLevel > 0u;

         Instance* instance;
         bool instanceValueUninitialized = true;

         if(isLocalStaticVariable)
         {
            instance =
               pContext.mNamespaceStack.back()->registerInstance(statement->mTypeUsage, statement->mVariableIdentifier);
            instance->mScopeLevel = pContext.mScopeLevel;

            const uint64_t uniqueID = (uint64_t)statement;
            StaticValuesRegistry::const_iterator it = mLocalStaticValues.find(uniqueID);

            if(it == mLocalStaticValues.end())
            {
               mLocalStaticValues[uniqueID] = Value();
               mLocalStaticValues[uniqueID].initOnHeap(statement->mTypeUsage);
            }
            else
            {
               instanceValueUninitialized = false;
            }

            instance->mValue = mLocalStaticValues[uniqueID];
         }
         else
         {
            instance = registerInstance(pContext, statement->mTypeUsage, statement->mVariableIdentifier);
         }

         if(instanceValueUninitialized)
         {
            const bool isStructOrClassInstance =
               instance->mTypeUsage.mType &&
               instance->mTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
               !instance->mTypeUsage.isPointer() &&
               !instance->mTypeUsage.isReference();

            if(isStructOrClassInstance)
            {
               const bool defaultCtorCalled = tryCallDefaultConstructor(pContext, instance, instance->mTypeUsage.mType);

               if(!defaultCtorCalled)
               {
                  Struct* structOrClassType = static_cast<Struct*>(instance->mTypeUsage.mType);

                  for(size_t i = 0u; i < structOrClassType->mMembers.size(); i++)
                  {
                     Member* member = &structOrClassType->mMembers[i];

                     const bool isMemberStructOrClassInstance =
                         member->mTypeUsage.mType &&
                         member->mTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
                         !member->mTypeUsage.isPointer() &&
                         !member->mTypeUsage.isReference();

                     if(!isMemberStructOrClassInstance)
                     {
                        continue;
                     }

                     tryCallDefaultConstructor(pContext, instance, member->mTypeUsage.mType, member->mOffset);
                  }
               }
            }

            if(statement->mInitialValue)
            {
               // Special case: reference from dereferenced pointer
               if(statement->mTypeUsage.isReference() &&
                  !statement->mTypeUsage.isConst() &&
                  statement->mInitialValue->getType() == ExpressionType::UnaryOperation &&
                  static_cast<ExpressionUnaryOperation*>(statement->mInitialValue)->mOperator[0] == '*')
               {
                  Expression* deferencedExpression =
                     static_cast<ExpressionUnaryOperation*>(statement->mInitialValue)->mExpression;
                  CflatAssert(deferencedExpression->getTypeUsage().isPointer());

                  Value initialValueAddress;
                  initialValueAddress.mTypeUsage = deferencedExpression->getTypeUsage();
                  initialValueAddress.mValueInitializationHint = ValueInitializationHint::Stack;
                  evaluateExpression(pContext, deferencedExpression, &initialValueAddress);

                  CflatAssert(instance->mValue.mValueBufferType == ValueBufferType::External);
                  instance->mValue.mValueBuffer = CflatValueAs(&initialValueAddress, char*);
               }
               // Regular case
               else
               {
                  Value initialValue;
                  initialValue.mTypeUsage = instance->mTypeUsage;
                  initialValue.mValueInitializationHint = ValueInitializationHint::Stack;
                  evaluateExpression(pContext, statement->mInitialValue, &initialValue);

                  const bool initialValueIsArray = initialValue.mTypeUsage.isArray();
                  initialValue.mTypeUsage.mFlags = instance->mTypeUsage.mFlags;

                  if(initialValueIsArray)
                  {
                     CflatSetFlag(initialValue.mTypeUsage.mFlags, TypeUsageFlags::Array);
                  }

                  assignValue(pContext, initialValue, &instance->mValue, !isLocalStaticVariable);
               }
            }
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

               mErrorMessage.clear();

               const bool mustReturnValue = function->mReturnTypeUsage != mTypeUsageVoid;
               
               if(mustReturnValue)
               {
                  if(pOutReturnValue)
                  {
                     assertValueInitialization(pContext, function->mReturnTypeUsage, pOutReturnValue);
                  }

                  pContext.mReturnValues.emplace_back();
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

            if(!mErrorMessage.empty())
            {
               break;
            }

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

            if(!mErrorMessage.empty())
            {
               break;
            }

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

               if(!mErrorMessage.empty())
               {
                  break;
               }

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

         {
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
                  collectionType->findMethod("begin", TypeUsage::kEmptyList());
               Value iteratorValue;
               iteratorValue.initOnStack(collectionBeginMethod->mReturnTypeUsage, &pContext.mStack);
               collectionBeginMethod->execute(collectionThisValue, Value::kEmptyList(), &iteratorValue);

               Method* collectionEndMethod =
                  collectionType->findMethod("end", TypeUsage::kEmptyList());
               Value collectionEndValue;
               collectionEndValue.initOnStack(collectionEndMethod->mReturnTypeUsage, &pContext.mStack);
               collectionEndMethod->execute(collectionThisValue, Value::kEmptyList(), &collectionEndValue);

               Type* iteratorType = collectionBeginMethod->mReturnTypeUsage.mType;

               Value conditionValue;
               conditionValue.initOnStack(mTypeUsageBool, &pContext.mStack);
               applyBinaryOperator(pContext, iteratorValue, collectionEndValue, "!=", &conditionValue);

               while(CflatValueAs(&conditionValue, bool))
               {
                  applyUnaryOperator(pContext, iteratorValue, "*", &elementInstance->mValue);

                  execute(pContext, statement->mLoopStatement);

                  if(!mErrorMessage.empty())
                  {
                     break;
                  }

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
            Method* copyConstructor = nullptr;

            const TypeUsage& functionReturnTypeUsage =
               pContext.mCallStack.back().mFunction->mReturnTypeUsage;

            if(functionReturnTypeUsage.mType &&
               functionReturnTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
               !functionReturnTypeUsage.isPointer() &&
               !functionReturnTypeUsage.isReference())
            {
               Struct* functionReturnType = static_cast<Struct*>(functionReturnTypeUsage.mType);
               copyConstructor = functionReturnType->getCopyConstructor();

               if(copyConstructor)
               {
                  TypeUsage typeUsageReference;
                  typeUsageReference.mType = functionReturnType;
                  typeUsageReference.mFlags |= (uint8_t)TypeUsageFlags::Reference;

                  Value returnValue;
                  evaluateExpression(pContext, statement->mExpression, &returnValue);
                  pContext.mReturnValues.back() = returnValue;

                  TypeUsage thisPtrTypeUsage;
                  thisPtrTypeUsage.mType = functionReturnType;
                  thisPtrTypeUsage.mPointerLevel = 1u;

                  Value thisPtrValue;
                  thisPtrValue.initExternal(thisPtrTypeUsage);
                  thisPtrValue.set(&pContext.mReturnValues.back().mValueBuffer);

                  Value referenceValue;
                  referenceValue.initExternal(typeUsageReference);
                  referenceValue.set(returnValue.mValueBuffer);

                  CflatArgsVector(Value) args;
                  args.push_back(referenceValue);
                  copyConstructor->execute(thisPtrValue, args, nullptr);
               }
            }

            if(!copyConstructor)
            {
               Value returnValue;
               returnValue.mValueInitializationHint = ValueInitializationHint::Stack;
               evaluateExpression(pContext, statement->mExpression, &returnValue);
               assignValue(pContext, returnValue, &pContext.mReturnValues.back(), false);
            }
         }

         pContext.mJumpStatement = JumpStatement::Return;
      }
      break;
   default:
      break;
   }
}

void Environment::assignReturnValueFromFunctionCall(const TypeUsage& pReturnTypeUsage,
   const void* pReturnValue, Value* pOutValue)
{
   bool assigned = false;

   if(pReturnTypeUsage.mType->mCategory == TypeCategory::StructOrClass &&
      !pReturnTypeUsage.isReference() &&
      !pReturnTypeUsage.isPointer())
   {
      Struct* returnType = static_cast<Struct*>(pReturnTypeUsage.mType);
      Method* copyConstructor = returnType->getCopyConstructor();

      if(copyConstructor)
      {
         TypeUsage typeUsageReference;
         typeUsageReference.mType = returnType;
         typeUsageReference.mFlags |= (uint8_t)TypeUsageFlags::Reference;

         TypeUsage thisPtrTypeUsage;
         thisPtrTypeUsage.mType = returnType;
         thisPtrTypeUsage.mPointerLevel = 1u;

         Value thisPtrValue;
         thisPtrValue.initExternal(thisPtrTypeUsage);
         thisPtrValue.set(&pOutValue->mValueBuffer);

         Value referenceValue;
         referenceValue.initExternal(typeUsageReference);
         referenceValue.set(pReturnValue);

         CflatArgsVector(Value) args;
         args.push_back(referenceValue);
         copyConstructor->execute(thisPtrValue, args, nullptr);

         assigned = true;
      }
   }

   if(!assigned)
   {
      pOutValue->set(pReturnValue);
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

void Environment::registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage)
{
   mGlobalNamespace.registerTypeAlias(pIdentifier, pTypeUsage);
}

Type* Environment::getType(const Identifier& pIdentifier) const
{
   return mGlobalNamespace.getType(pIdentifier);
}

Type* Environment::getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes) const
{
   return mGlobalNamespace.getType(pIdentifier, pTemplateTypes);
}

TypeUsage Environment::getTypeUsage(const char* pTypeName, Namespace* pNamespace) const
{
   if(!pTypeName || pTypeName[0] == '\0')
   {
      return TypeUsage();
   }

   ParsingContext parsingContext(const_cast<Namespace*>(&mGlobalNamespace));

   parsingContext.mNamespaceStack.clear();
   parsingContext.mNamespaceStack.push_back(pNamespace ? pNamespace : const_cast<Namespace*>(&mGlobalNamespace));

   parsingContext.mPreprocessedCode.assign(pTypeName);
   parsingContext.mPreprocessedCode.push_back('\n');

   tokenize(parsingContext);

   return parseTypeUsage(parsingContext, 0u);
}

Function* Environment::registerFunction(const Identifier& pIdentifier)
{
   return mGlobalNamespace.registerFunction(pIdentifier);
}

Function* Environment::getFunction(const Identifier& pIdentifier) const
{
   return mGlobalNamespace.getFunction(pIdentifier);
}

Function* Environment::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(TypeUsage)& pParameterTypes) const
{
   return mGlobalNamespace.getFunction(pIdentifier, pParameterTypes);
}

Function* Environment::getFunction(const Identifier& pIdentifier,
   const CflatArgsVector(Value)& pArguments) const
{
   return mGlobalNamespace.getFunction(pIdentifier, pArguments);
}

CflatSTLVector(Function*)* Environment::getFunctions(const Identifier& pIdentifier) const
{
   return mGlobalNamespace.getFunctions(pIdentifier);
}

Instance* Environment::setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
   const Value& pValue)
{
   return mGlobalNamespace.setVariable(pTypeUsage, pIdentifier, pValue);
}

Value* Environment::getVariable(const Identifier& pIdentifier) const
{
   return mGlobalNamespace.getVariable(pIdentifier);
}

Instance* Environment::registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier)
{
   return mGlobalNamespace.registerInstance(pTypeUsage, pIdentifier);
}

Instance* Environment::retrieveInstance(const Identifier& pIdentifier) const
{
   return mGlobalNamespace.retrieveInstance(pIdentifier);
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

   ProgramsRegistry::const_iterator it = mPrograms.find(programIdentifier.mHash);

   if(it != mPrograms.end())
   {
      CflatInvokeDtor(Program, it->second);
      CflatFree(it->second);
   }

   mPrograms[programIdentifier.mHash] = program;

   execute(mExecutionContext, *program);

   return mErrorMessage.empty();
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

void Environment::throwCustomRuntimeError(const char* pErrorMessage)
{
   if(!mErrorMessage.empty())
      return;

   char lineAsString[kSmallLocalStringBufferSize];
   snprintf(lineAsString, sizeof(lineAsString), "%d", mExecutionContext.mCallStack.back().mLine);

   mErrorMessage.assign("[Runtime Error] '");
   mErrorMessage.append(mExecutionContext.mProgram->mIdentifier.mName);
   mErrorMessage.append("' -- Line ");
   mErrorMessage.append(lineAsString);
   mErrorMessage.append(": ");
   mErrorMessage.append(pErrorMessage);
}

void Environment::resetStatics()
{
   // Execute all programs to reinitialize global statics
   for(ProgramsRegistry::const_iterator it = mPrograms.begin(); it != mPrograms.end(); it++)
   {
      execute(mExecutionContext, *it->second);
   }

   // Clear values for local statics
   mLocalStaticValues.clear();
}

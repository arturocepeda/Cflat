
// UE includes - Source Code Navigation for getting module paths
#include "SourceCodeNavigation.h"

namespace AutoRegister
{
// Constants
static const FName kFunctionScriptName("ScriptName");
static const FName kMetaComment("Comment");
static const FName kBlueprintType("BlueprintType");
static const FName kNotBlueprintType("NotBlueprintType");
static const size_t kCharConversionBufferSize = 128u;

// For Aid Header generation
static const FString kSpacing = "   ";
static const FString kNewLineWithIndent1 = "\n   ";
static const FString kNewLineWithIndent2 = "\n      ";
static const FString kHeaderSeparator = "//----------------------------------------------------------------------------//";
static const Cflat::Identifier kEmptyId;


static Cflat::Environment* gEnv = nullptr;


struct RegisteredInfo
{
   Cflat::Struct* mStruct;
   TSet<Cflat::Type*> mDependencies;
   TArray<UFunction*> mFunctions;
   TArray<FProperty*> mProperties;
   FName mHeader;
};

struct RegisteredEnumInfo
{
   Cflat::Type* mEnum;
   FName mHeader;
};

struct PerHeaderTypes
{
   TSet<UEnum*> mEnums;
   TSet<UStruct*> mStructs;
   TSet<UStruct*> mClasses;
   TSet<UStruct*> mIncluded;
   FString mHeaderContent;
};

struct RegisterContext
{
   TSet<FName> mModulesToIgnore;
   TMap<UPackage*, bool> mIgnorePackageCache;
   TMap<UEnum*, RegisteredEnumInfo> mRegisteredEnums;
   TMap<UStruct*, RegisteredInfo> mRegisteredStructs;
   TMap<UStruct*, RegisteredInfo> mRegisteredClasses;
   TMap<Cflat::Type*, UStruct*> mCflatTypeToStruct;
   TMap<Cflat::Type*, UEnum*> mCflatTypeToEnum;
   TMap<Cflat::Type*, FName> mCflatTypeToHeader;
   TMap<FName, PerHeaderTypes> mTypesPerHeader;
   TArray<UStruct*> mRegisteredClassesInOrder;
   TArray<UStruct*> mRegisteredStructsInOrder;
   TSet<FName> mHeaderStructsToIgnore; // For header generation. Some types are manually registered
   TSet<FName> mHeaderClassesToIgnore;// For header generation. Some types are manually registered
   TSet<FName> mHeaderAlreadyIncluded;
   TSet<Cflat::Type*> mForwardDeclartionTypes;
   float mTimeStarted; // For Debugging
};


void Init(Cflat::Environment* pEnv)
{
   gEnv = pEnv;
   // Pre cache source files
   FSourceCodeNavigation::GetSourceFileDatabase();
}

void UObjFuncExecute(UFunction* pFunction, UObject* pObject, const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
{
   const size_t kParamBuffMax = 1024;
   uint8 stack[kParamBuffMax];

   // Add parameteres to Stack
   uint32_t paramIndex = 0u;
   for (FProperty* property = (FProperty*)(pFunction->ChildProperties); property && (property->PropertyFlags&(CPF_Parm)) == CPF_Parm; property = (FProperty*)property->Next)
   {
      if (property->HasAnyPropertyFlags(CPF_ReturnParm))
      {
         continue;
      }

      size_t offset = property->GetOffset_ForUFunction();
      size_t size = property->GetSize();

      check(offset + size < kParamBuffMax);

      if(paramIndex < pArgs.size())
      {
         memcpy(&stack[offset], pArgs[paramIndex].mValueBuffer, size);
      }
      else
      {
         FName metadataKey(FString::Printf(TEXT("CPP_Default_%s"), *property->GetName()));
         if (pFunction->HasMetaData(metadataKey))
         {
            FString defaultValue = pFunction->GetMetaData(metadataKey);
            property->ImportText_Direct(*defaultValue, &stack[offset], nullptr, PPF_None);
         }
         else
         {
            UE_LOG(LogTemp, Error, TEXT("[Cflat] Too many arguments for function:: %s"), *pFunction->GetName());
            return;
         }
      }

      paramIndex++;
   }

   // Call function
   pObject->ProcessEvent(pFunction, stack);

   // Retrieve return/out values
   paramIndex = 0u;
   for (FProperty* property = (FProperty*)(pFunction->ChildProperties); property && (property->PropertyFlags&(CPF_Parm)) == CPF_Parm; property = (FProperty*)property->Next)
   {
      size_t offset = property->GetOffset_ForUFunction();
      size_t size = 0u;
      void* target = nullptr;

      if (!property->HasAnyPropertyFlags(CPF_OutParm))
      {
         paramIndex++;
         continue;
      }

      if (property->HasAnyPropertyFlags(CPF_ReturnParm))
      {
         target = pOutReturnValue->mValueBuffer;
         size = pOutReturnValue->mTypeUsage.getSize();
      }
      else
      {
         check(paramIndex < pArgs.size());
         target = pArgs[paramIndex].mValueBuffer;
         size = pArgs[paramIndex].mTypeUsage.getSize();
      }
      check(offset + size < kParamBuffMax);

      memcpy(target, &stack[offset], size);
      paramIndex++;
   }
}

bool IsCflatIdentifierRegistered(const char* pTypeName)
{
   Cflat::Hash typeNameHash = Cflat::hash(pTypeName);
   const Cflat::Identifier::NamesRegistry* registry = Cflat::Identifier::getNamesRegistry();

   return registry->mRegistry.find(typeNameHash) != registry->mRegistry.end();
}

bool IsCflatIdentifierRegistered(const FString& pTypeName)
{
   char nameBuff[kCharConversionBufferSize];
   if (pTypeName.EndsWith(TEXT("*")))
   {
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize,
                                                *(pTypeName.Mid(0, pTypeName.Len() - 1)));
   }
   else
   {
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pTypeName);
   }

   return IsCflatIdentifierRegistered(nameBuff);
}

bool IsCflatIdentifierRegistered(const FString& pTypeName, const FString& pExtendedType)
{
   const bool typeIsRegistered = IsCflatIdentifierRegistered(pTypeName);
   if (!typeIsRegistered)
   {
      return false;
   }

   if (pExtendedType.IsEmpty())
   {
      return typeIsRegistered;
   }

   if (pExtendedType.StartsWith(TEXT("<")))
   {
      const FRegexPattern pattern(TEXT("<(\\w+)>"));
      FRegexMatcher matcher(pattern, pExtendedType);
      if (matcher.FindNext())
      {
         FString substring = matcher.GetCaptureGroup(1); // Get the first captured group
         return IsCflatIdentifierRegistered(substring);
      }
   }
   else
   {
      return IsCflatIdentifierRegistered(pExtendedType);
   }

  return false;
}

Cflat::Struct* GetCflatStructFromUStruct(UStruct* pStruct)
{
   const TCHAR* prefix = pStruct->GetPrefixCPP();
   FString className = FString::Printf(TEXT("%s%s"), prefix, *pStruct->GetName());

   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *className);

   if (!IsCflatIdentifierRegistered(nameBuff))
   {
     return nullptr;
   }

   Cflat::Type* type = gEnv->getType(nameBuff);
   if (type)
   {
      return static_cast<Cflat::Struct*>(type);
   }
   return nullptr;
}

bool CheckShouldIgnoreModule(RegisterContext& pContext, UPackage* pPackage)
{
   if (pPackage == nullptr)
   {
      return true;
   }

   bool ignoreModule = false;
   bool *cachedIgnore= pContext.mIgnorePackageCache.Find(pPackage);

   if (cachedIgnore)
   {
      ignoreModule = *cachedIgnore;
   }
   else
   {
      FString modulePath;
      FName moduleName = FPackageName::GetShortFName(pPackage->GetFName());
      if (pContext.mModulesToIgnore.Contains(moduleName))
      {
         ignoreModule = true;
      }
      else if(FSourceCodeNavigation::FindModulePath(pPackage, modulePath))
      {
         // Ignore Editor modules
         ignoreModule = moduleName.ToString().EndsWith(TEXT("Editor")) || modulePath.Contains(TEXT("/Editor/"));
      }
      else
      {
         ignoreModule = true;
      }
      pContext.mIgnorePackageCache.Add(pPackage, ignoreModule);
   }

   return ignoreModule;
}

bool CheckShouldRegisterType(RegisterContext& pContext, UStruct* pStruct)
{
   // Already registered
   if (pContext.mRegisteredStructs.Contains(pStruct))
   {
      return false;
   }
   if (pContext.mRegisteredClasses.Contains(pStruct))
   {
      return false;
   }

   if (CheckShouldIgnoreModule(pContext, pStruct->GetPackage()))
   {
      return false;
   }

	if (pStruct->GetBoolMetaData(kBlueprintType))
   {
      return true;
   }

	if (pStruct->GetBoolMetaData(kNotBlueprintType))
   {
		return false;
   }

   for (TFieldIterator<FProperty> propIt(pStruct); propIt; ++propIt)
   {
      if (propIt->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected | 
                                      CPF_NativeAccessSpecifierPrivate | 
                                      CPF_EditorOnly))
      {
         continue;
      }

      if (propIt->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintAssignable | CPF_Edit))
      {
         return true;
      }
   }

   for (TFieldIterator<UFunction> funcIt(pStruct); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;

      if (!function->HasAnyFunctionFlags(FUNC_EditorOnly) && 
         function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent))
      {
         return true;
      }
   }

   return false;
}

bool GetFunctionParameters(UFunction* pFunction, Cflat::TypeUsage& pReturn, CflatSTLVector(Cflat::TypeUsage)& pParams, int& pOutFirstDefaultParamIndex)
{
   pOutFirstDefaultParamIndex = -1;

   for (TFieldIterator<FProperty> propIt(pFunction); propIt && propIt->HasAnyPropertyFlags(CPF_Parm); ++propIt)
   {
      FString extendedType;
      FString cppType = propIt->GetCPPType(&extendedType);

      if (!IsCflatIdentifierRegistered(cppType, extendedType))
      {
        return false;
      }

      if (!extendedType.IsEmpty())
      {
         cppType += extendedType;
      }
      if (!propIt->HasAnyPropertyFlags(CPF_ReturnParm) && propIt->HasAnyPropertyFlags(CPF_OutParm))
      {
         cppType += TEXT("&");
      }

      char nameBuff[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *cppType);
      const TypeUsage type = gEnv->getTypeUsage(nameBuff);

      if (type.mType == nullptr)
      {
         return false;
      }

      if (propIt->HasAnyPropertyFlags(CPF_ReturnParm))
      {
         pReturn = type;
         continue;
      }

      if (pOutFirstDefaultParamIndex == -1)
      {
         FString metaDataName = FString::Printf(TEXT("CPP_Default_%s"), *propIt->GetName());

         if (pFunction->HasMetaData(*metaDataName))
         {
            pOutFirstDefaultParamIndex = pParams.size();
         }
      }

      pParams.push_back(type);
   }

   return true;
}

void RegisterCflatFunction(Cflat::Struct* pCfStruct, UFunction* pFunction, Cflat::Identifier pIdentifier, 
                           const CflatSTLVector(Cflat::TypeUsage)& pParameters, Cflat::TypeUsage pReturnType)
{
   if (pFunction->HasAnyFunctionFlags(FUNC_Static))
   {
      Cflat::Function* staticFunc = pCfStruct->registerStaticMethod(pIdentifier);
      staticFunc->mReturnTypeUsage = pReturnType;
      staticFunc->mParameters = pParameters;

      staticFunc->execute = [pFunction](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         UObject* context = pFunction->GetOuterUClassUnchecked()->ClassDefaultObject;
         UObjFuncExecute(pFunction, context, pArguments, pOutReturnValue);
      };
   }
   else
   {
      pCfStruct->mMethods.push_back(Cflat::Method(pIdentifier));
      Cflat::Method* method = &pCfStruct->mMethods.back();
      method->mReturnTypeUsage = pReturnType;
      method->mParameters = pParameters;

      method->execute = [pFunction] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         UObject* thisObj = CflatValueAs(&pThis, UObject*);
         UObjFuncExecute(pFunction, thisObj, pArguments, pOutReturnValue);
      };
   }
}

void AddDependencyIfNeeded(RegisterContext& pContext, RegisteredInfo* pRegInfo, Cflat::TypeUsage* pType)
{
   FName* header = pContext.mCflatTypeToHeader.Find(pType->mType);
   if (!header)
   {
      return;
   }

   if (*header == pRegInfo->mHeader)
   {
      return;
   }

   if (pType->isPointer() || pType->isReference())
   {
      pContext.mForwardDeclartionTypes.Add(pType->mType);
   }
   else
   {
      pRegInfo->mDependencies.Add(pType->mType);
   }
}

void RegisterUStructFunctions(RegisterContext& pContext, UStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;
   CflatSTLVector(Cflat::TypeUsage) parameters;
   int firstDefaultParamIndex;

   for (TFieldIterator<UFunction> funcIt(pStruct); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;
      parameters.clear();
      Cflat::TypeUsage funcReturn = {};

      // Ignore Editor
      if (function->HasAnyFunctionFlags(FUNC_EditorOnly))
      {
         continue;
      }

      if (!GetFunctionParameters(function, funcReturn, parameters, firstDefaultParamIndex))
      {
         continue;
      }

      pRegInfo->mFunctions.Push(function);

      FString functionName = function->HasMetaData(kFunctionScriptName) ? function->GetMetaData(kFunctionScriptName) : function->GetName();

      char funcName[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(funcName, kCharConversionBufferSize, *functionName);
      const Cflat::Identifier functionIdentifier(funcName);

      AddDependencyIfNeeded(pContext, pRegInfo, &funcReturn);
      for (int i = 0; i < parameters.size(); ++i)
      {
         AddDependencyIfNeeded(pContext, pRegInfo, &parameters[i]);
      }

      RegisterCflatFunction(cfStruct, function, functionIdentifier, parameters, funcReturn);
      if (firstDefaultParamIndex == -1)
      {
         continue;
      }

      CflatSTLVector(Cflat::TypeUsage) parametersForDefault;
      parametersForDefault.reserve(parameters.size());

      for (int i = 0; i < parameters.size() - 1; ++i)
      {
         parametersForDefault.push_back(parameters[i]);
         if (i >= firstDefaultParamIndex - 1)
         {
            RegisterCflatFunction(cfStruct, function, functionIdentifier, parametersForDefault, funcReturn);
         }
      }
   }
}

void RegisterUScriptStructConstructors(UScriptStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;

   const Cflat::Identifier emptyId;
   UScriptStruct::ICppStructOps* structOps = pStruct->GetCppStructOps();

   if(structOps == nullptr)
   {
      return;
   }

   if (structOps->HasNoopConstructor())
   {
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();
      method->execute = [] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
      };
   }
   else if (structOps->HasZeroConstructor())
   {
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();
      method->execute = [structOps] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         memset(pThis.mValueBuffer, 0, structOps->GetSize());
      };
   }
   // Default Constructor
   else
   {
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();
      method->execute = [structOps] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         void* thiz = CflatValueAs(&pThis, void*);
         structOps->Construct(thiz);
      };
   }
}

void RegisterUStructProperties(UStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;
   for (TFieldIterator<FProperty> propIt(pStruct); propIt; ++propIt)
   {
      FProperty* property = *propIt;


      if (propIt->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected | 
                                      CPF_NativeAccessSpecifierPrivate | 
                                      CPF_EditorOnly))
      {
         continue;
      }

      FString extendedType;
      FString cppType = propIt->GetCPPType(&extendedType);

      if (!IsCflatIdentifierRegistered(cppType, extendedType))
      {
         continue;
      }

      if (!extendedType.IsEmpty())
      {
         cppType += extendedType;
      }

      char nameBuff[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *property->GetName());
      const Cflat::Identifier memberIdentifier(nameBuff);
      Cflat::Member member(memberIdentifier);

      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *cppType);

      member.mTypeUsage = gEnv->getTypeUsage(nameBuff);

      // Type not recognized
      if (member.mTypeUsage.mType == nullptr)
      {
         continue;
      }

      member.mOffset = (uint16_t)property->GetOffset_ForInternal();
      cfStruct->mMembers.push_back(member);

      pRegInfo->mProperties.Push(property);

      // Concrete types should be considered dependencies to be included first in the generated header
      if (!member.mTypeUsage.isPointer() && !member.mTypeUsage.isReference())
      {
         pRegInfo->mDependencies.Add(member.mTypeUsage.mType);
      }
   }
}

Cflat::Struct* RegisterUStruct(RegisterContext& pContext, TMap<UStruct*, RegisteredInfo>& pRegisterMap, TArray<UStruct*>& pRegisteredList, UStruct* pStruct)
{
   // Early out if already registered
   {
      RegisteredInfo* regInfo = pRegisterMap.Find(pStruct);
      if (regInfo)
      {
         return regInfo->mStruct;
      }
   }

   Cflat::Struct* cfStruct = nullptr;
   {
      FString structName = FString::Printf(TEXT("%s%s"), pStruct->GetPrefixCPP(), *pStruct->GetName());
      char classTypeName[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(classTypeName, kCharConversionBufferSize, *structName);
      const Cflat::Identifier classTypeIdentifier(classTypeName);
      Cflat::Type* type = gEnv->getType(classTypeIdentifier);
      if (type)
      {
         return static_cast<Cflat::Struct*>(type);
      }
      cfStruct = gEnv->registerType<Cflat::Struct>(classTypeIdentifier);
   }
   cfStruct->mSize = pStruct->GetStructureSize();

   // Register Super Class/Struct
   {
      Cflat::Type* baseCflatType = nullptr;
      UStruct* superStruct = pStruct->GetSuperStruct();

      if (superStruct)
      {
         // Register base class/structure
         baseCflatType = RegisterUStruct(pContext, pRegisterMap, pRegisteredList, superStruct);
      }

      if (baseCflatType)
      {
         cfStruct->mBaseTypes.emplace_back();
         Cflat::BaseType& baseType = cfStruct->mBaseTypes.back();
         baseType.mType = baseCflatType;
         baseType.mOffset = 0u;
      }
   }
   RegisteredInfo& regInfo = pRegisterMap.Add(pStruct, {});
   regInfo.mStruct = cfStruct;
   if (!cfStruct->mBaseTypes.empty())
   {
      Cflat::Type* baseCflatType = cfStruct->mBaseTypes.back().mType;
      regInfo.mDependencies.Add(baseCflatType);
   }
   {
      UPackage* package = pStruct->GetPackage();
      const FString& modulePath = package->GetMetaData()->GetValue(pStruct, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   pContext.mCflatTypeToStruct.Add(cfStruct, pStruct);
   pContext.mCflatTypeToHeader.Add(cfStruct, regInfo.mHeader);
   pRegisteredList.Add(pStruct);

   return cfStruct;
}

void RegisterRegularEnum(RegisterContext& pContext, UEnum* pUEnum)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pUEnum->GetName());

   if (IsCflatIdentifierRegistered(nameBuff))
   {
      return;
   }

   Cflat::Identifier idEnumName(nameBuff);
   if (gEnv->getType(idEnumName))
   {
      return;
   }
   Cflat::Enum* cfEnum = gEnv->registerType<Cflat::Enum>(idEnumName);
   cfEnum->mSize = sizeof(int64);

   Cflat::Namespace* enumNameSpace = gEnv->requestNamespace(idEnumName);

   for (int32 i = 0; i < pUEnum->NumEnums() - 1; ++i)
   {
      int64 value = pUEnum->GetValueByIndex(i);
      FName enumValueName = pUEnum->GetNameByIndex(i);
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *enumValueName.ToString());
      Cflat::Identifier idEnumValueName(nameBuff);

      Cflat::Value enumValue;
      enumValue.mTypeUsage.mType = cfEnum;
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const);
      enumValue.initOnHeap(enumValue.mTypeUsage);
      enumValue.set(&value);

      Cflat::Instance* instance = gEnv->setVariable(enumValue.mTypeUsage, idEnumValueName, enumValue);
      cfEnum->mInstances.push_back(instance);
      enumNameSpace->setVariable(enumValue.mTypeUsage, idEnumValueName, enumValue);
   }

   RegisteredEnumInfo& regInfo = pContext.mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData()->GetValue(pUEnum, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   pContext.mCflatTypeToEnum.Add(cfEnum, pUEnum);
   pContext.mCflatTypeToHeader.Add(cfEnum, regInfo.mHeader);
}

void RegisterEnumClass(RegisterContext& pContext, UEnum* pUEnum)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pUEnum->GetName());

   if (IsCflatIdentifierRegistered(nameBuff))
   {
      return;
   }

   Cflat::Identifier idEnumName(nameBuff);
   if (gEnv->getType(idEnumName))
   {
      return;
   }
   Cflat::EnumClass* cfEnum = gEnv->registerType<Cflat::EnumClass>(idEnumName);
   cfEnum->mSize = sizeof(int64);

   Cflat::Namespace* enumNameSpace = gEnv->requestNamespace(idEnumName);

   for (int32 i = 0; i < pUEnum->NumEnums() - 1; ++i)
   {
      int64 value = pUEnum->GetValueByIndex(i);
      FString enumValueName = pUEnum->GetNameStringByIndex(i);
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *enumValueName);
      Cflat::Identifier idEnumValueName(nameBuff);

      Cflat::Value enumValue;
      enumValue.mTypeUsage.mType = cfEnum;
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const);
      enumValue.initOnHeap(enumValue.mTypeUsage);
      enumValue.set(&value);

      Cflat::Instance* instance = enumNameSpace->setVariable(enumValue.mTypeUsage, idEnumValueName, enumValue);
      cfEnum->mInstances.push_back(instance);
   }

   RegisteredEnumInfo& regInfo = pContext.mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData()->GetValue(pUEnum, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   pContext.mCflatTypeToEnum.Add(cfEnum, pUEnum);
   pContext.mCflatTypeToHeader.Add(cfEnum, regInfo.mHeader);
}

void RegisterEnums(RegisterContext& pContext)
{
   for (TObjectIterator<UEnum> enumIt; enumIt; ++enumIt)
   {
      UEnum* uEnum = *enumIt;

      {
         UObject* outer = uEnum->GetOuter();
         if (outer && CheckShouldIgnoreModule(pContext, outer->GetPackage()))
         {
            continue;
         }
      }

      UEnum::ECppForm enumForm = uEnum->GetCppForm();

      switch(enumForm)
      {
         case UEnum::ECppForm::Regular:
            RegisterRegularEnum(pContext, uEnum);
            break;
         case UEnum::ECppForm::Namespaced:
         case UEnum::ECppForm::EnumClass:
            RegisterEnumClass(pContext, uEnum);
            break;
      }
   }
}

void RegisterStructs(RegisterContext& pContext)
{
   for (TObjectIterator<UScriptStruct> structIt; structIt; ++structIt)
   {
      UScriptStruct* scriptStruct = *structIt;

      // Register Native Structs only
      if ((scriptStruct->StructFlags & STRUCT_Native) == 0u)
      {
         continue;
      }
      UStruct* uStruct = static_cast<UStruct*>(scriptStruct);
      if (!CheckShouldRegisterType(pContext, uStruct))
      {
         continue;
      }

      RegisterUStruct(pContext, pContext.mRegisteredStructs, pContext.mRegisteredStructsInOrder, uStruct);
   }
}

void RegisterClasses(RegisterContext& pContext)
{
   for (TObjectIterator<UClass> classIt; classIt; ++classIt)
   {
      UStruct* uStruct = static_cast<UStruct*>(*classIt);
      if (!CheckShouldRegisterType(pContext, uStruct))
      {
         continue;
      }

      RegisterUStruct(pContext, pContext.mRegisteredClasses, pContext.mRegisteredClassesInOrder, uStruct);
   }
}

void RegisterProperties(RegisterContext& pContext)
{
   for (auto& pair : pContext.mRegisteredStructs)
   {
      RegisterUStructProperties(pair.Key, &pair.Value);
   }
   for (auto& pair : pContext.mRegisteredClasses)
   {
      RegisterUStructProperties(pair.Key, &pair.Value);
   }
}

/* Same as Cflat macros, but using static const ids to speedup regestering thousends types */
#define DefineVoidMethodReturn(type, uStruct, returnType, funcName) \
   { \
      static const Cflat::Identifier kMethodId = #funcName; \
      static const Cflat::TypeUsage kReturnType= gEnv->getTypeUsage(#returnType); \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      pCfStruct->mMethods.push_back(Cflat::Method(kMethodId)); \
      Cflat::Method* method = &pCfStruct->mMethods.back(); \
      method->mReturnTypeUsage = kReturnType; \
      method->execute = [type, methodIndex] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(pOutReturnValue); \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         returnType result = CflatValueAs(&pThis, UStruct*)->funcName(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }


void RegisterObjectBaseFunctions(Cflat::Struct* pCfStruct, UStruct* pStruct)
{
   DefineVoidMethodReturn(pCfStruct, pStruct, UClass*, GetClass);
   DefineVoidMethodReturn(pCfStruct, pStruct, FString, GetName);
   DefineVoidMethodReturn(pCfStruct, pStruct, FName, GetFName);
   DefineVoidMethodReturn(pCfStruct, pStruct, UWorld*, GetWorld);
}

void RegisterFunctions(RegisterContext& pContext)
{
   const Cflat::TypeUsage uClassTypeUsage = gEnv->getTypeUsage("UClass*");
   const Cflat::TypeUsage uScriptStructTypUsage = gEnv->getTypeUsage("UScriptStruct*");
   const Cflat::Identifier staticStructIdentifier = "StaticStruct";
   const Cflat::Identifier staticClassIdentifier = "StaticClass";

   for (auto& pair : pContext.mRegisteredStructs)
   {
      // Register StaticStruct method
      UStruct* uStruct = pair.Key;
      UScriptStruct* uScriptStruct = static_cast<UScriptStruct*>(uStruct);
      Cflat::Struct* cfStruct = pair.Value.mStruct;
      {
         Cflat::Function* function = cfStruct->registerStaticMethod(staticStructIdentifier);
         function->mReturnTypeUsage = uScriptStructTypUsage;
         function->execute = [uStruct](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
         {
            CflatAssert(pOutReturnValue);
            pOutReturnValue->set(&uStruct);
         };
      }
      RegisterUScriptStructConstructors(uScriptStruct, &pair.Value);
      RegisterObjectBaseFunctions(cfStruct, uStruct);
      RegisterUStructFunctions(pContext, pair.Key, &pair.Value);
   }
   for (auto& pair : pContext.mRegisteredClasses)
   {
      // Register StaticClass method
      UStruct* uStruct = pair.Key;
      UStruct* uClass = static_cast<UClass*>(uStruct);
      Cflat::Struct* cfStruct = pair.Value.mStruct;
      // Register StaticClass method
      {
         Cflat::Function* function = cfStruct->registerStaticMethod(staticClassIdentifier);
         function->mReturnTypeUsage = uClassTypeUsage;
         function->execute = [uClass](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
         {
            CflatAssert(pOutReturnValue);
            pOutReturnValue->set(&uClass);
         };
      }
      RegisterObjectBaseFunctions(cfStruct, uStruct);
      RegisterUStructFunctions(pContext, pair.Key, &pair.Value);
   }
}

PerHeaderTypes* GetOrCreateHeaderType(RegisterContext& pContext, UStruct* pStruct, TMap<FName, PerHeaderTypes>& pHeaders)
{
   RegisteredInfo* regInfo = pContext.mRegisteredStructs.Find(pStruct);
   if (regInfo == nullptr)
   {
      regInfo = pContext.mRegisteredClasses.Find(pStruct);
   }

   check(regInfo);

   PerHeaderTypes* types = pHeaders.Find(regInfo->mHeader);
   if (types == nullptr)
   {
      types = &(pHeaders.Add(regInfo->mHeader, {}));
   }

   return types;
}

PerHeaderTypes* GetOrCreateHeaderType(FName pHeader, TMap<FName, PerHeaderTypes>& pHeaders)
{
   PerHeaderTypes* types = pHeaders.Find(pHeader);
   if (types == nullptr)
   {
      types = &(pHeaders.Add(pHeader, {}));
   }

   return types;
}

void MapTypesPerHeaders(RegisterContext& pContext)
{
   for (const auto& pair : pContext.mRegisteredEnums)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pair.Value.mHeader, pContext.mTypesPerHeader);
      types->mEnums.Add(pair.Key);
   }

   for (UStruct* uStruct : pContext.mRegisteredStructsInOrder)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pContext, uStruct, pContext.mTypesPerHeader);
      types->mStructs.Add(uStruct);
   }

   for (UStruct* uStruct : pContext.mRegisteredClassesInOrder)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pContext, uStruct, pContext.mTypesPerHeader);
      types->mClasses.Add(uStruct);
   }
}

void AidHeaderAppendEnum(const UEnum* pUEnum, FString& pOutContent)
{
  FString strEnum = "\n\n";
  UEnum::ECppForm enumForm = pUEnum->GetCppForm();

  if (pUEnum->HasMetaData(TEXT("Comment")))
  {
    strEnum.Append(pUEnum->GetMetaData(TEXT("Comment")));
    strEnum.Append("\n");
  }

  FString declarationBegin = {};
  FString declarationEnd = {};
  FString newLineSpace = kNewLineWithIndent1;

  switch (enumForm)
  {
  case UEnum::ECppForm::Regular:
    declarationBegin = FString::Printf(TEXT("enum %s\n{"), *pUEnum->GetName());
    declarationEnd = "\n};";
    break;
  case UEnum::ECppForm::Namespaced:
    declarationBegin = FString::Printf(
        TEXT("namespace %s\n{%senum Type%s{"),
        *pUEnum->GetName(),
        *kNewLineWithIndent1,
        *kNewLineWithIndent1);
    declarationEnd = kNewLineWithIndent1 + "};\n}";
    newLineSpace = kNewLineWithIndent2;
    break;
  case UEnum::ECppForm::EnumClass:
    declarationBegin =
        FString::Printf(TEXT("enum class %s\n{"), *pUEnum->GetName());
    declarationEnd = "\n};";
    break;
  }

  strEnum.Append(declarationBegin);

  for (int32 i = 0; i < pUEnum->NumEnums() - 1; ++i)
  {
    int64 value = pUEnum->GetValueByIndex(i);
    FString enumValueName = pUEnum->GetNameStringByIndex(i);
    if (i > 0)
    {
      strEnum.Append(",");
    }
    strEnum.Append(newLineSpace);
    if (pUEnum->HasMetaData(TEXT("Bitflags")))
    {
      strEnum.Append(
          FString::Printf(TEXT("%s = 0x%08x"), *enumValueName, value));
    }
    else
    {
      strEnum.Append(FString::Printf(TEXT("%s = %d"), *enumValueName, value));
    }
  }
  strEnum.Append(declarationEnd);
  pOutContent.Append(strEnum);
}

void AidHeaderAppendStruct(RegisterContext& pContext, const UStruct* pUStruct, FString& pOutContent)
{
  const RegisteredInfo* regInfo = pContext.mRegisteredStructs.Find(pUStruct);
  if (regInfo == nullptr)
  {
    return;
  }
  Cflat::Struct* cfStruct = regInfo->mStruct;

  FString strStruct = "\n";

  // Struct declaration
  {
    if (pUStruct->HasMetaData(kMetaComment))
    {
      strStruct.Append(pUStruct->GetMetaData(kMetaComment));
    }
    strStruct.Append("\nstruct ");
    strStruct.Append(cfStruct->mIdentifier.mName);

    // Base types
    if (cfStruct->mBaseTypes.size() > 0)
    {
      strStruct.Append(" :");
      for (size_t i = 0; i < cfStruct->mBaseTypes.size(); ++i)
      {
        strStruct.Append(" public ");
        strStruct.Append(cfStruct->mBaseTypes[i].mType->mIdentifier.mName);
        if (i < cfStruct->mBaseTypes.size() - 1)
        {
          strStruct.Append(",");
        }
      }
    }
  }

  // Body
  strStruct.Append("\n{");
  FString publicPropStr = {};

  // constructor
  for (size_t i = 0u; i < cfStruct->mMethods.size(); i++)
  {
    if (cfStruct->mMethods[i].mIdentifier == kEmptyId)
    {
      Cflat::Method* method = &cfStruct->mMethods[i];
      FString funcStr = kNewLineWithIndent1;
      funcStr.Append(cfStruct->mIdentifier.mName);
      funcStr.Append("(");

      for (size_t j = 0u; j < method->mParameters.size(); j++)
      {
        if (j > 0)
        {
          funcStr.Append(", ");
        }
        Cflat::TypeUsage* paramUsage = &method->mParameters[j];
        funcStr.Append(paramUsage->mType->mIdentifier.mName);
      }
      funcStr.Append(");");
      strStruct.Append(funcStr);
    }
  }
  strStruct.Append(kNewLineWithIndent1 + "static UStruct* StaticStruct();");

  // properties
  for (const FProperty* prop : regInfo->mProperties)
  {
    // Inherited properties should be in their base classes
    if (prop->GetOwnerStruct() != pUStruct)
    {
      continue;
    }

    // Ignore Protected/Private properties
    if (prop->HasAnyPropertyFlags(
            CPF_NativeAccessSpecifierProtected |
            CPF_NativeAccessSpecifierPrivate))
    {
      continue;
    }
    FString propStr = kNewLineWithIndent1;

    if (prop->HasMetaData(kMetaComment))
    {
      propStr.Append(prop->GetMetaData(kMetaComment));
      propStr.Append(kNewLineWithIndent1);
    }
    {
      FString extendedType;
      propStr.Append(prop->GetCPPType(&extendedType));
      if (!extendedType.IsEmpty())
      {
        propStr.Append(extendedType);
      }
    }
    propStr.Append(" ");
    propStr.Append(prop->GetName() + ";");

    publicPropStr.Append(propStr);
  }

  // functions
  FString publicFuncStr = {};

  for (const UFunction* func : regInfo->mFunctions)
  {
    // Inherited functions should be in their base classes
    if (func->GetOwnerStruct() != pUStruct)
    {
      continue;
    }
    if (!func->HasAnyFunctionFlags(FUNC_Public))
    {
      continue;
    }

    FString funcStr = kNewLineWithIndent1;

    if (func->HasAnyFunctionFlags(FUNC_Static))
    {
      funcStr.Append("static ");
    }

    FProperty* returnProperty = func->GetReturnProperty();
    if (returnProperty)
    {
      FString extendedType;
      funcStr.Append(returnProperty->GetCPPType(&extendedType));
      if (!extendedType.IsEmpty())
      {
        funcStr.Append(extendedType);
      }
    }
    else
    {
      funcStr.Append("void");
    }
    funcStr.Append(" ");

    FString functionName = func->HasMetaData(kFunctionScriptName)
                               ? func->GetMetaData(kFunctionScriptName)
                               : func->GetName();
    funcStr.Append(functionName + "(");

    int32 propCount = 0;
    for (TFieldIterator<FProperty> propIt(func);
         propIt && propIt->HasAnyPropertyFlags(CPF_Parm) &&
         !propIt->HasAnyPropertyFlags(CPF_ReturnParm);
         ++propIt, ++propCount)
    {
      if (propCount > 0)
      {
        funcStr.Append(", ");
      }
      FString extendedType;
      funcStr.Append(propIt->GetCPPType(&extendedType));

      if (!extendedType.IsEmpty())
      {
        funcStr.Append(extendedType);
      }
      if (propIt->HasAnyPropertyFlags(CPF_OutParm))
      {
        funcStr.Append("&");
      }
      funcStr.Append(" ");
      funcStr.Append(propIt->GetName());
    }
    funcStr.Append(");");

    publicFuncStr.Append(funcStr);
  }

  if (!publicPropStr.IsEmpty())
  {
    strStruct.Append("\n");
    strStruct.Append(publicPropStr);
  }
  strStruct.Append(publicFuncStr);
  strStruct.Append("\n};");

  pOutContent.Append(strStruct);
}

void AidHeaderAppendClass(RegisterContext& pContext, const UStruct* pUStruct, FString& pOutContent)
{
  const RegisteredInfo* regInfo = pContext.mRegisteredClasses.Find(pUStruct);
  if (regInfo == nullptr)
  {
    return;
  }
  const UClass* uClass = static_cast<const UClass*>(pUStruct);
  Cflat::Struct* cfStruct = regInfo->mStruct;

  FString strClass = "\n";

  // Class declaration
  {
    if (uClass->HasMetaData(kMetaComment))
    {
      strClass.Append(uClass->GetMetaData(kMetaComment));
    }
    strClass.Append("\nclass ");
    strClass.Append(cfStruct->mIdentifier.mName);

    // Base types
    if (cfStruct->mBaseTypes.size() > 0)
    {
      strClass.Append(" :");
      for (size_t i = 0; i < cfStruct->mBaseTypes.size(); ++i)
      {
        strClass.Append(" public ");
        strClass.Append(cfStruct->mBaseTypes[i].mType->mIdentifier.mName);
        if (i < cfStruct->mBaseTypes.size() - 1)
        {
          strClass.Append(",");
        }
      }
    }
  }

  // Body
  strClass.Append("\n{");
  FString publicPropStr = "";

  // properties
  for (const FProperty* prop : regInfo->mProperties)
  {
    // Inherited properties should be in their base classes
    if (prop->GetOwnerClass() != uClass)
    {
      continue;
    }

    // Ignore non public properties
    if (!prop->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
    {
      continue;
    }
    FString propStr = kNewLineWithIndent1;

    if (prop->HasMetaData(kMetaComment))
    {
      propStr.Append(prop->GetMetaData(kMetaComment));
      propStr.Append(kNewLineWithIndent1);
    }
    {
      FString extendedType;
      propStr.Append(prop->GetCPPType(&extendedType));
      if (!extendedType.IsEmpty())
      {
        propStr.Append(extendedType);
      }
    }
    propStr.Append(" ");
    propStr.Append(prop->GetName() + ";");

    publicPropStr.Append(propStr);
  }

  // functions
  FString publicFuncStr = kNewLineWithIndent1 + "static UClass* StaticClass();";

  for (const UFunction* func : regInfo->mFunctions)
  {
    // Inherited functions should be in their base classes
    if (func->GetOwnerClass() != uClass)
    {
      continue;
    }
    if (!func->HasAnyFunctionFlags(FUNC_Public))
    {
      continue;
    }

    FString funcStr = kNewLineWithIndent1;

    if (func->HasAnyFunctionFlags(FUNC_Static))
    {
      funcStr.Append("static ");
    }

    FProperty* returnProperty = func->GetReturnProperty();
    if (returnProperty)
    {
      FString extendedType;
      funcStr.Append(returnProperty->GetCPPType(&extendedType));
      if (!extendedType.IsEmpty())
      {
        funcStr.Append(extendedType);
      }
    }
    else
    {
      funcStr.Append("void");
    }
    funcStr.Append(" ");

    FString functionName = func->HasMetaData(kFunctionScriptName)
                               ? func->GetMetaData(kFunctionScriptName)
                               : func->GetName();
    funcStr.Append(functionName + "(");

    int32 propCount = 0;
    for (TFieldIterator<FProperty> propIt(func);
         propIt && propIt->HasAnyPropertyFlags(CPF_Parm) &&
         !propIt->HasAnyPropertyFlags(CPF_ReturnParm);
         ++propIt, ++propCount)
    {
      if (propCount > 0)
      {
        funcStr.Append(", ");
      }
      FString extendedType;
      funcStr.Append(propIt->GetCPPType(&extendedType));

      if (!extendedType.IsEmpty())
      {
        funcStr.Append(extendedType);
      }
      if (propIt->HasAnyPropertyFlags(CPF_OutParm))
      {
        funcStr.Append("&");
      }
      funcStr.Append(" ");
      funcStr.Append(propIt->GetName());
    }
    funcStr.Append(");");

    publicFuncStr.Append(funcStr);
  }

  strClass.Append("\npublic:");
  if (!publicPropStr.IsEmpty())
  {
    strClass.Append(publicPropStr);
    strClass.Append("\n");
  }
  strClass.Append(publicFuncStr);
  strClass.Append("\n};");

  pOutContent.Append(strClass);
}

void AppendStructWithDependenciesRecursively(RegisterContext& pContext, FName pHeader, PerHeaderTypes& pTypes, UStruct* pStruct, bool pIsClass)
{
   if (pTypes.mIncluded.Find(pStruct))
   {
      return;
   }

   RegisteredInfo* regInfo = pIsClass ? pContext.mRegisteredClasses.Find(pStruct)
                                      : pContext.mRegisteredStructs.Find(pStruct);
   if (!regInfo)
   {
      return;
   }

   pTypes.mIncluded.Add(pStruct);

   for (Cflat::Type* cfType : regInfo->mDependencies)
   {
      UStruct** depUStruct = pContext.mCflatTypeToStruct.Find(cfType);
      if (!depUStruct)
      {
         continue;
      }

      RegisteredInfo* depRegInfo = pContext.mRegisteredStructs.Find(*depUStruct);
      bool isClass = false;
      if (!depRegInfo)
      {
         depRegInfo = pContext.mRegisteredClasses.Find(*depUStruct);
         isClass = true;
      }

      if (!depRegInfo)
      {
         continue;
      }

      if (depRegInfo->mHeader != pHeader)
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pContext, pHeader, pTypes, *depUStruct, isClass);
   }

   if (pIsClass)
   {
      AidHeaderAppendClass(pContext, pStruct, pTypes.mHeaderContent);
   }
   else
   {
      AidHeaderAppendStruct(pContext, pStruct, pTypes.mHeaderContent);
   }
}

void CreateHeaderContent(RegisterContext& pContext, FName pHeader, TArray<FName>& pHeaderIncludeOrder)
{
   if (pContext.mHeaderAlreadyIncluded.Find(pHeader))
   {
      return;
   }

   PerHeaderTypes* types = pContext.mTypesPerHeader.Find(pHeader);
   if (!types)
   {
      return;
   }

   pContext.mHeaderAlreadyIncluded.Add(pHeader);

   // First we check for header dependency
   for (const UStruct* uStruct : types->mStructs)
   {
      if (pContext.mHeaderStructsToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      RegisteredInfo* regInfo = pContext.mRegisteredStructs.Find(uStruct);
      if (!regInfo)
      {
         continue;
      }

      for (Cflat::Type* cfType : regInfo->mDependencies)
      {
         FName* header = pContext.mCflatTypeToHeader.Find(cfType);

         if (!header || *header == pHeader)
         {
            continue;
         }

         if (!pContext.mHeaderAlreadyIncluded.Find(*header))
         {
            CreateHeaderContent(pContext, *header, pHeaderIncludeOrder);
         }
      }
   }

   for (const UStruct* uStruct : types->mClasses)
   {
      if (pContext.mHeaderClassesToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      RegisteredInfo* regInfo = pContext.mRegisteredClasses.Find(uStruct);
      if (!regInfo)
      {
         continue;
      }

      for (Cflat::Type* cfType : regInfo->mDependencies)
      {
         FName* header = pContext.mCflatTypeToHeader.Find(cfType);

         if (!header || *header == pHeader)
         {
            continue;
         }

         if (!pContext.mHeaderAlreadyIncluded.Find(*header))
         {
            CreateHeaderContent(pContext, *header, pHeaderIncludeOrder);
         }
      }
   }

   pHeaderIncludeOrder.Add(pHeader);

   // Generate the header strings
   types->mHeaderContent = FString::Printf(TEXT("\n\n%s\n// %s\n%s"), *kHeaderSeparator, *pHeader.ToString(), *kHeaderSeparator);

   // Enums
   for (const UEnum* uEnum : types->mEnums)
   {
      AidHeaderAppendEnum(uEnum, types->mHeaderContent);
   }

   for (UStruct* uStruct : types->mStructs)
   {
      if (pContext.mHeaderStructsToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pContext, pHeader, *types, uStruct, false);
   }

   for (UStruct* uStruct : types->mClasses)
   {
      if (pContext.mHeaderClassesToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pContext, pHeader, *types, uStruct, true);
   }

}

void GenerateAidHeader(RegisterContext& pContext, const FString& pFilePath, const TArray<FString>& pAidIncludes)
{
   FString content = "// Auto Generated From Auto Registered UClasses";
   content.Append("\n#pragma once");
   content.Append("\n#if defined (CFLAT_ENABLED)");
   for (int32 i = 0; i < pAidIncludes.Num(); ++i)
   {
      content.Append(FString::Printf(TEXT("\n#include \"%s\""), *pAidIncludes[i]));
   }

   FString includeContent = "// Auto Generated From Auto Registered UClasses";
   includeContent.Append("\n#pragma once");
   includeContent.Append("\n#if !defined (CFLAT_ENABLED)");

   MapTypesPerHeaders(pContext);

   TArray<FName> headerIncludeOrder;
   headerIncludeOrder.Reserve(pContext.mTypesPerHeader.Num());

   for (auto& typesPair : pContext.mTypesPerHeader)
   {
      CreateHeaderContent(pContext, typesPair.Key, headerIncludeOrder);
   }

   // Forward declartions
   {
      FString fwdStructs = "\n\n// Forward Structs Declaration";
      FString fwdClasses = "\n\n// Forward Classes Declaration";

      for (Cflat::Type* fwdType : pContext.mForwardDeclartionTypes)
      {
         UStruct** uStruct = pContext.mCflatTypeToStruct.Find(fwdType);
         if (!uStruct)
         {
            continue;
         }

         RegisteredInfo* regInfo = pContext.mRegisteredStructs.Find(*uStruct);
         if (pContext.mRegisteredStructs.Find(*uStruct))
         {
            fwdStructs.Append("\nstruct ");
            fwdStructs.Append(fwdType->mIdentifier.mName);
            fwdStructs.Append(";");
         }
         else if (pContext.mRegisteredClasses.Find(*uStruct))
         {
            fwdClasses.Append("\nclass ");
            fwdClasses.Append(fwdType->mIdentifier.mName);
            fwdClasses.Append(";");
         }
      }

      content.Append(fwdStructs);
      content.Append(fwdClasses);
      content.Append("\n");
   }

   for (const FName& headerName : headerIncludeOrder)
   {
      PerHeaderTypes& types = pContext.mTypesPerHeader[headerName];
      content.Append(types.mHeaderContent);

      FString headerPath = headerName.ToString();
      if (!headerPath.IsEmpty() && !headerPath.StartsWith(TEXT("Private/")))
      {
         headerPath.RemoveFromStart("Public/");
         headerPath.RemoveFromStart("Classes/");
         includeContent.Append(FString::Printf(TEXT("\n#include \"%s\""), *headerPath));
      }
   }

   content.Append("\n\n#endif // CFLAT_ENABLED");
   includeContent.Append("\n\n#endif // CFLAT_ENABLED");

   FString aidFilePath = pFilePath + "/_aid.gen.h";
   if(!FFileHelper::SaveStringToFile(content, *aidFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
   {
      UE_LOG(LogTemp, Error, TEXT("[Cflat] Could not write Aid Header File: %s"), *aidFilePath);
   }
   FString includeFilePath = pFilePath + "/_includes.gen.h";
   if(!FFileHelper::SaveStringToFile(includeContent, *includeFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
   {
      UE_LOG(LogTemp, Error, TEXT("[Cflat] Could not write Include Header File: %s"), *includeFilePath);
   }
}

void AppendClassAndFunctionsForDebugging(UStruct* pStruct, FString& pOutString)
{
   Cflat::Struct* cfStruct = GetCflatStructFromUStruct(pStruct);
   if (cfStruct == nullptr)
   {
      pOutString.Append(FString::Printf(TEXT("%s\n\tNOT FOUND"), *pStruct->GetFullName()));
      return;
   }

   FString strMembers;
   for (size_t i = 0; i < cfStruct->mMembers.size(); ++i)
   {
      FString strFunc;
      const Cflat::Member& member = cfStruct->mMembers[i];
      strMembers.Append("\n\t");
      strMembers.Append(member.mIdentifier.mName);
   }

   FString strFunctions;
   {
      CflatSTLVector(Function*) functions;
      cfStruct->mFunctionsHolder.getAllFunctions(&functions);
      for (size_t i = 0; i < functions.size(); ++i)
      {
         strFunctions.Append("\n\t");
         strFunctions.Append(functions[i]->mIdentifier.mName);
      }
   }

   pOutString.Append("\n\n");
   pOutString.Append(pStruct->GetFullName());
   pOutString.Append("\n");
   pOutString.Append("Properties:");
   pOutString.Append(strMembers);
   pOutString.Append("\n");
   pOutString.Append("Functions:");
   pOutString.Append(strFunctions);
}

void PrintDebugStats(RegisterContext& pContext)
{
   UE_LOG(LogTemp, Log, TEXT("[Cflat] AutoRegisterCflatTypes: total: %d time: %f"), 
          pContext.mRegisteredStructs.Num() + pContext.mRegisteredClasses.Num(),
          FPlatformTime::Seconds() - pContext.mTimeStarted);
   {
     const Cflat::Identifier::NamesRegistry* registry =
         Cflat::Identifier::getNamesRegistry();
      const char* buffBegin = (const char*)registry->mPointer;
      const char* buffEnd = (const char*)(&registry->mMemory);
      int sizeDiff = buffBegin - buffEnd;
      int count = registry->mRegistry.size();
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat] StringRegistry count: %d usage: %d of %d\n\n"), count, sizeDiff, Cflat::kIdentifierStringsPoolSize);
   }

   {
      FString addedStructs = {};
      for (UStruct* uStruct : pContext.mRegisteredStructsInOrder)
      {
         AutoRegister::AppendClassAndFunctionsForDebugging(uStruct, addedStructs);
      }
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat][Added UStructs]\n\n%s\n\n\n"), *addedStructs);

      FString addedClasses = {};
      for (const auto& pair : pContext.mRegisteredClasses)
      {
         AutoRegister::AppendClassAndFunctionsForDebugging(pair.Key, addedClasses);
      }
      UE_LOG(LogTemp, Log, TEXT("%s"), *addedClasses);
   }

   {
      TMap<FName, int32> moduleCount;
      for (const auto& pair : pContext.mRegisteredClasses)
      {
         UPackage* classPackage = pair.Key->GetPackage();
         FName moduleName = FPackageName::GetShortFName(classPackage->GetFName());
         int32* count = moduleCount.Find(moduleName);
         if (count)
         {
            (*count)++;
         }
         else
         {
           moduleCount.Add(moduleName, 1);
         }
      }
      FString modulesCountStr = TEXT("\n\nModule count:\n\n");
      int32 total = 0;
      for (auto& it : moduleCount)
      {
         modulesCountStr.Append(FString::Printf(TEXT("%s,%d\n"), *it.Key.ToString(), it.Value));
         total += it.Value;
      }
      UE_LOG(LogTemp, Log, TEXT("%s\n\nTotal: %d"), *modulesCountStr, total);
   }
}

} // namespace AutoRegister

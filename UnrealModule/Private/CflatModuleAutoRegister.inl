
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
static const FString kStringEmpty = "";

// For Aid Header generation
static const FString kSpacing = "   ";
static const FString kNewLineWithIndent1 = "\n   ";
static const FString kNewLineWithIndent2 = "\n      ";
static const FString kHeaderSeparator = "//----------------------------------------------------------------------------//";
static const Cflat::Identifier kEmptyId;



void UObjFuncExecute(UFunction* pFunction, UObject* pObject, const CflatArgsVector(Cflat::Value)& pArgs, 
                     Cflat::Value* pOutReturnValue, const Cflat::TypeUsage& pReturnType)
{
   const size_t kParamBuffMax = 1024;
   uint8 stack[kParamBuffMax] = {0};

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

   uint8* returnAddress = nullptr;
   if (pFunction->ReturnValueOffset != MAX_uint16 && pOutReturnValue)
   {
      check(pFunction->ReturnValueOffset < kParamBuffMax);
      returnAddress = &stack[pFunction->ReturnValueOffset];
   }

   // Call function
   pObject->ProcessEvent(pFunction, stack);

   // Retrieve return/out values
   paramIndex = 0u;
   for (FProperty* property = (FProperty*)(pFunction->ChildProperties); property && (property->PropertyFlags&(CPF_Parm)) == CPF_Parm; property = (FProperty*)property->Next)
   {

      if (property->HasAnyPropertyFlags(CPF_ReturnParm))
      {
         check(returnAddress);
         Cflat::Environment::assignReturnValueFromFunctionCall(pReturnType, returnAddress, pOutReturnValue);
      }
      else if (property->HasAnyPropertyFlags(CPF_OutParm))
      {
         size_t offset = property->GetOffset_ForUFunction();
         check(paramIndex < pArgs.size());

         void* target = pArgs[paramIndex].mValueBuffer;
         size_t size = pArgs[paramIndex].mTypeUsage.getSize();

         check(offset + size < kParamBuffMax);

         memcpy(target, &stack[offset], size);
      }

      paramIndex++;
   }
}

struct RegisteredFunctionInfo
{
   UFunction* mFunction;
   Cflat::Identifier mIdentifier;
   Cflat::TypeUsage mReturnType;
   FString mName;
   int mFirstDefaultParamIndex;
   int mRegisteredIndex;
   CflatSTLVector(Cflat::TypeUsage) mParameters;
};

struct RegisteredInfo
{
   Cflat::Struct* mStruct;
   Cflat::Identifier mIdentifier;
   TSet<Cflat::Type*> mDependencies;
   TArray<RegisteredFunctionInfo> mFunctions;
   TArray<FProperty*> mProperties;
   TSet<Cflat::Function*> mStaticFunctions;
   int mMembersCount;
   int mMethodCount;
   int mFunctionCount;
   FName mHeader;
   bool mIsClass;
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
   UPackage* mPackage;
};


class TypesRegister
{
public:
   TSet<FName> mAllowedModules;
   TMap<UPackage*, bool> mIgnorePackageCache;
   TMap<UPackage*, FString> mPackagePaths;
   TMap<UEnum*, RegisteredEnumInfo> mRegisteredEnums;
   TMap<UStruct*, RegisteredInfo> mRegisteredStructs;
   TMap<UStruct*, RegisteredInfo> mRegisteredClasses;
   TMap<UStruct*, RegisteredInfo> mRegisteredInterfaces;
   TMap<Cflat::Type*, UStruct*> mCflatTypeToStruct;
   TMap<Cflat::Type*, UEnum*> mCflatTypeToEnum;
   TMap<Cflat::Type*, FName> mCflatTypeToHeader;
   TMap<FName, PerHeaderTypes> mTypesPerHeader;
   TSet<FName> mHeaderEnumsToIgnore;
   TSet<FName> mHeaderStructsToIgnore;
   TSet<FName> mHeaderClassesToIgnore;
   TMap<FName, TArray<FString>> mModuleHeaderPathsToIgnore;
   TSet<FName> mHeaderAlreadyIncluded;
   TSet<FName> mIgnoredTypes;
   TSet<Cflat::Type*> mForwardDeclartionTypes;
   double mTimeStarted; // For Debugging

   Cflat::Environment* mEnv = nullptr;
   Cflat::TypeUsage mUObjectTypeUsage;

TypesRegister(Cflat::Environment* pEnv) : mEnv(pEnv)
{
   mTimeStarted = FPlatformTime::Seconds();
   // Pre cache source files
   FSourceCodeNavigation::GetSourceFileDatabase();
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
   int32 templateIndexBegin = 0;
   int32 templateIndexEnd = 0;

   bool typeIsRegistered = false;

   if (pTypeName.FindChar(TCHAR('<'), templateIndexBegin) &&
       pTypeName.FindLastChar(TCHAR('>'), templateIndexEnd) && templateIndexBegin < templateIndexEnd)
   {
      if (templateIndexBegin == 0)
      {
         FString typeBase = pTypeName.Mid(templateIndexBegin + 1, templateIndexEnd - 1);
         typeIsRegistered = IsCflatIdentifierRegistered(typeBase, kStringEmpty);
      }
      else
      {
         FString typeBase = pTypeName.Mid(0, templateIndexBegin);
         FString typeTemplate = pTypeName.Mid(templateIndexBegin, templateIndexEnd);
         typeIsRegistered = IsCflatIdentifierRegistered(typeBase, typeTemplate);
      }
   }
   else
   {
      typeIsRegistered = IsCflatIdentifierRegistered(pTypeName);
   }

   if (!typeIsRegistered)
   {
      return false;
   }

   if (pExtendedType.IsEmpty())
   {
      return typeIsRegistered;
   }

   return IsCflatIdentifierRegistered(pExtendedType, kStringEmpty);
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

   Cflat::Type* type = mEnv->getType(nameBuff);
   if (type)
   {
      return static_cast<Cflat::Struct*>(type);
   }
   return nullptr;
}

RegisteredInfo* FindRegisteredInfoFromUStruct(UStruct* pStruct)
{
   RegisteredInfo* depRegInfo = mRegisteredStructs.Find(pStruct);
   if (depRegInfo)
   {
      return depRegInfo;
   }
   depRegInfo = mRegisteredClasses.Find(pStruct);
   if (depRegInfo)
   {
      return depRegInfo;
   }
   return nullptr;
}

bool CheckShouldIgnoreModule(UPackage* pPackage)
{
   if (pPackage == nullptr)
   {
      return true;
   }

   bool ignoreModule = false;
   bool *cachedIgnore= mIgnorePackageCache.Find(pPackage);

   if (cachedIgnore)
   {
      ignoreModule = *cachedIgnore;
   }
   else
   {
      FString modulePath;
      FName moduleName = FPackageName::GetShortFName(pPackage->GetFName());
      if (!mAllowedModules.Contains(moduleName))
      {
         ignoreModule = true;
      }
      else if(FSourceCodeNavigation::FindModulePath(pPackage, modulePath))
      {
         // Ignore Editor and Develper modules
         ignoreModule = moduleName.ToString().EndsWith(TEXT("Editor")) || modulePath.Contains(TEXT("/Editor/")) ||
                        modulePath.Contains(TEXT("/Developer/"));
      }
      else
      {
         ignoreModule = true;
      }
      mIgnorePackageCache.Add(pPackage, ignoreModule);
      if (!ignoreModule)
      {
         mPackagePaths.Add(pPackage, modulePath);
      }
   }

   return ignoreModule;
}

bool CheckShouldIgnoreHeaderPath(UStruct* pStruct)
{
   UPackage* package = pStruct->GetPackage();

   if (package == nullptr)
   {
      return true;
   }

   FName moduleName = FPackageName::GetShortFName(package->GetFName());

   TArray<FString>* pathsToIgnore = mModuleHeaderPathsToIgnore.Find(moduleName);
   if (pathsToIgnore == nullptr)
   {
      return false;
   }

   const FString& modulePath = package->GetMetaData().GetValue(pStruct, TEXT("ModuleRelativePath"));
   if (modulePath.IsEmpty())
   {
      return false;
   }

   for (int i = 0; i < pathsToIgnore->Num(); ++i)
   {
      if (modulePath.Contains((*pathsToIgnore)[i]))
      {
         return true;
      }
   }

   return false;
}

bool CheckShouldRegisterType(UStruct* pStruct)
{
   if (mIgnoredTypes.Find(pStruct->GetFName()))
   {
      return false;
   }

   // Already registered
   if (mRegisteredStructs.Contains(pStruct))
   {
      return false;
   }
   if (mRegisteredClasses.Contains(pStruct))
   {
      return false;
   }

   if (CheckShouldIgnoreModule(pStruct->GetPackage()))
   {
      return false;
   }

   if (CheckShouldIgnoreHeaderPath(pStruct))
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

   int propertyCount = 0;
   for (TFieldIterator<FProperty> propIt(pStruct, EFieldIterationFlags::None); propIt; ++propIt)
   {
      propertyCount++;
      if (propIt->HasAnyPropertyFlags(CPF_EditorOnly))
      {
         continue;
      }

      if (propIt->HasAnyPropertyFlags(CPF_BlueprintVisible |
                                      CPF_BlueprintReadOnly |
                                      CPF_BlueprintAssignable |
                                      CPF_Edit))
      {
         return true;
      }
   }

   int functionCount = 0;
   for (TFieldIterator<UFunction> funcIt(pStruct, EFieldIterationFlags::None); funcIt; ++funcIt)
   {
      functionCount++;
      UFunction* function = *funcIt;

      if (!function->HasAnyFunctionFlags(FUNC_EditorOnly))
      {
         return true;
      }
   }

   // Nothing in the type to be registered, but it can still be used as a handle
   if (propertyCount == 0 && functionCount == 0)
   {
      return true;
   }

   return false;
}

bool CheckShouldRegisterInterface(UClass* pInterface)
{
   if (mIgnoredTypes.Find(pInterface->GetFName()))
   {
      return false;
   }

   // Already registered
   if (mRegisteredInterfaces.Contains(pInterface))
   {
      return false;
   }

   if (CheckShouldIgnoreModule(pInterface->GetPackage()))
   {
      return false;
   }

   if (pInterface->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
   {
      return false;
   }

   return true;
}

bool GetFunctionParameters(UFunction* pFunction, Cflat::TypeUsage& pReturn, CflatSTLVector(Cflat::TypeUsage)& pParams, int& pOutFirstDefaultParamIndex)
{
   pOutFirstDefaultParamIndex = -1;

   char nameBuff[kCharConversionBufferSize];

   for (TFieldIterator<FProperty> propIt(pFunction); propIt && propIt->HasAnyPropertyFlags(CPF_Parm); ++propIt)
   {
      FString extendedType;
      FString cppType;

      if (propIt->IsA<FByteProperty>())
      {
         FByteProperty* byteProp = static_cast<FByteProperty*>(*propIt);
         if (byteProp->Enum)
         {
            UEnum* uEnum = byteProp->Enum;
            if (mRegisteredEnums.Find(uEnum) == nullptr)
            {
               return false;
            }
            cppType = uEnum->CppType;
         }
      }

      if (cppType.IsEmpty())
      {
         cppType = propIt->GetCPPType(&extendedType);
         if (!IsCflatIdentifierRegistered(cppType, extendedType))
         {
            return false;
         }
      }

      if (!extendedType.IsEmpty())
      {
         cppType += extendedType;
      }
      if (propIt->HasAnyPropertyFlags(CPF_ConstParm))
      {
         cppType = "const " + cppType;
      }

      if (propIt->HasAnyPropertyFlags(CPF_ReferenceParm) || propIt->HasAnyPropertyFlags(CPF_OutParm))
      {
         // Treat return refs as copies
         if (!propIt->HasAnyPropertyFlags(CPF_ReturnParm))
         {
            cppType += TEXT("&");
         }
      }

      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *cppType);
      const TypeUsage type = mEnv->getTypeUsage(nameBuff);

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

      staticFunc->execute = [pFunction, pReturnType](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         UObject* context = pFunction->GetOuterUClassUnchecked()->ClassDefaultObject;
         UObjFuncExecute(pFunction, context, pArguments, pOutReturnValue, pReturnType);
      };
   }
   else
   {
      pCfStruct->mMethods.push_back(Cflat::Method(pIdentifier));
      Cflat::Method* method = &pCfStruct->mMethods.back();
      method->mReturnTypeUsage = pReturnType;
      method->mParameters = pParameters;
      if (pFunction->HasAnyFunctionFlags(FUNC_Const))
      {
         CflatSetFlag(method->mFlags, Cflat::MethodFlags::Const);
      }

      method->execute = [pFunction, pReturnType] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         UObject* thisObj = CflatValueAs(&pThis, UObject*);
         UObjFuncExecute(pFunction, thisObj, pArguments, pOutReturnValue, pReturnType);
      };
   }
}

void RegisterInterfaceFunction(Cflat::Struct* pCfStruct, UFunction* pFunction, Cflat::Identifier pIdentifier, 
                           const CflatSTLVector(Cflat::TypeUsage)& pParameters, Cflat::TypeUsage pReturnType)
{
   char nameBuff[kCharConversionBufferSize];
   snprintf(nameBuff, kCharConversionBufferSize - 1, "Execute_%s", pIdentifier.mName);
   CflatSTLVector(Cflat::TypeUsage) parameters;
   parameters.reserve(pParameters.size() + 1);
   parameters.push_back(mUObjectTypeUsage);
   parameters.insert(parameters.end(), pParameters.begin(), pParameters.end());
   Cflat::Function* staticFunc = pCfStruct->registerStaticMethod(nameBuff);
   staticFunc->mReturnTypeUsage = pReturnType;
   staticFunc->mParameters = parameters;

   staticFunc->execute = [pFunction, pReturnType](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) {
      check(pArguments.size() >= 1);
      UObject* Obj = CflatValueAs(&pArguments[0], UObject*);
      UObjFuncExecute(pFunction, Obj, pArguments, pOutReturnValue, pReturnType);
   };
}

void AddDependencyIfNeeded(RegisteredInfo* pRegInfo, Cflat::TypeUsage* pType)
{
   if (pRegInfo->mStruct == pType->mType)
   {
      return;
   }

   FName* header = mCflatTypeToHeader.Find(pType->mType);
   if (!header)
   {
      return;
   }

   if (pType->isPointer() || pType->isReference())
   {
      mForwardDeclartionTypes.Add(pType->mType);
   }
   else
   {
      pRegInfo->mDependencies.Add(pType->mType);
   }
}

void GatherFunctionInfos(UStruct* pStruct, TArray<RegisteredFunctionInfo>& pOutFunctions)
{
   char funcName[kCharConversionBufferSize];

   int count = 0;
   for (TFieldIterator<UFunction> funcIt(pStruct); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;

      UStruct* funcOwner = static_cast<UStruct*>(function->GetOuter());
      if (funcOwner != pStruct)
      {
        continue;
      }

      // Ignore Editor
      if (function->HasAnyFunctionFlags(FUNC_EditorOnly))
      {
         continue;
      }

      // Ignore non public
      if (function->HasAnyFunctionFlags(FUNC_Private | FUNC_Protected))
      {
         continue;
      }

      pOutFunctions.Add({});
      RegisteredFunctionInfo& funcInfo = pOutFunctions.Last();

      if (!GetFunctionParameters(function, funcInfo.mReturnType, funcInfo.mParameters, funcInfo.mFirstDefaultParamIndex))
      {
         pOutFunctions.Pop(false);
         continue;
      }

      funcInfo.mFunction = function;
      funcInfo.mName = function->GetName();
      funcInfo.mRegisteredIndex = count++;

      FPlatformString::Convert<TCHAR, ANSICHAR>(funcName, kCharConversionBufferSize, *funcInfo.mName);
      funcInfo.mIdentifier = Cflat::Identifier(funcName);
   }
}

void RegisterUStructFunctions(UStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;

   GatherFunctionInfos(pStruct, pRegInfo->mFunctions);

   for (RegisteredFunctionInfo& info : pRegInfo->mFunctions)
   {
      AddDependencyIfNeeded(pRegInfo, &info.mReturnType);
      for (int i = 0; i < info.mParameters.size(); ++i)
      {
         AddDependencyIfNeeded(pRegInfo, &info.mParameters[i]);
      }

      RegisterCflatFunction(cfStruct, info.mFunction, info.mIdentifier, info.mParameters, info.mReturnType);

      if (info.mFirstDefaultParamIndex == -1)
      {
         continue;
      }

      // Functions with default parameter
      CflatSTLVector(Cflat::TypeUsage) parametersForDefault;
      parametersForDefault.reserve(info.mParameters.size());
      for (int i = 0; i < info.mParameters.size() - 1; ++i)
      {
         parametersForDefault.push_back(info.mParameters[i]);
         if (i >= info.mFirstDefaultParamIndex - 1)
         {
            RegisterCflatFunction(cfStruct, info.mFunction, info.mIdentifier, parametersForDefault, info.mReturnType);
         }
      }
   }

   pRegInfo->mMembersCount = cfStruct->mMembers.size();
   pRegInfo->mMethodCount = cfStruct->mMethods.size();

   {
      CflatSTLVector(Function*) staticFunctions;
      cfStruct->mFunctionsHolder.getAllFunctions(&staticFunctions);
      pRegInfo->mFunctionCount = staticFunctions.size();

      for (int i = 0; i < staticFunctions.size(); ++i)
      {
         pRegInfo->mStaticFunctions.Add(staticFunctions[i]);
      }
   }
}

void RegisterInterfaceFunctions(UStruct* pInterface, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;

   GatherFunctionInfos(pInterface, pRegInfo->mFunctions);

   for (RegisteredFunctionInfo& info : pRegInfo->mFunctions)
   {
      AddDependencyIfNeeded(pRegInfo, &info.mReturnType);
      for (int i = 0; i < info.mParameters.size(); ++i)
      {
         AddDependencyIfNeeded(pRegInfo, &info.mParameters[i]);
      }

      RegisterCflatFunction(cfStruct, info.mFunction, info.mIdentifier, info.mParameters, info.mReturnType);
      if (info.mFunction->HasAllFunctionFlags(FUNC_BlueprintEvent | FUNC_Event))
      {
         RegisterInterfaceFunction(cfStruct, info.mFunction, info.mIdentifier, info.mParameters, info.mReturnType);
      }

      if (info.mFirstDefaultParamIndex == -1)
      {
         continue;
      }

      // Functions with default parameter
      CflatSTLVector(Cflat::TypeUsage) parametersForDefault;
      parametersForDefault.reserve(info.mParameters.size());
      for (int i = 0; i < info.mParameters.size() - 1; ++i)
      {
         parametersForDefault.push_back(info.mParameters[i]);
         if (i >= info.mFirstDefaultParamIndex - 1)
         {
            RegisterCflatFunction(cfStruct, info.mFunction, info.mIdentifier, parametersForDefault, info.mReturnType);
         }
      }
   }

   pRegInfo->mMembersCount = cfStruct->mMembers.size();
   pRegInfo->mMethodCount = cfStruct->mMethods.size();

   {
      CflatSTLVector(Function*) staticFunctions;
      cfStruct->mFunctionsHolder.getAllFunctions(&staticFunctions);
      pRegInfo->mFunctionCount = staticFunctions.size();

      for (int i = 0; i < staticFunctions.size(); ++i)
      {
         pRegInfo->mStaticFunctions.Add(staticFunctions[i]);
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
      cfStruct->mCachedMethodIndexDefaultConstructor = cfStruct->mMethods.size();
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();
      method->execute = [] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
      };
   }
   else if (structOps->HasZeroConstructor())
   {
      cfStruct->mCachedMethodIndexDefaultConstructor = cfStruct->mMethods.size();
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
      cfStruct->mCachedMethodIndexDefaultConstructor = cfStruct->mMethods.size();
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();
      method->execute = [structOps] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         void* thiz = CflatValueAs(&pThis, void*);
         structOps->Construct(thiz);
      };
   }

   // Copy Constructor
   if (structOps->HasCopy())
   {
      cfStruct->mCachedMethodIndexCopyConstructor = cfStruct->mMethods.size();
      cfStruct->mMethods.push_back(Cflat::Method(emptyId));
      Cflat::Method* method = &cfStruct->mMethods.back();

      Cflat::TypeUsage refTypeUsage;
      refTypeUsage.mType = cfStruct;
      refTypeUsage.mFlags = (uint8_t)Cflat::TypeUsageFlags::Const | (uint8_t)Cflat::TypeUsageFlags::Reference;
      method->mParameters.push_back(refTypeUsage);

      method->execute = [structOps, refTypeUsage] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         CflatAssert(pArguments.size() == 1u);
         CflatAssert(pArguments[0].mTypeUsage == refTypeUsage);

         void* sourcePtr = pArguments[0].mValueBuffer;
         void* thiz = CflatValueAs(&pThis, void*);
         structOps->Copy(thiz, sourcePtr, 1);
      };
   }
}

void RegisterUStructProperties(UStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;
   for (TFieldIterator<FProperty> propIt(pStruct); propIt; ++propIt)
   {
      FProperty* prop = *propIt;
      if (prop->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected | 
                                      CPF_NativeAccessSpecifierPrivate | 
                                      CPF_EditorOnly))
      {
         continue;
      }

      UStruct* owner = prop->GetOwnerStruct();
      if (owner != pStruct)
      {
        continue;
      }

      FString extendedType;
      FString cppType = prop->GetCPPType(&extendedType);

      if (!IsCflatIdentifierRegistered(cppType, extendedType))
      {
         continue;
      }

      if (!extendedType.IsEmpty())
      {
         cppType += extendedType;
      }

      char nameBuff[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *prop->GetName());
      const Cflat::Identifier memberIdentifier(nameBuff);
      Cflat::Member member(memberIdentifier);

      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *cppType);

      member.mTypeUsage = mEnv->getTypeUsage(nameBuff);

      // Type not recognized
      if (member.mTypeUsage.mType == nullptr)
      {
         continue;
      }

      member.mOffset = (uint16_t)prop->GetOffset_ForInternal();
      cfStruct->mMembers.push_back(member);

      pRegInfo->mProperties.Push(prop);

      AddDependencyIfNeeded(pRegInfo, &member.mTypeUsage);
   }
}

Cflat::Struct* RegisterInterface(UStruct* pInterface)
{
   {
      RegisteredInfo* regInfo = mRegisteredInterfaces.Find(pInterface);
      if (regInfo)
      {
         return regInfo->mStruct;
      }
   }

   FString interfaceName = FString::Printf(TEXT("I%s"), *pInterface->GetName());
   char classTypeName[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(classTypeName, kCharConversionBufferSize, *interfaceName);
   const Cflat::Identifier interfaceIdentifier(classTypeName);

   Cflat::Type* type = mEnv->getType(interfaceIdentifier);
   if (type)
   {
      return static_cast<Cflat::Struct*>(type);
   }
   Cflat::Struct* interfaceStruct = mEnv->registerType<Cflat::Struct>(interfaceIdentifier);
   interfaceStruct->mSize = sizeof(void*);
   interfaceStruct->mAlignment = alignof(void*);

   RegisteredInfo& regInfo = mRegisteredInterfaces.Add(pInterface, {});
   regInfo.mStruct = interfaceStruct;
   regInfo.mIdentifier = interfaceStruct->mIdentifier;


   UPackage* package = pInterface->GetPackage();
   const FString& modulePath = package->GetMetaData().GetValue(pInterface, TEXT("ModuleRelativePath"));
   regInfo.mHeader = FName(*modulePath);

   mCflatTypeToStruct.Add(interfaceStruct, pInterface);
   mCflatTypeToHeader.Add(interfaceStruct, regInfo.mHeader);

   return interfaceStruct;
}

Cflat::Struct* RegisterUStruct(TMap<UStruct*, RegisteredInfo>& pRegisterMap, UStruct* pStruct)
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
      Cflat::Type* type = mEnv->getType(classTypeIdentifier);
      if (type)
      {
         return static_cast<Cflat::Struct*>(type);
      }
      cfStruct = mEnv->registerType<Cflat::Struct>(classTypeIdentifier);
   }
   cfStruct->mSize = pStruct->GetStructureSize();
   cfStruct->mAlignment = pStruct->GetMinAlignment();

   bool isClass = pStruct->IsA<UClass>();

   // Register Super Class/Struct
   {
      Cflat::Type* baseCflatType = nullptr;
      UStruct* superStruct = pStruct->GetSuperStruct();

      if (superStruct)
      {
         // Register base class/structure
         baseCflatType = RegisterUStruct(pRegisterMap, superStruct);
         if (superStruct->IsA<UClass>())
         {
            UClass* baseClass = static_cast<UClass*>(superStruct);
            if (baseClass->HasAnyClassFlags(CLASS_Interface))
            {
               RegisterInterface(baseClass);
            }
         }
      }

      if (baseCflatType)
      {
         cfStruct->mBaseTypes.emplace_back();
         Cflat::BaseType& baseType = cfStruct->mBaseTypes.back();
         baseType.mType = baseCflatType;
         baseType.mOffset = 0u;
      }

      if (isClass)
      {
         UClass* uClass = static_cast<UClass*>(pStruct);
         for (int32 i = 0; i < uClass->Interfaces.Num(); ++i)
         {
            FImplementedInterface& iface = uClass->Interfaces[i];
            {
               RegisterUStruct(mRegisteredClasses, iface.Class);
               Cflat::Struct* cfIface = RegisterInterface(iface.Class);

               cfStruct->mBaseTypes.emplace_back();
               Cflat::BaseType& baseType = cfStruct->mBaseTypes.back();
               baseType.mType = cfIface;
               baseType.mOffset = iface.PointerOffset;
            }
         }
      }
   }

   RegisteredInfo& regInfo = pRegisterMap.Add(pStruct, {});
   regInfo.mIsClass = isClass;
   regInfo.mStruct = cfStruct;
   regInfo.mIdentifier = cfStruct->mIdentifier;
   if (!cfStruct->mBaseTypes.empty())
   {
      for (int i = 0; i < cfStruct->mBaseTypes.size(); ++i)
      {
         Cflat::Type* baseCflatType = cfStruct->mBaseTypes[i].mType;
         regInfo.mDependencies.Add(baseCflatType);
      }
   }
   {
      UPackage* package = pStruct->GetPackage();
      const FString& modulePath = package->GetMetaData().GetValue(pStruct, TEXT("ModuleRelativePath"));
      if (modulePath.IsEmpty())
      {
         /* Package path not found, so we use its name as reference for sorting headers. */
         regInfo.mHeader = package->GetFName();
      }
      else
      {
         regInfo.mHeader = FName(*modulePath);
      }
   }
   mCflatTypeToStruct.Add(cfStruct, pStruct);
   mCflatTypeToHeader.Add(cfStruct, regInfo.mHeader);

   return cfStruct;
}

void RegisterRegularEnum(UEnum* pUEnum, const Cflat::Identifier& pEnumIdentifier, Cflat::Namespace* pNamespace)
{
   char nameBuff[kCharConversionBufferSize];

   if (pNamespace->getType(pEnumIdentifier))
   {
      return;
   }

   Cflat::Enum* cfEnum = pNamespace->registerType<Cflat::Enum>(pEnumIdentifier);
   if (pUEnum->GetBoolMetaData(kBlueprintType))
   {
      cfEnum->mSize = sizeof(uint8);
   }
   else
   {
      cfEnum->mSize = sizeof(int64);
   }

   Cflat::TypeUsage enumTypeUsage;
   enumTypeUsage.mType = cfEnum;
   CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const);

   for (int32 i = 0; i < pUEnum->NumEnums() - 1; ++i)
   {
      int64 value = pUEnum->GetValueByIndex(i);
      FString enumValueName = pUEnum->GetNameStringByIndex(i);
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *enumValueName);
      Cflat::Identifier idEnumValueName(nameBuff);

      Cflat::Instance* enumInstance = cfEnum->mInstancesHolder.registerInstance(enumTypeUsage, idEnumValueName);
      enumInstance->mValue.initOnHeap(enumTypeUsage);
      enumInstance->mValue.set(&value);
      CflatSetFlag(enumInstance->mFlags, Cflat::InstanceFlags::EnumValue);
      Cflat::Instance* nsInstance = pNamespace->registerInstance(enumTypeUsage, idEnumValueName);
      nsInstance->mValue = enumInstance->mValue;
      CflatSetFlag(nsInstance->mFlags, Cflat::InstanceFlags::EnumValue);
   }

   RegisteredEnumInfo& regInfo = mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData().GetValue(pUEnum, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   mCflatTypeToEnum.Add(cfEnum, pUEnum);
   mCflatTypeToHeader.Add(cfEnum, regInfo.mHeader);
}

void RegisterRegularEnum(UEnum* pUEnum)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pUEnum->GetName());

   if (IsCflatIdentifierRegistered(nameBuff))
   {
      return;
   }
   Cflat::Identifier enumIdentifier(nameBuff);
   RegisterRegularEnum(pUEnum, enumIdentifier, mEnv->getGlobalNamespace());
}

void RegisterEnumNamespaced(UEnum* pUEnum)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pUEnum->GetName());

   Cflat::Identifier idEnumName(nameBuff);

   Cflat::Namespace* globalNamespace = mEnv->getGlobalNamespace();
   Cflat::Namespace* enumNamespace = globalNamespace->getNamespace(idEnumName);
   if (enumNamespace)
   {
      return;
   }

   enumNamespace = globalNamespace->requestNamespace(idEnumName);
   Cflat::Identifier idEnumType("Type");

   RegisterRegularEnum(pUEnum, idEnumType, enumNamespace);
}

void RegisterEnumClass(UEnum* pUEnum)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pUEnum->GetName());

   if (IsCflatIdentifierRegistered(nameBuff))
   {
      return;
   }

   Cflat::Identifier idEnumName(nameBuff);
   if (mEnv->getType(idEnumName))
   {
      return;
   }
   Cflat::EnumClass* cfEnum = mEnv->registerType<Cflat::EnumClass>(idEnumName);
   if (pUEnum->GetBoolMetaData(kBlueprintType))
   {
      cfEnum->mSize = sizeof(uint8);
   }
   else
   {
      cfEnum->mSize = sizeof(int64);
   }

   Cflat::TypeUsage enumTypeUsage;
   enumTypeUsage.mType = cfEnum;
   CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const);

   for (int32 i = 0; i < pUEnum->NumEnums() - 1; ++i)
   {
      int64 value = pUEnum->GetValueByIndex(i);
      FString enumValueName = pUEnum->GetNameStringByIndex(i);
      FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *enumValueName);
      Cflat::Identifier idEnumValueName(nameBuff);

      Cflat::Instance* enumInstance = cfEnum->mInstancesHolder.registerInstance(enumTypeUsage, idEnumValueName);
      enumInstance->mValue.initOnHeap(enumTypeUsage);
      enumInstance->mValue.set(&value);
      CflatSetFlag(enumInstance->mFlags, Cflat::InstanceFlags::EnumValue);
   }

   RegisteredEnumInfo& regInfo = mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData().GetValue(pUEnum, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   mCflatTypeToEnum.Add(cfEnum, pUEnum);
   mCflatTypeToHeader.Add(cfEnum, regInfo.mHeader);
}

void RegisterEnums()
{
   for (TObjectIterator<UEnum> enumIt; enumIt; ++enumIt)
   {
      UEnum* uEnum = *enumIt;

      if (mIgnoredTypes.Find(uEnum->GetFName()))
      {
         continue;
      }

      {
         UObject* outer = uEnum->GetOuter();
         if (outer && CheckShouldIgnoreModule(outer->GetPackage()))
         {
            continue;
         }
      }

      UEnum::ECppForm enumForm = uEnum->GetCppForm();

      switch(enumForm)
      {
      case UEnum::ECppForm::Regular:
         RegisterRegularEnum(uEnum);
         break;
      case UEnum::ECppForm::Namespaced:
         RegisterEnumNamespaced(uEnum);
         break;
      case UEnum::ECppForm::EnumClass:
         RegisterEnumClass(uEnum);
         break;
      }
   }
}

void RegisterStructs()
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
      if (!CheckShouldRegisterType(uStruct))
      {
         continue;
      }

      RegisterUStruct(mRegisteredStructs, uStruct);
   }
}

void RegisterClasses()
{
   RegisterUStruct(mRegisteredClasses, UObject::StaticClass());
   RegisterUStruct(mRegisteredClasses, UInterface::StaticClass());
   RegisterUStruct(mRegisteredClasses, UField::StaticClass());
   RegisterUStruct(mRegisteredClasses, UStruct::StaticClass());
   RegisterUStruct(mRegisteredClasses, UClass::StaticClass());
   RegisterUStruct(mRegisteredClasses, UScriptStruct::StaticClass());
   RegisterUStruct(mRegisteredClasses, ULineBatchComponent::StaticClass());

   for (TObjectIterator<UClass> classIt; classIt; ++classIt)
   {
      UClass* uClass = *classIt;
      UStruct* uStruct = static_cast<UStruct*>(uClass);
      if (!CheckShouldRegisterType(uStruct))
      {
         continue;
      }
      RegisterUStruct(mRegisteredClasses, uStruct);

      if (uClass->HasAnyClassFlags(CLASS_Interface))
      {
         if (CheckShouldRegisterInterface(uClass))
         {
            RegisterInterface(uClass);
         }
      }
   }
}

void RegisterProperties()
{
   for (auto& pair : mRegisteredStructs)
   {
      RegisterUStructProperties(pair.Key, &pair.Value);
   }
   for (auto& pair : mRegisteredClasses)
   {
      RegisterUStructProperties(pair.Key, &pair.Value);
   }
}

void RegisterCastFromObject(UClass* pClass, Cflat::Struct* pCfStruct, const Cflat::TypeUsage& pParamTypeUsage)
{
   Cflat::TypeUsage typeUsage;
   typeUsage.mType = pCfStruct;

   Cflat::TypeUsage returnTypeUsage;
   returnTypeUsage.mType = pCfStruct;
   returnTypeUsage.mPointerLevel = 1u;

   Cflat::Function* castFromObjectFunction = mEnv->registerFunction("Cast");
   castFromObjectFunction->mTemplateTypes.push_back(typeUsage);
   castFromObjectFunction->mParameters.push_back(pParamTypeUsage);
   castFromObjectFunction->mReturnTypeUsage = returnTypeUsage;
   castFromObjectFunction->execute = [pClass](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) 
   {
      CflatAssert(pArguments.size() == 1);
      CflatAssert(pArguments[0].mTypeUsage.mType->mCategory == TypeCategory::StructOrClass);

      char* ptr = nullptr;

      UObject* uObj = CflatValueAs(&pArguments[0], UObject*);
      if (uObj)
      {
         UClass* sourceClass = uObj->GetClass();

         if (sourceClass == pClass)
         {
            ptr = CflatValueAs(&pArguments[0], char*);
         }
         else if (sourceClass->IsChildOf(pClass))
         {
            ptr = CflatValueAs(&pArguments[0], char*);
         }
      }

      pOutReturnValue->set(&ptr);
   };
}

void RegisterFunctions()
{
   mUObjectTypeUsage = mEnv->getTypeUsage("UObject*");
   const Cflat::TypeUsage uClassTypeUsage = mEnv->getTypeUsage("UClass*");
   const Cflat::TypeUsage uScriptStructTypUsage = mEnv->getTypeUsage("UScriptStruct*");
   const Cflat::Identifier staticStructIdentifier = "StaticStruct";
   const Cflat::Identifier staticClassIdentifier = "StaticClass";

   for (auto& pair : mRegisteredStructs)
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
      RegisterUStructFunctions(pair.Key, &pair.Value);
   }
   for (auto& pair : mRegisteredClasses)
   {
      UStruct* uStruct = pair.Key;
      UClass* uClass = static_cast<UClass*>(uStruct);
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
      if (uClass->HasAnyClassFlags(CLASS_Interface))
      {
         pair.Value.mFunctionCount = cfStruct->mFunctionsHolder.getFunctionsCount();
      }
      else
      {
         RegisterUStructFunctions(uStruct, &pair.Value);
      }
      RegisterCastFromObject(uClass, cfStruct, mUObjectTypeUsage);
   }
   for (auto& pair : mRegisteredInterfaces)
   {
      RegisterInterfaceFunctions(pair.Key, &pair.Value);
   }
}

template<typename BaseSubsystemType, typename OwnerType>
void RegisterSubsystem(Cflat::Class* pCfOwnerType, UClass* pClass, Cflat::Struct* pCfStruct)
{
   Cflat::TypeUsage typeUsage;
   typeUsage.mType = pCfStruct;

   Cflat::TypeUsage returnTypeUsage;
   returnTypeUsage.mType = pCfStruct;
   returnTypeUsage.mPointerLevel = 1u;

   const size_t methodIndex = pCfOwnerType->mMethods.size() - 1u;
   Cflat::Method getSubsystemMethod("GetSubsystem");
   getSubsystemMethod.mTemplateTypes.push_back(typeUsage);
   getSubsystemMethod.mReturnTypeUsage = returnTypeUsage;
   getSubsystemMethod.execute = [pCfOwnerType, methodIndex, pClass](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
   {
      Cflat::Method* method = &pCfOwnerType->mMethods[methodIndex];
      CflatAssert(method->mParameters.size() == pArguments.size());
      CflatAssert(pOutReturnValue);
      BaseSubsystemType* result = CflatValueAs(&pThis, OwnerType*)->GetSubsystemBase(pClass);
      pOutReturnValue->set(&result);
   };
   pCfOwnerType->mMethods.push_back(getSubsystemMethod);
}

void RegisterSubsystems()
{
   const Cflat::TypeUsage uObjectTypeUsage = mEnv->getTypeUsage("UObject*");
   Cflat::Class* gameInstanceClass = static_cast<Cflat::Class*>(mEnv->getGlobalNamespace()->getType("UGameInstance"));
   Cflat::Class* worldClass = static_cast<Cflat::Class*>(mEnv->getGlobalNamespace()->getType("UWorld"));
   for (auto& pair : mRegisteredClasses)
   {
      UStruct* uStruct = pair.Key;
      if (uStruct != UGameInstanceSubsystem::StaticClass() &&
          uStruct->IsChildOf(UGameInstanceSubsystem::StaticClass()))
      {
         UClass* uClass = static_cast<UClass*>(uStruct);
         Cflat::Struct* cfStruct = pair.Value.mStruct;
         RegisterSubsystem<UGameInstanceSubsystem, UGameInstance>(gameInstanceClass, uClass, cfStruct);
      }
      else if (uStruct != UWorldSubsystem::StaticClass() &&
          uStruct->IsChildOf(UWorldSubsystem::StaticClass()))
      {
         UClass* uClass = static_cast<UClass*>(uStruct);
         Cflat::Struct* cfStruct = pair.Value.mStruct;
         RegisterSubsystem<UWorldSubsystem, UWorld>(worldClass, uClass, cfStruct);
      }
   }
}

void RegisterTemplatedObjectPtr(UStruct* pStruct, Cflat::Struct* pCfStruct)
{
   static const Cflat::Identifier kTObjectPtrId("TObjectPtr");
   static const Cflat::Identifier kGetId("Get");
   static const Cflat::Identifier kOperatorStarId("operator*");

   Cflat::TypeUsage typeUsage;
   typeUsage.mType = pCfStruct;

   Cflat::TypeUsage returnTypeUsage;
   returnTypeUsage.mType = pCfStruct;
   returnTypeUsage.mPointerLevel = 1u;

   CflatArgsVector(Cflat::TypeUsage) templateTypes;
   templateTypes.push_back(typeUsage);
   Cflat::Struct* tObjPtr = mEnv->registerTemplate<Cflat::Struct>(kTObjectPtrId, templateTypes);
   tObjPtr->mSize = sizeof(TObjectPtr<UObject>);

   // Add constructor taking the Object pointer as parameter
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kEmptyId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mParameters.push_back(returnTypeUsage);
      method->execute = [](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value * pOutReturnValue) {
         CflatAssert(pArguments.size() == 1u);
         UObject* Obj = CflatValueAs(&pArguments[0], UObject*);
         TObjectPtr<UObject>* thisObj = CflatValueAs(&pThis, TObjectPtr<UObject>*);
         (*thisObj) = TObjectPtr<UObject>(Obj);
      };
   }

   const auto getPtrExec = [](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value) & pArguments, Cflat::Value * pOutReturnValue) {
      TObjectPtr<UObject>* thisObj = CflatValueAs(&pThis, TObjectPtr<UObject>*);
      UObject* Obj = thisObj->Get();
      pOutReturnValue->set(&Obj);
   };

   // Register Get
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kGetId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mReturnTypeUsage = returnTypeUsage;
      method->execute = getPtrExec;
   }

   // Register the operator *
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kOperatorStarId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mReturnTypeUsage = returnTypeUsage;
      method->execute = getPtrExec;
   }
}

void RegisterTemplatedWeakObjectPtr(UStruct* pStruct, Cflat::Struct* pCfStruct)
{
   static const Cflat::Identifier kTObjectPtrId("TWeakObjectPtr");
   static const Cflat::Identifier kGetId("Get");
   static const Cflat::Identifier kIsValidId("IsValid");
   static const Cflat::Identifier kOperatorStarId("operator*");

   Cflat::TypeUsage typeUsage;
   typeUsage.mType = pCfStruct;

   Cflat::TypeUsage returnTypeUsage;
   returnTypeUsage.mType = pCfStruct;
   returnTypeUsage.mPointerLevel = 1u;

   CflatArgsVector(Cflat::TypeUsage) templateTypes;
   templateTypes.push_back(typeUsage);
   Cflat::Struct* tObjPtr = mEnv->registerTemplate<Cflat::Struct>(kTObjectPtrId, templateTypes);
   tObjPtr->mSize = sizeof(TWeakObjectPtr<UObject>);

   // Add constructor taking the Object pointer as parameter
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kEmptyId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mParameters.push_back(returnTypeUsage);
      method->execute = [](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value * pOutReturnValue) {
         CflatAssert(pArguments.size() == 1u);
         UObject* Obj = CflatValueAs(&pArguments[0], UObject*);
         TWeakObjectPtr<UObject>* thisObj = CflatValueAs(&pThis, TWeakObjectPtr<UObject>*);
         (*thisObj) = TWeakObjectPtr<UObject>(Obj);
      };
   }

   const auto getPtrExec = [](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value) & pArguments, Cflat::Value * pOutReturnValue) {
      TWeakObjectPtr<UObject>* thisObj = CflatValueAs(&pThis, TWeakObjectPtr<UObject>*);
      UObject* Obj = thisObj->Get();
      pOutReturnValue->set(&Obj);
   };

   // Register Get
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kGetId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mReturnTypeUsage = returnTypeUsage;
      method->execute = getPtrExec;
   }

   // Register the operator *
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kOperatorStarId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mReturnTypeUsage = returnTypeUsage;
      method->execute = getPtrExec;
   }

   // Register IsValid
   {
      tObjPtr->mMethods.push_back(Cflat::Method(kIsValidId));
      Cflat::Method* method = &tObjPtr->mMethods.back();
      method->mReturnTypeUsage = mEnv->getTypeUsage("bool");
      method->execute = [](const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value * pOutReturnValue) {
         CflatAssert(pArguments.size() == 0u);
         TWeakObjectPtr<UObject>* thisObj = CflatValueAs(&pThis, TWeakObjectPtr<UObject>*);
         bool isValid = thisObj->IsValid();
         pOutReturnValue->set(&isValid);
      };
   }
}

void RegisterTemplates()
{
   for (auto& pair : mRegisteredClasses)
   {
      RegisterTemplatedObjectPtr(pair.Key, pair.Value.mStruct);
      RegisterTemplatedWeakObjectPtr(pair.Key, pair.Value.mStruct);
   }
}

PerHeaderTypes* GetOrCreateHeaderType(UStruct* pStruct, TMap<FName, PerHeaderTypes>& pHeaders)
{
   RegisteredInfo* regInfo = FindRegisteredInfoFromUStruct(pStruct);

   check(regInfo);

   PerHeaderTypes* types = pHeaders.Find(regInfo->mHeader);
   if (types == nullptr)
   {
      types = &(pHeaders.Add(regInfo->mHeader, {}));
      types->mPackage = pStruct->GetPackage();
   }

   return types;
}

PerHeaderTypes* GetOrCreateHeaderType(UEnum* pEnum, TMap<FName, PerHeaderTypes>& pHeaders)
{
   RegisteredEnumInfo* regInfo = mRegisteredEnums.Find(pEnum);

   check(regInfo);

   PerHeaderTypes* types = pHeaders.Find(regInfo->mHeader);
   if (types == nullptr)
   {
      types = &(pHeaders.Add(regInfo->mHeader, {}));
      types->mPackage = pEnum->GetPackage();
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

void MapTypesPerHeaders()
{
   for (const auto& pair : mRegisteredEnums)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pair.Key, mTypesPerHeader);
      types->mEnums.Add(pair.Key);
   }

   for (const auto& pair : mRegisteredStructs)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pair.Key, mTypesPerHeader);
      types->mStructs.Add(pair.Key);
   }

   for (const auto& pair : mRegisteredClasses)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pair.Key, mTypesPerHeader);
      types->mClasses.Add(pair.Key);
   }

   for (const auto& pair : mRegisteredInterfaces)
   {
      PerHeaderTypes* types = GetOrCreateHeaderType(pair.Key, mTypesPerHeader);
      types->mClasses.Add(pair.Key);
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

  const bool isBitFlags = pUEnum->HasMetaData(TEXT("Bitflags"));
  const bool isBlueprintType = pUEnum->GetBoolMetaData(kBlueprintType);
  const TCHAR* enumIntType = TEXT("");
  if (isBitFlags)
  {
     if (isBlueprintType)
     {
        enumIntType = TEXT(" : uint8");
     }
     else
     {
        enumIntType = TEXT(" : uint32");
     }
  }

  switch (enumForm)
  {
  case UEnum::ECppForm::Regular:
    declarationBegin = FString::Printf(TEXT("enum %s%s\n{"), *pUEnum->GetName(), enumIntType);
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
     declarationBegin = FString::Printf(TEXT("enum class %s%s\n{"), *pUEnum->GetName(), enumIntType);
     declarationEnd = "\n};";
     break;
  }

  strEnum.Append(declarationBegin);

  int32 enumCount = pUEnum->NumEnums() - 1;
  for (int32 i = 0; i < enumCount; ++i)
  {
    FString enumComment = pUEnum->GetMetaData(TEXT("Comment"), i);
    int64 value = pUEnum->GetValueByIndex(i);
    FString enumValueName = pUEnum->GetNameStringByIndex(i);
    strEnum.Append(newLineSpace);
    if (!enumComment.IsEmpty())
    {
      enumComment.RemoveFromEnd(TEXT("\n"));
      strEnum.Append(enumComment);
      strEnum.Append(newLineSpace);
    }
    if (isBitFlags)
    {
       if (isBlueprintType)
       {
          strEnum.Append(FString::Printf(TEXT("%s = 0x%02x"), *enumValueName, value));
       }
       else
       {
          strEnum.Append(FString::Printf(TEXT("%s = 0x%08x"), *enumValueName, value));
       }
    }
    else
    {
      strEnum.Append(FString::Printf(TEXT("%s = %d"), *enumValueName, value));
    }
    if (i < enumCount - 1)
    {
      strEnum.Append(",");
    }
  }
  strEnum.Append(declarationEnd);
  pOutContent.Append(strEnum);
}

FString FunctionInfoToString(const RegisteredFunctionInfo& pInfo, int pDefaultParameterIndex = -1)
{
   FString funcStr = "";
   UFunction* func = pInfo.mFunction;
   bool hasDefaultParameter = pInfo.mFirstDefaultParamIndex != -1 && pDefaultParameterIndex != -1;

   if (!hasDefaultParameter)
   {
      FString comment = func->GetMetaData(kMetaComment);
      if (!comment.IsEmpty())
      {
         comment.RemoveFromEnd(TEXT("\n"));
         funcStr.Append(comment);
         funcStr.Append(kNewLineWithIndent1);
      }
   }

   if (func->HasAnyFunctionFlags(FUNC_Static))
   {
      funcStr.Append("static ");
   }

   FProperty* returnProperty = func->GetReturnProperty();
   if (returnProperty)
   {
      if (returnProperty->HasAnyPropertyFlags(CPF_ConstParm))
      {
         funcStr.Append("const ");
      }
      FString extendedType;
      FString cppType;
      if (returnProperty->IsA<FByteProperty>())
      {
         FByteProperty* byteProp = static_cast<FByteProperty*>(returnProperty);
         if (byteProp->Enum)
         {
            UEnum* uEnum = byteProp->Enum;
            cppType = uEnum->CppType.IsEmpty() ? uEnum->GetName() : uEnum->CppType;
         }
      }
      if (cppType.IsEmpty())
      {
         cppType = returnProperty->GetCPPType(&extendedType);
      }
      funcStr.Append(cppType);
      if (!extendedType.IsEmpty())
      {
         funcStr.Append(extendedType);
      }
      if (returnProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
      {
         funcStr.Append("& ");
      }
   }
   else
   {
      funcStr.Append("void");
   }
   funcStr.Append(" ");
   funcStr.Append(pInfo.mIdentifier.mName);
   funcStr.Append("(");


   bool defaultParamsBegan = false;
   int32 propCount = 0;
   for (TFieldIterator<FProperty> propIt(func);
        propIt && propIt->HasAnyPropertyFlags(CPF_Parm) &&
        !propIt->HasAnyPropertyFlags(CPF_ReturnParm);
        ++propIt, ++propCount)
   {
      if (hasDefaultParameter && propCount >= pDefaultParameterIndex)
      {
         if (!defaultParamsBegan)
         {
            defaultParamsBegan = true;
            funcStr.Append("/*");
         }
      }
      if (propCount > 0)
      {
         funcStr.Append(", ");
      }

      FString extendedType;
      FString cppType;
      if (propIt->IsA<FByteProperty>())
      {
         FByteProperty* byteProp = static_cast<FByteProperty*>(*propIt);
         if (byteProp->Enum)
         {
            UEnum* uEnum = byteProp->Enum;
            cppType = uEnum->CppType.IsEmpty() ? uEnum->GetName() : uEnum->CppType;
         }
      }
      if (cppType.IsEmpty())
      {
         cppType = propIt->GetCPPType(&extendedType);
      }
      funcStr.Append(cppType);

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

      if (defaultParamsBegan)
      {
         FString metaDataName = FString::Printf(TEXT("CPP_Default_%s"), *propIt->GetName());
         const FString* metaDataValue = func->FindMetaData(*metaDataName);
         if (metaDataValue && !metaDataValue->IsEmpty())
         {
            funcStr.Append(" = ");
            funcStr.Append(*metaDataValue);
         }
      }
   }
   if (defaultParamsBegan)
   {
      funcStr.Append(" */");
   }
   funcStr.Append(")");
   if (func->HasAnyFunctionFlags(FUNC_Const))
   {
      funcStr.Append(" const");
   }
   funcStr.Append(";");
   return funcStr;
}

FString FunctionInfoInterfaceWrapperToString(const RegisteredFunctionInfo& pInfo, int pDefaultParameterIndex = -1)
{
   FString funcStr = "";
   UFunction* func = pInfo.mFunction;
   bool hasDefaultParameter = pInfo.mFirstDefaultParamIndex != -1 && pDefaultParameterIndex != -1;

   if (!hasDefaultParameter)
   {
      FString comment = func->GetMetaData(kMetaComment);
      if (!comment.IsEmpty())
      {
         comment.RemoveFromEnd(TEXT("\n"));
         funcStr.Append(comment);
         funcStr.Append(kNewLineWithIndent1);
      }
   }

   funcStr.Append("static ");

   FProperty* returnProperty = func->GetReturnProperty();
   if (returnProperty)
   {
      if (returnProperty->HasAnyPropertyFlags(CPF_ConstParm))
      {
         funcStr.Append("const ");
      }
      FString extendedType;
      FString cppType;
      if (returnProperty->IsA<FByteProperty>())
      {
         FByteProperty* byteProp = static_cast<FByteProperty*>(returnProperty);
         if (byteProp->Enum)
         {
            UEnum* uEnum = byteProp->Enum;
            cppType = uEnum->CppType.IsEmpty() ? uEnum->GetName() : uEnum->CppType;
         }
      }
      if (cppType.IsEmpty())
      {
         cppType = returnProperty->GetCPPType(&extendedType);
      }
      funcStr.Append(cppType);
      if (!extendedType.IsEmpty())
      {
         funcStr.Append(extendedType);
      }
      if (returnProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
      {
         funcStr.Append("& ");
      }
   }
   else
   {
      funcStr.Append("void");
   }
   funcStr.Append(" Execute_");
   funcStr.Append(pInfo.mIdentifier.mName);
   funcStr.Append("(");


   bool defaultParamsBegan = false;
   int32 propCount = 1;
   funcStr.Append("UObject* Obj");
   for (TFieldIterator<FProperty> propIt(func);
        propIt && propIt->HasAnyPropertyFlags(CPF_Parm) &&
        !propIt->HasAnyPropertyFlags(CPF_ReturnParm);
        ++propIt, ++propCount)
   {
      if (hasDefaultParameter && propCount >= pDefaultParameterIndex)
      {
         if (!defaultParamsBegan)
         {
            defaultParamsBegan = true;
            funcStr.Append("/*");
         }
      }
      if (propCount > 0)
      {
         funcStr.Append(", ");
      }

      FString extendedType;
      FString cppType;
      if (propIt->IsA<FByteProperty>())
      {
         FByteProperty* byteProp = static_cast<FByteProperty*>(*propIt);
         if (byteProp->Enum)
         {
            UEnum* uEnum = byteProp->Enum;
            cppType = uEnum->CppType.IsEmpty() ? uEnum->GetName() : uEnum->CppType;
         }
      }
      if (cppType.IsEmpty())
      {
         cppType = propIt->GetCPPType(&extendedType);
      }
      funcStr.Append(cppType);

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

      if (defaultParamsBegan)
      {
         FString metaDataName = FString::Printf(TEXT("CPP_Default_%s"), *propIt->GetName());
         const FString* metaDataValue = func->FindMetaData(*metaDataName);
         if (metaDataValue && !metaDataValue->IsEmpty())
         {
            funcStr.Append(" = ");
            funcStr.Append(*metaDataValue);
         }
      }
   }
   if (defaultParamsBegan)
   {
      funcStr.Append(" */");
   }
   funcStr.Append(");");
   return funcStr;
}

void AidHeaderAppendStruct(UStruct* pUStruct, FString& pOutContent)
{
  const RegisteredInfo* regInfo = mRegisteredStructs.Find(pUStruct);
  if (regInfo == nullptr)
  {
    return;
  }
  Cflat::Struct* cfStruct = regInfo->mStruct;
  // Check if the struct was overwritten
  {
    Cflat::Type* type = mEnv->getType(regInfo->mIdentifier);
    if (!type)
    {
      return;
    }
    Cflat::Struct* regStruct = static_cast<Cflat::Struct*>(type);
    // Was overwriten, ignore it
    if (regStruct != cfStruct)
    {
      return;
    }
  }

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
        funcStr.Append(UnrealModule::GetTypeUsageAsString(*paramUsage));
      }
      funcStr.Append(");");
      strStruct.Append(funcStr);
    }
  }
  strStruct.Append(kNewLineWithIndent1 + "static UScriptStruct* StaticStruct();");

  // properties
  for (const FProperty* prop : regInfo->mProperties)
  {
    // Inherited properties should be in their base classes
    UStruct* owner = prop->GetOwnerStruct();
    if (owner != pUStruct)
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
      FString comment = prop->GetMetaData(kMetaComment);
      comment.RemoveFromEnd(TEXT("\n"));
      propStr.Append(comment);
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

  // Members that where manually extended
  if (cfStruct->mMembers.size() > regInfo->mMembersCount)
  {
    publicPropStr.Append("\n");
    publicPropStr.Append(kNewLineWithIndent1);
    publicPropStr.Append("// Begin manually extended members: ");
    for (int i = regInfo->mMembersCount; i < cfStruct->mMembers.size(); ++i)
    {
      FString propStr = UnrealModule::GetMemberAsString(&cfStruct->mMembers[i]);
      publicPropStr.Append(kNewLineWithIndent1);
      publicPropStr.Append(propStr + ";");
    }
    publicPropStr.Append(kNewLineWithIndent1);
    publicPropStr.Append("// End manually extended members");
  }

  // functions
  FString publicFuncStr = {};

  for (const RegisteredFunctionInfo& info : regInfo->mFunctions)
  {
     {
        FString funcStr = FunctionInfoToString(info);
        publicFuncStr.Append(kNewLineWithIndent1);
        publicFuncStr.Append(funcStr);
     }

     if (info.mFirstDefaultParamIndex != -1)
     {
        for (int i = info.mParameters.size() - 1; i >= 0; --i)
        {
           if (i >= info.mFirstDefaultParamIndex)
           {
              FString funcStr = FunctionInfoToString(info, i);
              publicFuncStr.Append(kNewLineWithIndent1);
              publicFuncStr.Append(funcStr);
           }
        }
     }
  }


  // Manually extended methods/functions
  if (cfStruct->mMethods.size() > regInfo->mMethodCount)
  {
    publicFuncStr.Append("\n");
    publicFuncStr.Append(kNewLineWithIndent1);
    publicFuncStr.Append("// Begin Methods manually extended: ");
    for (int i = regInfo->mMethodCount; i < cfStruct->mMethods.size(); ++i)
    {
      FString methodStr = UnrealModule::GetMethodAsString(&cfStruct->mMethods[i]);
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append(methodStr + ";");
    }
    publicFuncStr.Append(kNewLineWithIndent1);
    publicFuncStr.Append("// End Methods manually extended");
  }

  size_t functionCount = cfStruct->mFunctionsHolder.getFunctionsCount();
  if (functionCount > regInfo->mFunctionCount)
  {
    CflatSTLVector(Function*) functions;
    cfStruct->mFunctionsHolder.getAllFunctions(&functions);
    publicFuncStr.Append("\n");
    publicFuncStr.Append(kNewLineWithIndent1);
    publicFuncStr.Append("// Begin Functions manually extended: ");
    for (int i = 0; i < functions.size(); ++i)
    {
      if (regInfo->mStaticFunctions.Find(functions[i]))
      {
         continue;
      }
      FString funcStr = UnrealModule::GetFunctionAsString(functions[i]);
      publicFuncStr.Append(kNewLineWithIndent1 + "static ");
      publicFuncStr.Append(funcStr + ";");
    }
    publicFuncStr.Append(kNewLineWithIndent1);
    publicFuncStr.Append("// End Functions manually extended");
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

void AidHeaderAppendInterface(UClass* pUClass, FString& pOutContent)
{
   const RegisteredInfo* regInfo = mRegisteredInterfaces.Find(pUClass);
   if (regInfo == nullptr)
   {
      return;
   }

   Cflat::Struct* cfStruct = regInfo->mStruct;

   // Check if the struct was overwritten
   {
      Cflat::Type* type = mEnv->getType(regInfo->mIdentifier);
      if (!type)
      {
         return;
      }
      Cflat::Struct* regStruct = static_cast<Cflat::Struct*>(type);
      // Was overwriten, ignore it
      if (regStruct != cfStruct)
      {
         return;
      }
   }

   FString strClass = "\n";

   // Interface declaration
   strClass.Append("\nclass ");
   strClass.Append(cfStruct->mIdentifier.mName);
   // Body
   strClass.Append("\n{");

   // functions
   FString publicFuncStr = "";

   for (const RegisteredFunctionInfo& info : regInfo->mFunctions)
   {
      {
         FString funcStr = FunctionInfoToString(info);
         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(funcStr);
      }

      if (info.mFirstDefaultParamIndex != -1)
      {
         for (int i = info.mParameters.size() - 1; i >= 0; --i)
         {
            if (i >= info.mFirstDefaultParamIndex)
            {
               FString funcStr = FunctionInfoToString(info, i);
               publicFuncStr.Append(kNewLineWithIndent1);
               publicFuncStr.Append(funcStr);
            }
         }
      }
      if (info.mFunction->HasAllFunctionFlags(FUNC_BlueprintEvent | FUNC_Event))
      {
         FString funcStr = FunctionInfoInterfaceWrapperToString(info);
         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(funcStr);
      }
   }

   // Manually extended methods/functions
   if (cfStruct->mMethods.size() > regInfo->mMethodCount)
   {
      publicFuncStr.Append("\n");
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// Begin Methods manually extended: ");
      TMap<FString, TArray<const Cflat::Method*>> registeredTemplated;

      for (int i = regInfo->mMethodCount; i < cfStruct->mMethods.size(); ++i)
      {
         const Cflat::Method* method = &cfStruct->mMethods[i];

         FString methodStr = UnrealModule::GetMethodAsString(method);
         bool hasTemplates = method->mTemplateTypes.size() > 0;
         if (hasTemplates)
         {
            TArray<const Cflat::Method*>* methods = registeredTemplated.Find(methodStr);
            if (methods == nullptr)
            {
               methods = &registeredTemplated.Add(methodStr, {});
            }
            methods->Add(method);
         }
         else
         {

            publicFuncStr.Append(kNewLineWithIndent1);
            publicFuncStr.Append(methodStr + ";");
         }
      }

      for (const auto& it : registeredTemplated)
      {
         FString typesComment = "/** T available as: ";
         for (const Cflat::Method* method : it.Value)
         {
            for (const Cflat::TypeUsage& templateType : method->mTemplateTypes)
            {
               typesComment.Append(kNewLineWithIndent1);
               typesComment.Append("*  ");
               typesComment.Append(UnrealModule::GetTypeUsageAsString(templateType));
            }
         }
         typesComment.Append(kNewLineWithIndent1);
         typesComment.Append("*/");

         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(typesComment);
         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(it.Key + ";");
      }

      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// End Methods manually extended");
   }

   size_t functionCount = cfStruct->mFunctionsHolder.getFunctionsCount();
   if (functionCount > regInfo->mFunctionCount)
   {
      CflatSTLVector(Function*) functions;
      cfStruct->mFunctionsHolder.getAllFunctions(&functions);
      publicFuncStr.Append("\n");
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// Begin Functions manually extended: ");
      for (int i = 0; i < functions.size(); ++i)
      {
         if (regInfo->mStaticFunctions.Find(functions[i]))
         {
            continue;
         }
         FString funcStr = UnrealModule::GetFunctionAsString(functions[i]);
         publicFuncStr.Append(kNewLineWithIndent1 + "static ");
         publicFuncStr.Append(funcStr + ";");
      }
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// End Functions manually extended");
   }

   strClass.Append("\npublic:");
   strClass.Append(publicFuncStr);
   strClass.Append("\n};");

   pOutContent.Append(strClass);
}

void AidHeaderAppendClass(UStruct* pUStruct, FString& pOutContent)
{
   const RegisteredInfo* regInfo = mRegisteredClasses.Find(pUStruct);
   if (regInfo == nullptr)
   {
      return;
   }

   UClass* uClass = static_cast<UClass*>(pUStruct);
   if (uClass->HasAnyClassFlags(CLASS_Interface))
   {
      AidHeaderAppendInterface(uClass, pOutContent);
   }

   Cflat::Struct* cfStruct = regInfo->mStruct;

   // Check if the struct was overwritten
   {
      Cflat::Type* type = mEnv->getType(regInfo->mIdentifier);
      if (!type)
      {
         return;
      }
      Cflat::Struct* regStruct = static_cast<Cflat::Struct*>(type);
      // Was overwriten, ignore it
      if (regStruct != cfStruct)
      {
         return;
      }
   }

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
         FString comment = prop->GetMetaData(kMetaComment);
         comment.RemoveFromEnd(TEXT("\n"));
         propStr.Append(comment);
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

   // Members that where manually extended
   if (cfStruct->mMembers.size() > regInfo->mMembersCount)
   {
      publicPropStr.Append("\n");
      publicPropStr.Append(kNewLineWithIndent1);
      publicPropStr.Append("// Begin manually extended members: ");
      for (int i = regInfo->mMembersCount; i < cfStruct->mMembers.size(); ++i)
      {
         FString propStr = UnrealModule::GetMemberAsString(&cfStruct->mMembers[i]);
         publicPropStr.Append(kNewLineWithIndent1);
         publicPropStr.Append(propStr + ";");
      }
      publicPropStr.Append(kNewLineWithIndent1);
      publicPropStr.Append("// End manually extended members");
   }

   // functions
   FString publicFuncStr = kNewLineWithIndent1 + "static UClass* StaticClass();";

   for (const RegisteredFunctionInfo& info : regInfo->mFunctions)
   {
      {
         FString funcStr = FunctionInfoToString(info);
         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(funcStr);
      }

      if (info.mFirstDefaultParamIndex != -1)
      {
         for (int i = info.mParameters.size() - 1; i >= 0; --i)
         {
            if (i >= info.mFirstDefaultParamIndex)
            {
               FString funcStr = FunctionInfoToString(info, i);
               publicFuncStr.Append(kNewLineWithIndent1);
               publicFuncStr.Append(funcStr);
            }
         }
      }
   }

   // Manually extended methods/functions
   if (cfStruct->mMethods.size() > regInfo->mMethodCount)
   {
      publicFuncStr.Append("\n");
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// Begin Methods manually extended: ");
      TMap<FString, TArray<const Cflat::Method*>> registeredTemplated;

      for (int i = regInfo->mMethodCount; i < cfStruct->mMethods.size(); ++i)
      {
         const Cflat::Method* method = &cfStruct->mMethods[i];

         FString methodStr = UnrealModule::GetMethodAsString(method);
         bool hasTemplates = method->mTemplateTypes.size() > 0;
         if (hasTemplates)
         {
            TArray<const Cflat::Method*>* methods = registeredTemplated.Find(methodStr);
            if (methods == nullptr)
            {
               methods = &registeredTemplated.Add(methodStr, {});
            }
            methods->Add(method);
         }
         else
         {

            publicFuncStr.Append(kNewLineWithIndent1);
            publicFuncStr.Append(methodStr + ";");
         }
      }

      for (const auto& it : registeredTemplated)
      {
         FString typesComment = "/** T available as: ";
         for (const Cflat::Method* method : it.Value)
         {
            for (const Cflat::TypeUsage& templateType : method->mTemplateTypes)
            {
               typesComment.Append(kNewLineWithIndent1);
               typesComment.Append("*  ");
               typesComment.Append(UnrealModule::GetTypeUsageAsString(templateType));
            }
         }
         typesComment.Append(kNewLineWithIndent1);
         typesComment.Append("*/");

         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(typesComment);
         publicFuncStr.Append(kNewLineWithIndent1);
         publicFuncStr.Append(it.Key + ";");
      }

      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// End Methods manually extended");
   }

   size_t functionCount = cfStruct->mFunctionsHolder.getFunctionsCount();
   if (functionCount > regInfo->mFunctionCount)
   {
      CflatSTLVector(Function*) functions;
      cfStruct->mFunctionsHolder.getAllFunctions(&functions);
      publicFuncStr.Append("\n");
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// Begin Functions manually extended: ");
      for (int i = 0; i < functions.size(); ++i)
      {
         if (regInfo->mStaticFunctions.Find(functions[i]))
         {
            continue;
         }
         FString funcStr = UnrealModule::GetFunctionAsString(functions[i]);
         publicFuncStr.Append(kNewLineWithIndent1 + "static ");
         publicFuncStr.Append(funcStr + ";");
      }
      publicFuncStr.Append(kNewLineWithIndent1);
      publicFuncStr.Append("// End Functions manually extended");
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

void AppendStructWithDependenciesRecursively(FName pHeader, PerHeaderTypes& pTypes, UStruct* pStruct)
{
   if (pTypes.mIncluded.Find(pStruct))
   {
      return;
   }

   if (mHeaderClassesToIgnore.Find(pStruct->GetFName()) || mHeaderStructsToIgnore.Find(pStruct->GetFName()))
   {
      return;
   }

   RegisteredInfo* regInfo = FindRegisteredInfoFromUStruct(pStruct);
   if (!regInfo)
   {
      return;
   }

   pTypes.mIncluded.Add(pStruct);

   for (Cflat::Type* cfType : regInfo->mDependencies)
   {
      UStruct** depUStruct = mCflatTypeToStruct.Find(cfType);
      if (!depUStruct)
      {
         continue;
      }

      RegisteredInfo* depRegInfo = FindRegisteredInfoFromUStruct(*depUStruct);

      if (!depRegInfo)
      {
         continue;
      }

      if (depRegInfo->mHeader != pHeader)
      {
         continue;
      }

      // Circular dependency. Forward declare it.
      if (depRegInfo->mDependencies.Find(regInfo->mStruct))
      {
         mForwardDeclartionTypes.Add(cfType);
         mForwardDeclartionTypes.Add(regInfo->mStruct);
      }

      AppendStructWithDependenciesRecursively(pHeader, pTypes, *depUStruct);
   }

   if (regInfo->mIsClass)
   {
      AidHeaderAppendClass(pStruct, pTypes.mHeaderContent);
   }
   else
   {
      AidHeaderAppendStruct(pStruct, pTypes.mHeaderContent);
   }
}

void CreateHeaderContent(FName pHeader, TArray<FName>& pHeaderIncludeOrder)
{
   if (mHeaderAlreadyIncluded.Find(pHeader))
   {
      return;
   }

   PerHeaderTypes* types = mTypesPerHeader.Find(pHeader);
   if (!types)
   {
      return;
   }

   mHeaderAlreadyIncluded.Add(pHeader);

   // First we check for header dependency
   for (const UStruct* uStruct : types->mStructs)
   {
      if (mHeaderStructsToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      RegisteredInfo* regInfo = mRegisteredStructs.Find(uStruct);
      if (!regInfo)
      {
         continue;
      }

      for (Cflat::Type* cfType : regInfo->mDependencies)
      {
         FName* header = mCflatTypeToHeader.Find(cfType);

         if (!header || *header == pHeader)
         {
            continue;
         }

         if (!mHeaderAlreadyIncluded.Find(*header))
         {
            CreateHeaderContent(*header, pHeaderIncludeOrder);
         }
      }
   }

   for (const UStruct* uStruct : types->mClasses)
   {
      if (mHeaderClassesToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      RegisteredInfo* regInfo = mRegisteredClasses.Find(uStruct);
      if (!regInfo)
      {
         regInfo = mRegisteredInterfaces.Find(uStruct);
      }
      if (!regInfo)
      {
         continue;
      }

      for (Cflat::Type* cfType : regInfo->mDependencies)
      {
         FName* header = mCflatTypeToHeader.Find(cfType);

         if (!header || *header == pHeader)
         {
            continue;
         }

         if (!mHeaderAlreadyIncluded.Find(*header))
         {
            CreateHeaderContent(*header, pHeaderIncludeOrder);
         }
      }
   }

   pHeaderIncludeOrder.Add(pHeader);

   // Generate the header strings
   {
      const FString kHeaderName = pHeader == NAME_None ? TEXT("Unknown Header") : pHeader.ToString();
      types->mHeaderContent = FString::Printf(TEXT("\n\n%s\n// %s\n%s"), *kHeaderSeparator, *kHeaderName, *kHeaderSeparator);
   }

   // Enums
   for (const UEnum* uEnum : types->mEnums)
   {
      if (mHeaderEnumsToIgnore.Find(uEnum->GetFName()))
      {
         continue;
      }
      AidHeaderAppendEnum(uEnum, types->mHeaderContent);
   }

   for (UStruct* uStruct : types->mStructs)
   {
      if (mHeaderStructsToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pHeader, *types, uStruct);
   }

   for (UStruct* uStruct : types->mClasses)
   {
      if (mHeaderClassesToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pHeader, *types, uStruct);
   }

}

void GenerateAidHeader(const FString& pFilePath)
{
   FString content = "// Auto Generated From Auto Registered UClasses";
   content.Append("\n#pragma once");
   content.Append("\n#if defined (CFLAT_ENABLED)");

   FString includeContent = "// Auto Generated From Auto Registered UClasses";
   includeContent.Append("\n#pragma once");

   MapTypesPerHeaders();

   TArray<FName> headerIncludeOrder;
   headerIncludeOrder.Reserve(mTypesPerHeader.Num());

   for (auto& typesPair : mTypesPerHeader)
   {
      CreateHeaderContent(typesPair.Key, headerIncludeOrder);
   }

   // Forward declartions
   {
      FString fwdStructs = "\n\n// Forward Structs Declaration";
      FString fwdClasses = "\n\n// Forward Classes Declaration";

      for (Cflat::Type* fwdType : mForwardDeclartionTypes)
      {
         UStruct** uStruct = mCflatTypeToStruct.Find(fwdType);
         if (!uStruct)
         {
            continue;
         }

         RegisteredInfo* regInfo = mRegisteredStructs.Find(*uStruct);
         if (mRegisteredStructs.Find(*uStruct))
         {
            fwdStructs.Append("\nstruct ");
            fwdStructs.Append(fwdType->mIdentifier.mName);
            fwdStructs.Append(";");
         }
         else if (mRegisteredClasses.Find(*uStruct))
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
      PerHeaderTypes& types = mTypesPerHeader[headerName];
      content.Append(types.mHeaderContent);

      const FString* modulePath = mPackagePaths.Find(types.mPackage);
      if (modulePath == nullptr)
      {
         continue;
      }

      FString headerPath = headerName.ToString();
      if (headerPath.IsEmpty() || headerPath.StartsWith(TEXT("Private/")))
      {
         continue;
      }

      FName moduleName = FPackageName::GetShortFName(types.mPackage->GetFName());

      if (TArray<FString>* pathsToIgnore = mModuleHeaderPathsToIgnore.Find(moduleName))
      {
         bool ignore = false;
         for (int i = 0; i < pathsToIgnore->Num(); ++i)
         {
            if (headerPath.Contains((*pathsToIgnore)[i]))
            {
               ignore = true;
               break;
            }
         }
         if (ignore)
         {
            continue;
         }
      }

      if (!headerPath.StartsWith(TEXT("Public/")))
      {
         if (modulePath->Contains("Source/Runtime/Engine"))
         {
            if (!headerPath.StartsWith(TEXT("Classes/")))
            {
               continue;
            }
         }
      }

      FString fullPath = (*modulePath) / headerPath;
      if (FPaths::FileExists(fullPath))
      {
         includeContent.Append(FString::Printf(TEXT("\n#include \"%s\""), *fullPath));
      }
   }

   content.Append("\n\n#endif // CFLAT_ENABLED");

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

void CallRegisteredTypeCallbacks(UStruct* pUStruct, const RegisteredInfo& pInfo, const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   Cflat::Struct* cfStruct = pInfo.mStruct;

   FName typeName(*UnrealModule::GetTypeNameAsString(cfStruct));
   TArray<FName> baseTypes;
   for (size_t i = 0; i < cfStruct->mBaseTypes.size(); ++i)
   {
      const Cflat::Type* baseType = cfStruct->mBaseTypes[i].mType;
      baseTypes.Add(FName(*UnrealModule::GetTypeNameAsString(baseType)));
   }

   if (pRegisteringCallbacks.RegisteredType)
   {
      pRegisteringCallbacks.RegisteredType(typeName, baseTypes);
   }

   if (pRegisteringCallbacks.RegisteredStruct)
   {
      pRegisteringCallbacks.RegisteredStruct(cfStruct, pUStruct);
   }
}

void CallRegisteredFunctionCallbacks(UStruct* pUStruct, const RegisteredInfo& pInfo, const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   Cflat::Struct* cfStruct = pInfo.mStruct;

   FName typeName(*UnrealModule::GetTypeNameAsString(cfStruct));

   TArray<FName> parameterNames;
   TArray<FName> parameterTypes;
   TArray<FString> parameterDefaultValues;

   const FString kEmptyString("");

   for (const RegisteredFunctionInfo& funcInfo : pInfo.mFunctions)
   {
      parameterNames.Empty(false);
      parameterTypes.Empty(false);
      parameterDefaultValues.Empty(false);

      FName funcName(funcInfo.mIdentifier.mName);
      bool hasDefaultParameter = funcInfo.mFirstDefaultParamIndex != -1;
      UFunction* func = funcInfo.mFunction;

      int32 propCount = 0;
      for (TFieldIterator<FProperty> propIt(func);
           propIt && propIt->HasAnyPropertyFlags(CPF_Parm) && !propIt->HasAnyPropertyFlags(CPF_ReturnParm);
           ++propIt, ++propCount)
      {
         FString parameterType = UnrealModule::GetTypeUsageAsString(funcInfo.mParameters[propCount]);
         parameterTypes.Add(FName(*parameterType));
         parameterNames.Add(propIt->GetFName());
         if (hasDefaultParameter)
         {
            if (propCount >= funcInfo.mFirstDefaultParamIndex)
            {
               FString metaDataName = FString::Printf(TEXT("CPP_Default_%s"), *propIt->GetName());
               const FString* metaDataValue = func->FindMetaData(*metaDataName);
               if (metaDataValue)
               {
                  parameterDefaultValues.Add(*metaDataValue);
               }
               else
               {
                  parameterDefaultValues.Add(kEmptyString);
               }
            }
            else
            {
               parameterDefaultValues.Add(kEmptyString);
            }
         }
      }

      if (funcInfo.mFunction->HasAnyFunctionFlags(FUNC_Static))
      {
         if (pRegisteringCallbacks.RegisteredFunction)
         {
            pRegisteringCallbacks.RegisteredFunction(func, typeName, funcName, parameterTypes, parameterNames, parameterDefaultValues);
         }
      }
      else
      {
         if (pRegisteringCallbacks.RegisteredMethod)
         {
            pRegisteringCallbacks.RegisteredMethod(func, typeName, funcName, parameterTypes, parameterNames, parameterDefaultValues);
         }
      }
   }
}

void CallRegisteringTypeCallbacks(const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   for (const auto& pair : mRegisteredStructs)
   {
      CallRegisteredTypeCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }
   for (const auto& pair : mRegisteredClasses)
   {
      CallRegisteredTypeCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }
   for (const auto& pair : mRegisteredInterfaces)
   {
      CallRegisteredTypeCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }

   if (pRegisteringCallbacks.RegisteredEnum)
   {
      for (const auto& pair : mRegisteredEnums)
      {
         pRegisteringCallbacks.RegisteredEnum(pair.Value.mEnum, pair.Key);
      }
   }

   if (pRegisteringCallbacks.RegisteredType)
   {
      // Global Namespace
      pRegisteringCallbacks.RegisteredType(NAME_None, {});
   }
}

void CallRegisteringFunctionsCallbacks(const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   if (!pRegisteringCallbacks.RegisteredFunction)
   {
      return;
   }

   for (const auto& pair : mRegisteredStructs)
   {
      CallRegisteredFunctionCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }
   for (const auto& pair : mRegisteredClasses)
   {
      CallRegisteredFunctionCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }
   for (const auto& pair : mRegisteredInterfaces)
   {
      CallRegisteredFunctionCallbacks(pair.Key, pair.Value, pRegisteringCallbacks);
   }

   // Cast
   pRegisteringCallbacks.RegisteredFunction(nullptr, NAME_None, FName("Cast"), {FName("UObject*")}, {FName("Src")}, {});
}


void AppendClassAndFunctionsForDebugging(UStruct* pStruct, const RegisteredInfo* pRegInfo, FString& pOutString)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;
   if (cfStruct == nullptr)
   {
      pOutString.Append(FString::Printf(TEXT("\n\nNOT FOUND: %s\n\n"), *pStruct->GetFullName()));
      return;
   }

   FString strMembers;
   for (size_t i = 0; i < cfStruct->mMembers.size(); ++i)
   {
      const Cflat::Member* member = &cfStruct->mMembers[i];
      strMembers.Append("\n\t");
      strMembers.Append(UnrealModule::GetMemberAsString(member));
      strMembers.Append(";");
   }

   FString strFunctions;
   {
      CflatSTLVector(Function*) functions;
      cfStruct->mFunctionsHolder.getAllFunctions(&functions);
      for (size_t i = 0; i < functions.size(); ++i)
      {
         const Cflat::Function* function = functions[i];
         strFunctions.Append("\n\t");
         strFunctions.Append(UnrealModule::GetFunctionAsString(function));
         strFunctions.Append(";");
      }
   }

   FString strMethods;
   {
      for (size_t i = 0; i < cfStruct->mMethods.size(); ++i)
      {
         const Cflat::Method* method = &cfStruct->mMethods[i];
         strMethods.Append("\n\t");
         strMethods.Append(UnrealModule::GetMethodAsString(method));
         strMethods.Append(";");
      }
   }

   FString strBaseClasses;
   for (int i = 0; i < cfStruct->mBaseTypes.size(); ++i)
   {
      if (i == 0)
      {
         strBaseClasses.Append(" : ");
      }
      else
      {
         strBaseClasses.Append(", ");
      }
      Cflat::Type* baseType = cfStruct->mBaseTypes[i].mType;
      uint16_t offset = cfStruct->mBaseTypes[i].mOffset;
      strBaseClasses.Append(baseType->mIdentifier.mName);
      strBaseClasses.Appendf(TEXT(" (offset: %d)"), (int)offset);
   }

   pOutString.Append("\n\n");
   pOutString.Append(cfStruct->mIdentifier.mName);
   pOutString.Append(strBaseClasses);
   pOutString.Append("\n");
   pOutString.Append(pStruct->GetFullName());
   pOutString.Append("\n");
   pOutString.Append("Header: ");
   pOutString.Append(pRegInfo->mHeader.ToString());
   pOutString.Append("\n");
   pOutString.Append("Properties:");
   pOutString.Append(strMembers);
   pOutString.Append("\n");
   pOutString.Append("Methods:");
   pOutString.Append(strMethods);
   pOutString.Append("\n");
   pOutString.Append("Functions:");
   pOutString.Append(strFunctions);
}

void PrintDebugStats()
{
   UE_LOG(LogTemp, Log, TEXT("[Cflat] AutoRegisterCflatTypes: total: %d time: %f"), 
          mRegisteredStructs.Num() + mRegisteredClasses.Num(),
          FPlatformTime::Seconds() - mTimeStarted);
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
      for (const auto& pair : mRegisteredStructs)
      {
         AppendClassAndFunctionsForDebugging(pair.Key, &pair.Value, addedStructs);
      }
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat][Registered UStructs]\n\n%s\n\n\n"), *addedStructs);

      FString addedClasses = {};
      for (const auto& pair : mRegisteredClasses)
      {
         AppendClassAndFunctionsForDebugging(pair.Key, &pair.Value, addedClasses);
      }
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat][Registered Classes]\n\n%s\n\n\n"), *addedClasses);

      FString addedInterfaces = {};
      for (const auto& pair : mRegisteredInterfaces)
      {
         AppendClassAndFunctionsForDebugging(pair.Key, &pair.Value, addedInterfaces);
      }
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat][Registered Interfaces]\n\n%s\n\n\n"), *addedInterfaces);
   }

   {
      TMap<FName, int32> moduleCount;

      for (const auto& pair : mRegisteredStructs)
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
      for (const auto& pair : mRegisteredClasses)
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

      int32 total = 0;
      struct ModuleCount
      {
         FName name;
         int32 count;
      };

      TArray<ModuleCount> sortedModuleCount;
      for (auto& it : moduleCount)
      {
         sortedModuleCount.Add({it.Key, it.Value});
         total += it.Value;
      }

      sortedModuleCount.Sort([](const ModuleCount& A, const ModuleCount& B) { return A.count > B.count; });

      FString modulesCountStr = TEXT("\n\nRegistered Types Per Module:\n\n");
      for (auto& it : sortedModuleCount)
      {
         modulesCountStr.Append(FString::Printf(TEXT("%s,%d\n"), *it.name.ToString(), it.count));
      }
      UE_LOG(LogTemp, Log, TEXT("%s\n\nTotal: %d"), *modulesCountStr, total);
   }
}

};

} // namespace AutoRegister

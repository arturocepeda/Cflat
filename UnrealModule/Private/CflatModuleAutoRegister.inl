
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
   FString mScriptName;
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
   TMap<Cflat::Type*, UStruct*> mCflatTypeToStruct;
   TMap<Cflat::Type*, UEnum*> mCflatTypeToEnum;
   TMap<Cflat::Type*, FName> mCflatTypeToHeader;
   TMap<FName, PerHeaderTypes> mTypesPerHeader;
   TSet<FName> mHeaderEnumsToIgnore;
   TSet<FName> mHeaderStructsToIgnore;
   TSet<FName> mHeaderClassesToIgnore;
   TSet<FName> mHeaderAlreadyIncluded;
   TSet<FName> mIgnoredTypes;
   TSet<Cflat::Type*> mForwardDeclartionTypes;
   float mTimeStarted; // For Debugging

   Cflat::Environment* mEnv = nullptr;

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
       pTypeName.FindLastChar(TCHAR('>'), templateIndexEnd))
   {
      FString typeBase = pTypeName.Mid(0, templateIndexBegin);
      FString typeTemplate = pTypeName.Mid(templateIndexBegin, templateIndexEnd);

      typeIsRegistered = IsCflatIdentifierRegistered(typeBase, typeTemplate);
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

   Cflat::Type* type = mEnv->getType(nameBuff);
   if (type)
   {
      return static_cast<Cflat::Struct*>(type);
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

   if (pStruct->GetBoolMetaData(kBlueprintType))
   {
      return true;
   }

   if (pStruct->GetBoolMetaData(kNotBlueprintType))
   {
      return false;
   }

   for (TFieldIterator<FProperty> propIt(pStruct, EFieldIterationFlags::None); propIt; ++propIt)
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

   for (TFieldIterator<UFunction> funcIt(pStruct, EFieldIterationFlags::None); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;

      if (!function->HasAnyFunctionFlags(FUNC_EditorOnly))
      {
         return true;
      }
   }

   return false;
}

bool GetFunctionParameters(UFunction* pFunction, Cflat::TypeUsage& pReturn, CflatSTLVector(Cflat::TypeUsage)& pParams, int& pOutFirstDefaultParamIndex)
{
   pOutFirstDefaultParamIndex = -1;

   char nameBuff[kCharConversionBufferSize];

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

   if (*header == pRegInfo->mHeader)
   {
      pRegInfo->mDependencies.Add(pType->mType);
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
      funcInfo.mScriptName = function->GetMetaData(kFunctionScriptName);

      if (funcInfo.mScriptName.IsEmpty() && funcInfo.mName.StartsWith(TEXT("K2_")))
      {
         funcInfo.mScriptName = funcInfo.mName;
         funcInfo.mScriptName.RemoveFromStart(TEXT("K2_"));
      }

      if (!funcInfo.mScriptName.IsEmpty())
      {
         if (pStruct->GetClass()->FindFunctionByName(FName(funcInfo.mScriptName)))
         {
            pOutFunctions.Pop(false);
            continue;
         }
      }

      funcInfo.mRegisteredIndex = count++;

      bool useScriptName = !funcInfo.mScriptName.IsEmpty() && funcInfo.mParameters.size() == 0;
      const FString& functionName = useScriptName ? funcInfo.mScriptName : funcInfo.mName;
      FPlatformString::Convert<TCHAR, ANSICHAR>(funcName, kCharConversionBufferSize, *functionName);
      funcInfo.mIdentifier = Cflat::Identifier(funcName);
   }
}

bool ContainsEquivalentNativeFuntion(const TArray<RegisteredFunctionInfo>& pFunctions, const RegisteredFunctionInfo& pFuncInfo)
{
   for (const RegisteredFunctionInfo& info : pFunctions)
   {
      if (!info.mScriptName.IsEmpty())
      {
         continue;
      }
      if (info.mRegisteredIndex == pFuncInfo.mRegisteredIndex)
      {
         continue;
      }
      if (info.mIdentifier != pFuncInfo.mIdentifier)
      {
         continue;
      }
      if (info.mParameters.size() != pFuncInfo.mParameters.size())
      {
         continue;
      }
      if (info.mParameters.size() == 0)
      {
         return true;
      }

      bool equals = true;
      for (int i = 0; i < info.mParameters.size(); ++i)
      {
         if (info.mParameters[i].mType != pFuncInfo.mParameters[i].mType)
         {
            equals = false;
            break;
         }
      }
      if (equals)
      {
         return true;
      }
   }

   return false;
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

   // Register Super Class/Struct
   {
      Cflat::Type* baseCflatType = nullptr;
      UStruct* superStruct = pStruct->GetSuperStruct();

      if (superStruct)
      {
         // Register base class/structure
         baseCflatType = RegisterUStruct(pRegisterMap, superStruct);
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
   regInfo.mIdentifier = cfStruct->mIdentifier;
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
   mCflatTypeToStruct.Add(cfStruct, pStruct);
   mCflatTypeToHeader.Add(cfStruct, regInfo.mHeader);

   return cfStruct;
}

void RegisterRegularEnum(UEnum* pUEnum)
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
   Cflat::Enum* cfEnum = mEnv->registerType<Cflat::Enum>(idEnumName);
   cfEnum->mSize = sizeof(int64);

   Cflat::Namespace* enumNameSpace = mEnv->requestNamespace(idEnumName);

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

      Cflat::Instance* instance = mEnv->setVariable(enumValue.mTypeUsage, idEnumValueName, enumValue);
      cfEnum->mInstances.push_back(instance);
      enumNameSpace->setVariable(enumValue.mTypeUsage, idEnumValueName, enumValue);
   }

   RegisteredEnumInfo& regInfo = mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData()->GetValue(pUEnum, TEXT("ModuleRelativePath"));
      regInfo.mHeader = FName(*modulePath);
   }
   mCflatTypeToEnum.Add(cfEnum, pUEnum);
   mCflatTypeToHeader.Add(cfEnum, regInfo.mHeader);
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
   cfEnum->mSize = sizeof(int64);

   Cflat::Namespace* enumNameSpace = mEnv->requestNamespace(idEnumName);

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

   RegisteredEnumInfo& regInfo = mRegisteredEnums.Add(pUEnum, {});
   regInfo.mEnum = cfEnum;
   {
      UPackage* package = pUEnum->GetPackage();
      const FString& modulePath = package->GetMetaData()->GetValue(pUEnum, TEXT("ModuleRelativePath"));
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
      UStruct* uStruct = static_cast<UStruct*>(*classIt);
      if (!CheckShouldRegisterType(uStruct))
      {
         continue;
      }

      RegisterUStruct(mRegisteredClasses, uStruct);
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
         else if (pClass->IsChildOf(sourceClass))
         {
            ptr = CflatValueAs(&pArguments[0], char*);
         }
      }

      pOutReturnValue->set(&ptr);
   };
}

void RegisterFunctions()
{
   const Cflat::TypeUsage uObjectTypeUsage = mEnv->getTypeUsage("UObject*");
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
      RegisterUStructFunctions(uStruct, &pair.Value);
      RegisterCastFromObject(uClass, cfStruct, uObjectTypeUsage);
   }
}

PerHeaderTypes* GetOrCreateHeaderType(UStruct* pStruct, TMap<FName, PerHeaderTypes>& pHeaders)
{
   RegisteredInfo* regInfo = mRegisteredStructs.Find(pStruct);
   if (regInfo == nullptr)
   {
      regInfo = mRegisteredClasses.Find(pStruct);
   }

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
    if (pUEnum->HasMetaData(TEXT("Bitflags")))
    {
      strEnum.Append(
          FString::Printf(TEXT("%s = 0x%08x"), *enumValueName, value));
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
      funcStr.Append(returnProperty->GetCPPType(&extendedType));
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

void AidHeaderAppendStruct(const UStruct* pUStruct, FString& pOutContent)
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


  // Manually extended methods/functinos
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

void AidHeaderAppendClass(const UStruct* pUStruct, FString& pOutContent)
{
  const RegisteredInfo* regInfo = mRegisteredClasses.Find(pUStruct);
  if (regInfo == nullptr)
  {
    return;
  }

  const UClass* uClass = static_cast<const UClass*>(pUStruct);
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

  // Manually extended methods/functinos
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

void AppendStructWithDependenciesRecursively(FName pHeader, PerHeaderTypes& pTypes, UStruct* pStruct, bool pIsClass)
{
   if (pTypes.mIncluded.Find(pStruct))
   {
      return;
   }

   if (pIsClass)
   {
      if (mHeaderClassesToIgnore.Find(pStruct->GetFName()))
      {
         return;
      }
   }
   else
   {
      if (mHeaderStructsToIgnore.Find(pStruct->GetFName()))
      {
         return;
      }
   }


   RegisteredInfo* regInfo = pIsClass ? mRegisteredClasses.Find(pStruct)
                                      : mRegisteredStructs.Find(pStruct);
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

      RegisteredInfo* depRegInfo = mRegisteredStructs.Find(*depUStruct);
      bool isClass = false;
      if (!depRegInfo)
      {
         depRegInfo = mRegisteredClasses.Find(*depUStruct);
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

      // Circular dependency. Forward declare it.
      if (depRegInfo->mDependencies.Find(regInfo->mStruct))
      {
         mForwardDeclartionTypes.Add(cfType);
         mForwardDeclartionTypes.Add(regInfo->mStruct);
      }

      AppendStructWithDependenciesRecursively(pHeader, pTypes, *depUStruct, isClass);
   }

   if (pIsClass)
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
   types->mHeaderContent = FString::Printf(TEXT("\n\n%s\n// %s\n%s"), *kHeaderSeparator, *pHeader.ToString(), *kHeaderSeparator);

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

      AppendStructWithDependenciesRecursively(pHeader, *types, uStruct, false);
   }

   for (UStruct* uStruct : types->mClasses)
   {
      if (mHeaderClassesToIgnore.Find(uStruct->GetFName()))
      {
         continue;
      }

      AppendStructWithDependenciesRecursively(pHeader, *types, uStruct, true);
   }

}

void GenerateAidHeader(const FString& pFilePath)
{
   FString content = "// Auto Generated From Auto Registered UClasses";
   content.Append("\n#pragma once");
   content.Append("\n#if defined (CFLAT_ENABLED)");

   FString includeContent = "// Auto Generated From Auto Registered UClasses";
   includeContent.Append("\n#pragma once");
   includeContent.Append("\n#if !defined (CFLAT_ENABLED)");

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
      if (headerName.IsNone())
      {
         continue;
      }

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

      if (!headerPath.StartsWith(TEXT("Public/")) && modulePath->Contains("Source/Runtime/Engine"))
      {
         continue;
      }

      FString fullPath = (*modulePath) / headerPath;
      if (FPaths::FileExists(fullPath))
      {
         includeContent.Append(FString::Printf(TEXT("\n#include \"%s\""), *fullPath));
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

void CallRegisteredTypeCallbacks(const RegisteredInfo& pInfo, const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   const Cflat::Struct* cfStruct = pInfo.mStruct;

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

void CallRegisteringCallbacks(const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks)
{
   for (const auto& pair : mRegisteredStructs)
   {
      CallRegisteredTypeCallbacks(pair.Value, pRegisteringCallbacks);
   }
   for (const auto& pair : mRegisteredClasses)
   {
      CallRegisteredTypeCallbacks(pair.Value, pRegisteringCallbacks);
   }

   if (pRegisteringCallbacks.RegisteredType)
   {
      // Global Namespace
      pRegisteringCallbacks.RegisteredType(NAME_None, {});
   }

   if (pRegisteringCallbacks.RegisteredFunction)
   {
      // Cast
      pRegisteringCallbacks.RegisteredFunction(nullptr, NAME_None, FName("Cast"), {FName("UObject*")}, {FName("Src")}, {});
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

   RegisteredInfo* regInfo = mRegisteredStructs.Find(pStruct);
   if (regInfo == nullptr)
   {
      regInfo = mRegisteredClasses.Find(pStruct);
   }

   pOutString.Append("\n\n");
   pOutString.Append(pStruct->GetFullName());
   pOutString.Append("\n");
   pOutString.Append("Header: ");
   pOutString.Append(regInfo->mHeader.ToString());
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
         AppendClassAndFunctionsForDebugging(pair.Key, addedStructs);
      }
      UE_LOG(LogTemp, Log, TEXT("\n\n[Cflat][Added UStructs]\n\n%s\n\n\n"), *addedStructs);

      FString addedClasses = {};
      for (const auto& pair : mRegisteredClasses)
      {
         AppendClassAndFunctionsForDebugging(pair.Key, addedClasses);
      }
      UE_LOG(LogTemp, Log, TEXT("%s"), *addedClasses);
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

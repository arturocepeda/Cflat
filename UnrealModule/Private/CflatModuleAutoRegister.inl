
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

static Cflat::Environment* gEnv = nullptr;


struct RegisteredInfo
{
   Cflat::Struct* mStruct;
   TArray<UFunction*> mFunctions;
   TArray<FProperty*> mProperties;
};

struct RegisterContext
{
   TSet<FName> mModulesToIgnore;
   TMap<UPackage*, bool> mIgnorePackageCache;
   TMap<UStruct*, RegisteredInfo> mRegisteredStructs;
   TMap<UStruct*, RegisteredInfo> mRegisteredClasses;
   TArray<UEnum*> mRegisteredEnumsInOrder;
   TArray<UStruct*> mRegisteredClassesInOrder;
   TArray<UStruct*> mRegisteredStructsInOrder;
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

      check(paramIndex < pArgs.size());
      size_t offset = property->GetOffset_ForUFunction();
      size_t size = pArgs[paramIndex].mTypeUsage.getSize();
      check(offset + size < kParamBuffMax);

      memcpy(&stack[offset], pArgs[paramIndex].mValueBuffer, size);

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
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *pTypeName);

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

   return false;
}

bool GetFunctionParameters(UFunction* pFunction, Cflat::TypeUsage& pReturn, CflatSTLVector(Cflat::TypeUsage)& pParams)
{
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
      }
      else
      {
         pParams.push_back(type);
      }
   }

   return true;
}

void RegisterUStructFunctions(UStruct* pStruct, RegisteredInfo* pRegInfo)
{
   Cflat::Struct* cfStruct = pRegInfo->mStruct;
   CflatSTLVector(Cflat::TypeUsage) parameters;

   for (TFieldIterator<UFunction> funcIt(pStruct); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;
      parameters.clear();
      Cflat::TypeUsage funcReturn = {};

      // Ignore Editor Only and Protected/Private
      if (function->HasAnyFunctionFlags(FUNC_Protected | FUNC_Private | FUNC_EditorOnly))
      {
         continue;
      }

      // Ignore Functionsnot Visible to Blueprints
      if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent))
      {
         continue;
      }

      if (!GetFunctionParameters(function, funcReturn, parameters))
      {
         continue;
      }

      pRegInfo->mFunctions.Push(function);

      FString functionName = function->HasMetaData(kFunctionScriptName) ? function->GetMetaData(kFunctionScriptName) : function->GetName();

      char funcName[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(funcName, kCharConversionBufferSize, *functionName);
      const Cflat::Identifier functionIdentifier(funcName);

      if (function->HasAnyFunctionFlags(FUNC_Static))
      {
         Cflat::Function* staticFunc = cfStruct->registerStaticMethod(functionIdentifier);
         staticFunc->mReturnTypeUsage = funcReturn;
         staticFunc->mParameters = parameters;

         staticFunc->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
         {
            UObject* context = function->GetOuterUClassUnchecked()->ClassDefaultObject;
            UObjFuncExecute(function, context, pArguments, pOutReturnValue);
         };
      }
      else
      {
         cfStruct->mMethods.push_back(Cflat::Method(functionIdentifier));
         Cflat::Method* method = &cfStruct->mMethods.back();
         method->mReturnTypeUsage = funcReturn;
         method->mParameters = parameters;

         method->execute = [function] (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
         {
            UObject* thisObj = CflatValueAs(&pThis, UObject*);
            UObjFuncExecute(function, thisObj, pArguments, pOutReturnValue);
         };
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
         UE_LOG(LogTemp, Log, TEXT("[Cflat] Default Constructor"));
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

      if (!propIt->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintAssignable | CPF_Edit))
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
   }
}

Cflat::Struct* RegisterUStruct(TMap<UStruct*, RegisteredInfo>& pRegisterMap, TArray<UStruct*>& pRegisteredList, UStruct* pStruct)
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
         baseCflatType = RegisterUStruct(pRegisterMap, pRegisteredList, superStruct);
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

   pContext.mRegisteredEnumsInOrder.Add(pUEnum);
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

   pContext.mRegisteredEnumsInOrder.Add(pUEnum);
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

      RegisterUStruct(pContext.mRegisteredStructs, pContext.mRegisteredStructsInOrder, uStruct);
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

      RegisterUStruct(pContext.mRegisteredClasses, pContext.mRegisteredClassesInOrder, uStruct);
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
      RegisterUStructFunctions(pair.Key, &pair.Value);
   }
   for (auto& pair : pContext.mRegisteredClasses)
   {
      // Register StaticClass method
      UStruct* uStruct = pair.Key;
      UStruct* uClass = static_cast<UClass*>(uStruct);
      Cflat::Struct* cfStruct = pair.Value.mStruct;
      {
         Cflat::Function* function = cfStruct->registerStaticMethod(staticClassIdentifier);
         function->mReturnTypeUsage = uClassTypeUsage;
         function->execute = [uClass](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
         {
            CflatAssert(pOutReturnValue);
            pOutReturnValue->set(&uClass);
         };
      }
      RegisterUStructFunctions(pair.Key, &pair.Value);
   }
}

void GenerateAidHeader(RegisterContext& pContext, const FString& pFilePath)
{
   const FString kSpacing = "   ";
   const FString kNewLineWithIndent1 = "\n   ";
   const FString kNewLineWithIndent2 = "\n      ";
   const Cflat::Identifier emptyId;

   FString content = "// Auto Generated From Auto Registered UClasses";
   content.Append("\n#pragma once");
   content.Append("\n#if defined (CFLAT_ENABLED)");

   // Enums
   for (const UEnum* uEnum : pContext.mRegisteredEnumsInOrder)
   {
      FString strEnum = "\n\n";
      UEnum::ECppForm enumForm = uEnum->GetCppForm();

      if (uEnum->HasMetaData(TEXT("Comment")))
      {
        strEnum.Append(uEnum->GetMetaData(TEXT("Comment")));
        strEnum.Append("\n");
      }

      FString declarationBegin = {};
      FString declarationEnd = {};
      FString newLineSpace = kNewLineWithIndent1;

      switch(enumForm)
      {
         case UEnum::ECppForm::Regular:
            declarationBegin = FString::Printf(TEXT("enum %s\n{"), *uEnum->GetName());
            declarationEnd = "\n};";
            break;
         case UEnum::ECppForm::Namespaced:
            declarationBegin = FString::Printf(TEXT("namespace %s\n{%senum Type%s{"), *uEnum->GetName(), *kNewLineWithIndent1, *kNewLineWithIndent1);
            declarationEnd = kNewLineWithIndent1 + "};\n}";
            newLineSpace = kNewLineWithIndent2;
            break;
         case UEnum::ECppForm::EnumClass:
            declarationBegin = FString::Printf(TEXT("enum class %s\n{"), *uEnum->GetName());
            declarationEnd = "\n};";
            break;
      }

      strEnum.Append(declarationBegin);

      for (int32 i = 0; i < uEnum->NumEnums() - 1; ++i)
      {
         int64 value = uEnum->GetValueByIndex(i);
         FString enumValueName = uEnum->GetNameStringByIndex(i);
         if (i > 0)
         {
            strEnum.Append(",");
         }
         strEnum.Append(newLineSpace);
         if (uEnum->HasMetaData(TEXT("Bitflags")))
         {
            strEnum.Append(FString::Printf(TEXT("%s = 0x%08x"), *enumValueName, value));
         }
         else
         {
            strEnum.Append(FString::Printf(TEXT("%s = %d"), *enumValueName, value));
         }
      }
      strEnum.Append(declarationEnd);
      content.Append(strEnum);
   }

   for (const UStruct* uStruct : pContext.mRegisteredStructsInOrder)
   {
      const RegisteredInfo* regInfo = pContext.mRegisteredStructs.Find(uStruct);
      if (regInfo == nullptr)
      {
         continue;
      }
      Cflat::Struct* cfStruct = regInfo->mStruct;

      FString strStruct = "\n\n";

      // Struct declaration
      {
        if (uStruct->HasMetaData(kMetaComment))
        {
          strStruct.Append(uStruct->GetMetaData(kMetaComment));
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
      for(size_t i = 0u; i < cfStruct->mMethods.size(); i++)
      {
         if(cfStruct->mMethods[i].mIdentifier == emptyId)
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
         if (prop->GetOwnerStruct() != uStruct)
         {
           continue;
         }

         // Ignore Protected/Private properties
         if (prop->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected | CPF_NativeAccessSpecifierPrivate))
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
         if (func->GetOwnerStruct() != uStruct)
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

         FString functionName = func->HasMetaData(kFunctionScriptName) ? func->GetMetaData(kFunctionScriptName) : func->GetName();
         funcStr.Append(functionName + "(");

         int32 propCount = 0;
         for (TFieldIterator<FProperty> propIt(func); propIt && propIt->HasAnyPropertyFlags(CPF_Parm) && !propIt->HasAnyPropertyFlags(CPF_ReturnParm); ++propIt, ++propCount)
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

      content.Append(strStruct);
   }


   for (const UStruct* uStruct : pContext.mRegisteredClassesInOrder)
   {
      const RegisteredInfo* regInfo = pContext.mRegisteredClasses.Find(uStruct);
      if (regInfo == nullptr)
      {
         continue;
      }
      const UClass* uClass = static_cast<const UClass*>(uStruct);
      Cflat::Struct* cfStruct = regInfo->mStruct;

      FString strClass = "\n\n";

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

         FString functionName = func->HasMetaData(kFunctionScriptName) ? func->GetMetaData(kFunctionScriptName) : func->GetName();
         funcStr.Append(functionName + "(");

         int32 propCount = 0;
         for (TFieldIterator<FProperty> propIt(func); propIt && propIt->HasAnyPropertyFlags(CPF_Parm) && !propIt->HasAnyPropertyFlags(CPF_ReturnParm); ++propIt, ++propCount)
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

      content.Append(strClass);
   }
   content.Append("\n#endif // CFLAT_ENABLED");

   if(!FFileHelper::SaveStringToFile(content, *pFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
   {
      UE_LOG(LogTemp, Error, TEXT("[Cflat] Could not write Aid Header File: %s"), *pFilePath);
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
   for (size_t i = 0; i < cfStruct->mMethods.size(); ++i)
   {
      const Cflat::Method& method = cfStruct->mMethods[i];
      strFunctions.Append("\n\t");
      strFunctions.Append(method.mIdentifier.mName);
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

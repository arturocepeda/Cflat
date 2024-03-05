
// UE includes - Source Code Navigation for getting module paths
#include "SourceCodeNavigation.h"

namespace AutoRegister
{
// Constants
static const FName kFunctionScriptName("ScriptName");
static const FName kMetaComment("Comment");
static const size_t kCharConversionBufferSize = 128u;

static Cflat::Environment* gEnv = nullptr;

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

struct RegisteredInfo
{
   Cflat::Class* mClass;
   TArray<UFunction*> mFunctions;
   TArray<FProperty*> mProperties;
};

struct RegisterContext
{
   TSet<FName> mModulesToIgnore;
   TMap<UPackage*, bool> mIgnorePackageCache;
   TMap<UClass*, RegisteredInfo> mRegisteredClasses;
   TArray<UClass*> mRegisteredClassesInOrder;
   float mTimeStarted; // For Debugging
};

bool IsCflatIdentifierRegistered(const char* TypeName)
{
   Cflat::Hash typeNameHash = Cflat::hash(TypeName);
   const Cflat::Identifier::NamesRegistry* registry = Cflat::Identifier::getNamesRegistry();

   return registry->mRegistry.find(typeNameHash) != registry->mRegistry.end();
}

bool IsCflatIdentifierRegistered(const FString& TypeName)
{
   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *TypeName);

   return IsCflatIdentifierRegistered(nameBuff);
}

bool IsCflatIdentifierRegistered(const FString& TypeName, const FString& ExtendedType)
{
   const bool typeIsRegistered = IsCflatIdentifierRegistered(TypeName);
   if (!typeIsRegistered)
   {
      return false;
   }

   if (ExtendedType.IsEmpty())
   {
      return typeIsRegistered;
   }

   if (ExtendedType.StartsWith(TEXT("<")))
   {
      const FRegexPattern pattern(TEXT("<(\\w+)>"));
      FRegexMatcher matcher(pattern, ExtendedType);
      if (matcher.FindNext())
      {
         FString substring = matcher.GetCaptureGroup(1); // Get the first captured group
         return IsCflatIdentifierRegistered(substring);
      }
   }
   else
   {
      return IsCflatIdentifierRegistered(ExtendedType);
   }

  return false;
}

Cflat::Class* GetCflatClassFromUClass(UClass* Class)
{
   const TCHAR* prefix = Class->GetPrefixCPP();
   FString className = FString::Printf(TEXT("%s%s"), prefix, *Class->GetName());

   char nameBuff[kCharConversionBufferSize];
   FPlatformString::Convert<TCHAR, ANSICHAR>(nameBuff, kCharConversionBufferSize, *className);

   if (!IsCflatIdentifierRegistered(nameBuff))
   {
     return nullptr;
   }

   Cflat::Type* type = gEnv->getType(nameBuff);
   if (type)
   {
      return static_cast<Cflat::Class*>(type);
   }
   return nullptr;
}

bool CheckShouldBindClass(RegisterContext& Context, UClass* Class)
{
   // Already registered
   if (Context.mRegisteredClasses.Contains(Class))
   {
      return false;
   }

   UPackage* classPackage = Class->GetPackage();
   if (!classPackage)
   {
      return false;
   }
   // Check if it is Editor Only type
   {
      bool ignoreModule = false;

      bool *cachedIgnore= Context.mIgnorePackageCache.Find(classPackage);

      if (cachedIgnore)
      {
         ignoreModule = *cachedIgnore;
      }
      else
      {
         FString modulePath;
         FName moduleName = FPackageName::GetShortFName(classPackage->GetFName());
         if (Context.mModulesToIgnore.Contains(moduleName))
         {
            ignoreModule = true;
         }
         else if(FSourceCodeNavigation::FindModulePath(classPackage, modulePath))
         {
            // Ignore Editor modules
            ignoreModule = moduleName.ToString().EndsWith(TEXT("Editor")) || modulePath.Contains(TEXT("/Editor/"));
         }
         else
         {
            ignoreModule = true;
         }
         Context.mIgnorePackageCache.Add(classPackage, ignoreModule);
      }

      if (ignoreModule)
      {
         return false;
      }
   }

   for (TFieldIterator<FProperty> propIt(Class); propIt; ++propIt)
   {
      if (propIt->HasAllPropertyFlags(CPF_NativeAccessSpecifierPublic) 
         && propIt->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintCallable) 
         && !propIt->HasAnyPropertyFlags(CPF_EditorOnly))
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

void RegisterUClassFunctions(UClass* pClass, RegisteredInfo* pRegInfo)
{
   Cflat::Class* cfClass = pRegInfo->mClass;
   CflatSTLVector(Cflat::TypeUsage) parameters;

   for (TFieldIterator<UFunction> funcIt(pClass); funcIt; ++funcIt)
   {
      UFunction* function = *funcIt;
      parameters.clear();
      Cflat::TypeUsage funcReturn = {};

      // Ignore Editor Only functions
      if (function->HasAnyFunctionFlags(FUNC_EditorOnly))
      {
         continue;
      }

      // Register only the ones that are publicly visible to blueprint
      if (!function->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public))
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
         Cflat::Function* staticFunc = cfClass->registerStaticMethod(functionIdentifier);
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
         cfClass->mMethods.push_back(Cflat::Method(functionIdentifier));
         Cflat::Method* method = &cfClass->mMethods.back();
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

void RegisterUClassProperties(UClass* pClass, RegisteredInfo* pRegInfo)
{
   Cflat::Class* cfClass = pRegInfo->mClass;
   for (TFieldIterator<FProperty> propIt(pClass); propIt; ++propIt)
   {
      FProperty* property = *propIt;

      // Register only the ones that are publicly visible to blueprint
      if (!property->HasAllPropertyFlags(CPF_BlueprintVisible | CPF_NativeAccessSpecifierPublic))
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
      cfClass->mMembers.push_back(member);

      pRegInfo->mProperties.Push(property);
   }
}

Cflat::Type* RegisterUClass(RegisterContext& pContext, UClass* pClass, bool pCheckShouldBind = true)
{
   if (pCheckShouldBind && !CheckShouldBindClass(pContext, pClass))
   {
      return nullptr;
   }

   // Early out if already registered
   {
      RegisteredInfo* regInfo = pContext.mRegisteredClasses.Find(pClass);
      if (regInfo)
      {
         return regInfo->mClass;
      }
   }

   Cflat::Type* type = nullptr;

   const TCHAR* prefix = pClass->GetPrefixCPP();
   FString className = FString::Printf(TEXT("%s%s"), prefix, *pClass->GetName());

   {
      char classTypeName[kCharConversionBufferSize];
      FPlatformString::Convert<TCHAR, ANSICHAR>(classTypeName, kCharConversionBufferSize, *className);
      const Cflat::Identifier classTypeIdentifier(classTypeName);
      type = gEnv->getType(classTypeIdentifier);
      if (type)
      {
         return type;
      }
      type = gEnv->registerType<Cflat::Class>(classTypeIdentifier);
   }

   type->mSize = sizeof(UClass);
   Cflat::Class* cfClass = static_cast<Cflat::Class*>(type);


   // Register BaseClass
   {
      Cflat::Type* baseCflatType = nullptr;
      UClass* baseClass = pClass->GetSuperClass();

      if (baseClass)
      {
         // Make sure the base class is registered
         baseCflatType = RegisterUClass(pContext, baseClass, false);
      }

      if (baseCflatType)
      {
         cfClass->mBaseTypes.emplace_back();
         Cflat::BaseType& baseType = cfClass->mBaseTypes.back();
         baseType.mType = baseCflatType;
         baseType.mOffset = 0u;
      }
   }

   // Register StaticClass method
   {
      Cflat::Function* function = cfClass->registerStaticMethod("StaticClass");
      function->mReturnTypeUsage = gEnv->getTypeUsage("UClass*");
      function->execute = [pClass](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue)
      {
         CflatAssert(pOutReturnValue);
         pOutReturnValue->set(&pClass);
      };
   }

   RegisteredInfo& regInfo = pContext.mRegisteredClasses.Add(pClass, {});
   regInfo.mClass = cfClass;
   pContext.mRegisteredClassesInOrder.Add(pClass);


   return type;
}

void RegisterClasses(RegisterContext& pContext)
{
   for (TObjectIterator<UClass> classIt; classIt; ++classIt)
   {
      AutoRegister::RegisterUClass(pContext, *classIt);
   }
}

void RegisterProperties(RegisterContext& pContext)
{
   for (auto& pair : pContext.mRegisteredClasses)
   {
      RegisterUClassProperties(pair.Key, &pair.Value);
   }
}

void RegisterFunctions(RegisterContext& pContext)
{
   for (auto& pair : pContext.mRegisteredClasses)
   {
      RegisterUClassFunctions(pair.Key, &pair.Value);
   }
}

void GenerateAidHeader(RegisterContext& pContext, const FString& pFilePath)
{
   const FString kSpacing = "   ";
   const FString kNewLineWithSpacing = "\n   ";

   FString content = "// Auto Generated From Auto Registered UClasses";
   content.Append("\n#pragma once");
   content.Append("\n#if defined (CFLAT_ENABLED)");

   for (const UClass* uClass : pContext.mRegisteredClassesInOrder)
   {
      const RegisteredInfo* regInfo = pContext.mRegisteredClasses.Find(uClass);
      if (regInfo == nullptr)
      {
         continue;
      }
      Cflat::Class* cfClass = regInfo->mClass;

      FString strClass = "\n\n";

      // Class declaration
      {
        if (uClass->HasMetaData(kMetaComment))
        {
          strClass.Append(uClass->GetMetaData(kMetaComment));
        }
        strClass.Append("\nclass ");
        strClass.Append(cfClass->mIdentifier.mName);

        // Base types
        if (cfClass->mBaseTypes.size() > 0)
        {
          strClass.Append(" :");
          for (size_t i = 0; i < cfClass->mBaseTypes.size(); ++i)
          {
            strClass.Append(" public ");
            strClass.Append(cfClass->mBaseTypes[i].mType->mIdentifier.mName);
            if (i < cfClass->mBaseTypes.size() - 1)
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
         FString propStr = kNewLineWithSpacing;

         if (prop->HasMetaData(kMetaComment))
         {
           propStr.Append(prop->GetMetaData(kMetaComment));
           propStr.Append(kNewLineWithSpacing);
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
      FString publicFuncStr = kNewLineWithSpacing + "static UClass* StaticClass();";

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

         FString funcStr = kNewLineWithSpacing;

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

void AppendClassAndFunctionsForDebugging(UClass* pClass, FString& pOutString)
{
   Cflat::Class* cfClass = GetCflatClassFromUClass(pClass);
   if (cfClass == nullptr)
   {
      pOutString.Append(FString::Printf(TEXT("%s\n\tNOT FOUND"), *pClass->GetFullName()));
      return;
   }

   FString strMembers;
   for (size_t i = 0; i < cfClass->mMembers.size(); ++i)
   {
      FString strFunc;
      const Cflat::Member& member = cfClass->mMembers[i];
      strMembers.Append("\n\t");
      strMembers.Append(member.mIdentifier.mName);
   }

   FString strFunctions;
   for (size_t i = 0; i < cfClass->mMethods.size(); ++i)
   {
      const Cflat::Method& method = cfClass->mMethods[i];
      strFunctions.Append("\n\t");
      strFunctions.Append(method.mIdentifier.mName);
   }

   pOutString.Append("\n\n");
   pOutString.Append(pClass->GetFullName());
   pOutString.Append("\n");
   pOutString.Append("Properties:");
   pOutString.Append(strMembers);
   pOutString.Append("\n");
   pOutString.Append("Functions:");
   pOutString.Append(strFunctions);
}

void PrintDebugStats(RegisterContext& pContext)
{
   UE_LOG(LogTemp, Log, TEXT("[Cflat] AutoRegisterCflatTypes: total: %d time: %f"), pContext.mRegisteredClasses.Num(), FPlatformTime::Seconds() - pContext.mTimeStarted);
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

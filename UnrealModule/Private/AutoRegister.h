#pragma once

#if defined(CFLAT_ENABLED)

#include "CoreMinimal.h"
#include "CflatModule.h"

namespace Cflat
{

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

   // For manually registered members
   int mMembersInitialCount;
   int mMethodInitialCount;
   int mFunctionInitialCount;

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

class AutoRegister
{
 public:
   AutoRegister(Cflat::Environment* pEnv);
   TMap<FName, TArray<FString>> mModuleHeaderPathsToIgnore;
   TSet<FName> mAllowedModules;
   TSet<FName> mIgnoredTypes;
   TSet<FName> mHeaderEnumsToIgnore;
   TSet<FName> mHeaderStructsToIgnore;
   TSet<FName> mHeaderClassesToIgnore;

   void RegisterEnums();
   void RegisterStructs();
   void RegisterClasses();
   void RegisterProperties();
   void RegisterFunctions();
   void RegisterTemplates();
   void RegisterSubsystems();
   void PrintDebugStats();
   void GenerateAidHeader(const FString& pFilePath);
   void
   CallRegisteringTypeCallbacks(const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks);
   void CallRegisteringFunctionsCallbacks(
       const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks);

 protected:
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
   TSet<FName> mHeaderAlreadyIncluded;
   TSet<Cflat::Type*> mForwardDeclartionTypes;
   double mTimeStarted; // For Debugging

   Cflat::Environment* mEnv = nullptr;
   Cflat::TypeUsage mUObjectTypeUsage;

   bool IsCflatIdentifierRegistered(const char* pTypeName);

   bool IsCflatIdentifierRegistered(const FString& pTypeName);

   bool IsCflatIdentifierRegistered(const FString& pTypeName, const FString& pExtendedType);

   Cflat::Struct* GetCflatStructFromUStruct(UStruct* pStruct);

   RegisteredInfo* FindRegisteredInfoFromUStruct(UStruct* pStruct);

   bool CheckShouldIgnoreModule(UPackage* pPackage);

   bool CheckShouldIgnoreHeaderPath(UStruct* pStruct);

   bool CheckShouldRegisterType(UStruct* pStruct);

   bool CheckShouldRegisterInterface(UClass* pInterface);

   bool GetFunctionParameters(
       UFunction* pFunction,
       Cflat::TypeUsage& pReturn,
       CflatSTLVector(Cflat::TypeUsage) & pParams,
       int& pOutFirstDefaultParamIndex);

   void RegisterCflatFunction(
       Cflat::Struct* pCfStruct,
       UFunction* pFunction,
       Cflat::Identifier pIdentifier,
       const CflatSTLVector(Cflat::TypeUsage) & pParameters,
       Cflat::TypeUsage pReturnType);

   void RegisterInterfaceFunction(
       Cflat::Struct* pCfStruct,
       UFunction* pFunction,
       Cflat::Identifier pIdentifier,
       const CflatSTLVector(Cflat::TypeUsage) & pParameters,
       Cflat::TypeUsage pReturnType);

   void AddDependencyIfNeeded(RegisteredInfo* pRegInfo, Cflat::TypeUsage* pTypeUsage);

   void GatherFunctionInfos(UStruct* pStruct, TArray<RegisteredFunctionInfo>& pOutFunctions);

   void RegisterUStructFunctions(UStruct* pStruct, RegisteredInfo* pRegInfo);

   void RegisterInterfaceFunctions(UStruct* pInterface, RegisteredInfo* pRegInfo);

   void RegisterUScriptStructConstructors(UScriptStruct* pStruct, RegisteredInfo* pRegInfo);

   void RegisterUStructProperties(UStruct* pStruct, RegisteredInfo* pRegInfo);

   Cflat::Struct* RegisterInterface(UStruct* pInterface);

   Cflat::Struct* RegisterUStruct(TMap<UStruct*, RegisteredInfo>& pRegisterMap, UStruct* pStruct);

   void RegisterRegularEnum(
       UEnum* pUEnum, const Cflat::Identifier& pEnumIdentifier, Cflat::Namespace* pNamespace);

   void RegisterRegularEnum(UEnum* pUEnum);

   void RegisterEnumNamespaced(UEnum* pUEnum);

   void RegisterEnumClass(UEnum* pUEnum);

   void RegisterCastFromObject(
       UClass* pClass, Cflat::Struct* pCfStruct, const Cflat::TypeUsage& pParamTypeUsage);

   template <typename BaseSubsystemType, typename OwnerType>
   void RegisterSubsystem(Cflat::Class* pCfOwnerType, UClass* pClass, Cflat::Struct* pCfStruct);

   void RegisterTemplatedObjectPtr(UStruct* pStruct, Cflat::Struct* pCfStruct);

   void RegisterTemplatedWeakObjectPtr(UStruct* pStruct, Cflat::Struct* pCfStruct);

   void
   RegisterTemplatedSubclassOf(Cflat::Struct* pCfStruct, const Cflat::TypeUsage* pUClassTypeUsage);

   PerHeaderTypes* GetOrCreateHeaderType(UStruct* pStruct, TMap<FName, PerHeaderTypes>& pHeaders);

   PerHeaderTypes* GetOrCreateHeaderType(UEnum* pEnum, TMap<FName, PerHeaderTypes>& pHeaders);

   PerHeaderTypes* GetOrCreateHeaderType(FName pHeader, TMap<FName, PerHeaderTypes>& pHeaders);

   void MapTypesPerHeaders();

   void AidHeaderAppendEnum(const UEnum* pUEnum, FString& pOutContent);

   FString
   FunctionInfoToString(const RegisteredFunctionInfo& pInfo, int pDefaultParameterIndex = -1);

   FString FunctionInfoInterfaceWrapperToString(
       const RegisteredFunctionInfo& pInfo, int pDefaultParameterIndex = -1);

   void AidHeaderAppendNonAutoRegisteredMembers(const RegisteredInfo* pInfo, FString& pOutString);

   void AppendCflatMethodsAsString(const Cflat::Method* pMethod, FString& pOutString);

   void AidHeaderAppendNonAutoRegisteredMethods(const RegisteredInfo* pInfo, FString& pOutString);

   void AidHeaderAppendNonAutoRegisteredFunctions(const RegisteredInfo* pInfo, FString& pOutString);

   void AidHeaderAppendStruct(UStruct* pUStruct, FString& pOutContent);

   void AidHeaderAppendInterface(UClass* pUClass, FString& pOutContent);

   void AidHeaderAppendClass(UStruct* pUStruct, FString& pOutContent);

   void
   AppendStructWithDependenciesRecursively(FName pHeader, PerHeaderTypes& pTypes, UStruct* pStruct);

   void CreateHeaderContent(FName pHeader, TArray<FName>& pHeaderIncludeOrder);

   void CallRegisteredTypeCallbacks(
       UStruct* pUStruct,
       const RegisteredInfo& pInfo,
       const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks);

   void CallRegisteredFunctionCallbacks(
       UStruct* pUStruct,
       const RegisteredInfo& pInfo,
       const UnrealModule::RegisteringCallbacks& pRegisteringCallbacks);

   void AppendClassAndFunctionsForDebugging(
       UStruct* pStruct, const RegisteredInfo* pRegInfo, FString& pOutString);
};

}; // namespace Cflat

#endif // CFLAT_ENABLED

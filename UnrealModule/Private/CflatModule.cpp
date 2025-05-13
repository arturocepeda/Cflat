
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


#if defined (CFLAT_ENABLED)

#include "CflatModule.h"

// Cflat source code
#include "../../Cflat.cpp"
#include "../../CflatHelper.h"

// Standard includes
#include <mutex>

// UE includes - Module manager
#include "Modules/ModuleManager.h"

// UE includes - File watcher
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"

// UE includes - Editor notifications
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

// UE includes - Engine types
#include "CoreMinimal.h"
#include "Components/LineBatchComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"

// Auto Register
#include "CflatModuleAutoRegister.inl"

//
//  Constants
//
static const float kEditorNotificationDuration = 3.0f;

//
//  Module implementation
//
IMPLEMENT_MODULE(FDefaultModuleImpl, Cflat);



//
//  Global instances
//
static Cflat::Environment gEnv;
static std::mutex gLock;

static AutoRegister::TypesRegister* gAutoRegister;
static Cflat::UnrealModule::RegisteringCallbacks gRegisteringCallbacks = {0};

#define CallbackRegisterType(pTypeName) \
   if (gRegisteringCallbacks.RegisteredType) \
   { \
      Cflat::TypeUsage typeUsage = gEnv.getTypeUsage(#pTypeName); \
      const Cflat::Type* registeredType = typeUsage.mType; \
      if (registeredType && registeredType->mCategory == TypeCategory::StructOrClass) \
      { \
         const Cflat::Struct* cfStruct = static_cast<const Cflat::Struct*>(registeredType); \
         FName typeName(*UnrealModule::GetTypeNameAsString(cfStruct)); \
         TArray<FName> baseTypes; \
         for (size_t i = 0; i < cfStruct->mBaseTypes.size(); ++i) \
         { \
            const Cflat::Type* baseType = cfStruct->mBaseTypes[i].mType; \
            baseTypes.Add(FName(*UnrealModule::GetTypeNameAsString(baseType))); \
         } \
         gRegisteringCallbacks.RegisteredType(typeName, baseTypes); \
      } \
   }
#define CallbackRegisterBaseStructureType(pTypeName) \
   if (gRegisteringCallbacks.RegisteredStruct) \
   { \
      Cflat::Type* registeredType = gEnv.getType(#pTypeName); \
      if (registeredType && registeredType->mCategory == TypeCategory::StructOrClass) \
      { \
         Cflat::Struct* cfStruct = static_cast<Cflat::Struct*>(registeredType); \
         gRegisteringCallbacks.RegisteredStruct(cfStruct, TBaseStructure<pTypeName>::Get()); \
      } \
   }
#define CallbackRegisterMethod(pType, pMethod) \
   if (gRegisteringCallbacks.ManuallyRegisteredMethod) \
   { \
      gRegisteringCallbacks.ManuallyRegisteredMethod(FName(#pType), #pMethod); \
   }
#define CallbackRegisterFunction(pType, pFunction) \
   if (gRegisteringCallbacks.ManuallyRegisteredFunction) \
   { \
      gRegisteringCallbacks.ManuallyRegisteredFunction(FName(#pType), #pFunction); \
   }
#define CallbackRegisterGlobalFunction(pFunction) \
   if (gRegisteringCallbacks.ManuallyRegisteredFunction) \
   { \
      gRegisteringCallbacks.ManuallyRegisteredFunction(NAME_None, #pFunction); \
   }

#define CallbackRegisterTArray(T) \
   CallbackRegisterType(TArray<T>); \
   CallbackRegisterMethod(TArray<T>, bool IsEmpty() const); \
   CallbackRegisterMethod(TArray<T>, int32 Num() const); \
   CallbackRegisterMethod(TArray<T>, void Reserve(int32 Number)); \
   CallbackRegisterMethod(TArray<T>, void SetNum(int32 NewNum)); \
   CallbackRegisterMethod(TArray<T>, void SetNumZeroed(int32 NewNum)); \
   CallbackRegisterMethod(TArray<T>, void SetNumUninitialized(int32 NewNum)); \
   CallbackRegisterMethod(TArray<T>, void Empty(int32 Slack = 0)); \
   CallbackRegisterMethod(TArray<T>, void Add(const T& Item)); \
   CallbackRegisterMethod(TArray<T>, void RemoveAt(int32 Index));
#define CallbackRegisterTSet(T) \
   CallbackRegisterType(TSet<T>); \
   CallbackRegisterMethod(TSet<T>, void Empty()); \
   CallbackRegisterMethod(TSet<T>, bool IsEmpty() const); \
   CallbackRegisterMethod(TSet<T>, int32 Num() const); \
   CallbackRegisterMethod(TSet<T>, void Add(const T& InElement)); \
   CallbackRegisterMethod(TSet<T>, T* Find(const T& Key)); \
   CallbackRegisterMethod(TSet<T>, int32 Remove(const T& Key)); \
   CallbackRegisterMethod(TSet<T>, bool Contains(const T& Key) const); \

//
//  CflatGlobal implementations
//
namespace CflatGlobal
{
Cflat::Environment* getEnvironment()
{
   return &gEnv;
}

void lockEnvironment()
{
   gLock.lock();
}

void unlockEnvironment()
{
   gLock.unlock();
}

void onError(const char* pErrorMessage)
{
   const FString errorMessage(gEnv.getErrorMessage());
   UE_LOG(LogTemp, Error, TEXT("[Cflat] %s"), *errorMessage);
}
}


//
//  UnrealModule functions
//
namespace Cflat
{
TArray<UnrealModule::OnScriptReloadedCallbackEntry> UnrealModule::smOnScriptReloadedCallbacks;
TArray<UnrealModule::OnScriptReloadFailedEntry> UnrealModule::smOnScriptReloadFailedCallbacks;

void ShowNotification(bool pSuccess, const FString& pTitle, const FString& pText)
{
   FNotificationInfo notifyInfo(FText::FromString(pTitle));
   notifyInfo.SubText = FText::FromString(pText);
   notifyInfo.ExpireDuration = kEditorNotificationDuration;
   TSharedPtr<SNotificationItem> notification = FSlateNotificationManager::Get().AddNotification(notifyInfo);
   notification->SetCompletionState(pSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
}

void UELogImpl(uint8_t pCategory, uint8_t pVerbosity, const wchar_t* pFormat, const Cflat::Value* pVariadicArgs, size_t pVariadicArgsCount)
{
   (void)pCategory;
   const size_t kBufferSize = 512;
   wchar_t buffer[kBufferSize];
   buffer[kBufferSize - 1] = L'\0';
   Cflat::Helper::snwprintfFunction(buffer, kBufferSize - 1, pFormat, pVariadicArgs, pVariadicArgsCount);

   UE::Logging::Private::FStaticBasicLogDynamicData logData;
   UE::Logging::Private::FStaticBasicLogRecord logRecord
   (
      buffer,
      __FILE__,
      __LINE__,
      (ELogVerbosity::Type)pVerbosity,
      logData
   );

   UE::Logging::Private::BasicLog(LogTemp, &logRecord);
}

void UELogExecute(const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
{
   (void)pOutReturnValue;
   const size_t kFixedArgsCount = 3u;
   const size_t variadicArgsCount = pArgs.size() - kFixedArgsCount;

   const Cflat::Value* variadicArgs = variadicArgsCount > 0 ? &pArgs[kFixedArgsCount] : nullptr;

   UELogImpl
   (
      CflatValueAs(&pArgs[0], uint8_t),
      CflatValueAs(&pArgs[1], uint8_t),
      CflatValueAs(&pArgs[2], const wchar_t*),
      variadicArgs,
      variadicArgsCount
   );
}

void UnrealModule::AutoRegisterCflatTypes(const TSet<FName>& pModules, const TSet<FName>& pIgnoredTypes, void (*pRegisterComplementaryTypesCallback)(void))
{
   gAutoRegister = new AutoRegister::TypesRegister(&gEnv);

   gAutoRegister->mAllowedModules = pModules;
   gAutoRegister->mIgnoredTypes = pIgnoredTypes;

   // These are typedefd or manually registered
   gAutoRegister->mHeaderEnumsToIgnore =
   {
      FName("ETeleportType"),
      FName("ESpawnActorCollisionHandlingMethod"),
      FName("ESpawnActorScaleMethod")
   };
   gAutoRegister->mHeaderStructsToIgnore =
   {
      FName("HitResult")
   };
   gAutoRegister->mHeaderClassesToIgnore =
   {
      FName("Object"),
      FName("Field"),
      FName("Struct"),
      FName("Class"),
      FName("Interface")
   };

   gAutoRegister->RegisterEnums();
   gAutoRegister->RegisterStructs();
   gAutoRegister->RegisterClasses();

   {
      CflatRegisterTSubclassOf(&gEnv, UObject);
      CflatRegisterTSubclassOf(&gEnv, UActorComponent);
      CflatRegisterTSubclassOf(&gEnv, AActor);
      CflatRegisterTSubclassOf(&gEnv, AController);
      CflatRegisterTSubclassOf(&gEnv, APawn);

      CflatRegisterTArray(&gEnv, AActor*);

      CallbackRegisterTArray(AActor*);

      if (pRegisterComplementaryTypesCallback)
      {
         pRegisterComplementaryTypesCallback();
      }
   }

   gAutoRegister->RegisterProperties();
   gAutoRegister->RegisterFunctions();
   gAutoRegister->RegisterSubsystems();

   gAutoRegister->CallRegisteringCallbacks(gRegisteringCallbacks);
}

void UnrealModule::SetRegisteringCallbacks(const RegisteringCallbacks& pRegisteringCallbacks)
{
   gRegisteringCallbacks = pRegisteringCallbacks;
}

const UnrealModule::RegisteringCallbacks& UnrealModule::GetRegisteringCallbacks()
{
   return gRegisteringCallbacks;
}

void UnrealModule::GenerateAidHeaderFile()
{
   const FString aidFileDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts");
   gAutoRegister->GenerateAidHeader(aidFileDir);


   const bool printDebug = false;
   if (printDebug)
   {
      gAutoRegister->PrintDebugStats();
   }

   delete gAutoRegister;
}

void UnrealModule::RegisterOnScriptReloadedCallback(UObject* pOwner, OnScriptReloadedCallback pCallback)
{
   smOnScriptReloadedCallbacks.Emplace();
   OnScriptReloadedCallbackEntry& entry = smOnScriptReloadedCallbacks.Last();
   entry.mOwner = pOwner;
   entry.mCallback = pCallback;
}

void UnrealModule::DeregisterOnScriptReloadedCallbacks(UObject* pOwner)
{
   for(int32 i = 0; i < smOnScriptReloadedCallbacks.Num(); )
   {
      if(smOnScriptReloadedCallbacks[i].mOwner == pOwner)
      {
         smOnScriptReloadedCallbacks.RemoveAt(i);
      }
      else
      {
         i++;
      }
   }
}

void UnrealModule::RegisterOnScriptReloadFailedCallback(UObject* pOwner, OnScriptReloadFailedCallback pCallback)
{
   smOnScriptReloadFailedCallbacks.Emplace();
   OnScriptReloadFailedEntry& entry = smOnScriptReloadFailedCallbacks.Last();
   entry.mOwner = pOwner;
   entry.mCallback = pCallback;
}

void UnrealModule::DeregisterOnScriptReloadFailedCallback(UObject* pOwner)
{
   for(int32 i = 0; i < smOnScriptReloadFailedCallbacks.Num(); )
   {
      if(smOnScriptReloadFailedCallbacks[i].mOwner == pOwner)
      {
         smOnScriptReloadFailedCallbacks.RemoveAt(i);
      }
      else
      {
         i++;
      }
   }
}


void UnrealModule::RegisterTypes()
{
   {
      CflatRegisterTObjectPtr(&gEnv, UObject);
   }
   {
      CflatRegisterStruct(&gEnv, FSoftObjectPtr);
      CflatStructAddConstructor(&gEnv, FSoftObjectPtr);
      CflatStructAddConstructorParams1(&gEnv, FSoftObjectPtr, const UObject*);
      CflatStructAddConstructorParams1(&gEnv, FSoftObjectPtr, TObjectPtr<UObject>);
      CflatStructAddDestructor(&gEnv, FSoftObjectPtr);
      CflatStructAddMethodReturn(&gEnv, FSoftObjectPtr, UObject*, LoadSynchronous) CflatMethodConst;
      CflatStructAddMethodReturn(&gEnv, FSoftObjectPtr, UObject*, Get) CflatMethodConst;
   }
   {
      CflatRegisterTArray(&gEnv, UObject*);
      CflatRegisterTArray(&gEnv, UClass*);
      CflatRegisterTArray(&gEnv, FHitResult);
      CflatRegisterTArray(&gEnv, UActorComponent*);
      CflatRegisterTArray(&gEnv, TObjectPtr<UObject>);
      CflatRegisterTArray(&gEnv, FSoftObjectPtr);

      CallbackRegisterTArray(UObject*);
      CallbackRegisterTArray(UClass*);
      CallbackRegisterTArray(FHitResult);
      CallbackRegisterTArray(UActorComponent*);
   }
   {
      CflatRegisterTSet(&gEnv, UActorComponent*);

      CallbackRegisterTSet(UActorComponent*);
   }
   // Global
   {
      CflatRegisterFunctionReturnParams1(&gEnv, bool, IsValid, UObject*);
      CflatRegisterTemplateFunctionReturnParams2(&gEnv, UObject, UObject*, LoadObject, UObject*, const TCHAR*);

      // Callbacks for manually registered types
      CallbackRegisterGlobalFunction(bool IsValid(UObject* Test));
      CallbackRegisterGlobalFunction("template<class T> T* LoadObject(UObject* Outer, const TCHAR* Name)");
   }
   {
      // UObject - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("UObject"));
      CflatClassAddMethodReturn(&gEnv, UObject, UClass*, GetClass) CflatMethodConst;
      CflatClassAddMethodReturn(&gEnv, UObject, FString, GetName) CflatMethodConst;
      CflatClassAddMethodReturn(&gEnv, UObject, FName, GetFName) CflatMethodConst;
      CflatClassAddMethodReturn(&gEnv, UObject, UWorld*, GetWorld) CflatMethodConst;

      // Callbacks for manually registered types
      CallbackRegisterMethod(UObject, UClass* GetClass() const);
      CallbackRegisterMethod(UObject, FString GetName() const);
      CallbackRegisterMethod(UObject, FName GetFName() const);
      CallbackRegisterMethod(UObject, UWorld* GetWorld() const);
   }
   {
      // UClass - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("UClass"));
      CflatClassAddMethodReturn(&gEnv, UClass, UObject*, GetDefaultObject) CflatMethodConst;
      CflatClassAddMethodReturnParams1(&gEnv, UClass, UObject*, GetDefaultObject, bool) CflatMethodConst;

      // Callbacks for manually registered types
      CallbackRegisterMethod(UClass, UObject* GetDefaultObject(bool bCreateIfNeeded = true) const);
   }
   {
      // AActor - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("AActor"));
      CflatClassAddMethodReturn(&gEnv, AActor, UWorld*, GetWorld) CflatMethodConst;
      CflatClassAddMethodReturn(&gEnv, AActor, FQuat, GetActorQuat) CflatMethodConst;
      CflatClassAddMethodVoidParams1(&gEnv, AActor, void, GetComponents, TArray<UActorComponent*>&) CflatMethodConst;
      CflatClassAddMethodVoidParams2(&gEnv, AActor, void, GetComponents, TArray<UActorComponent*>&, bool)  CflatMethodConst;
      CflatClassAddMethodReturn(&gEnv, AActor, const TSet<UActorComponent*>&, GetComponents) CflatMethodConst;

      // Function overrides with default parameters
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorLocation, const FVector&);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorRotation, FRotator);

      // Callbacks for manually registered types
      CallbackRegisterMethod(AActor, void GetComponents(TArray<UActorComponent*>& OutComponents, bool bIncludeFromChildActors = false));
      CallbackRegisterMethod(AActor, TSet<UActorComponent*>& GetComponents());
      CallbackRegisterMethod(AActor, bool SetActorLocation(const FVector& NewLocation));
      CallbackRegisterMethod(AActor, bool SetActorRotation(FRotator NewRotation));
   }
   {
      // ULineBatchComponent - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("ULineBatchComponent"));
      CflatClassAddMethodVoidParams6(&gEnv, ULineBatchComponent, void, DrawBox, const FVector&, const FVector&, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawBox, const FVector&, const FVector&, const FQuat&, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams6(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8, float, float);
      CflatClassAddMethodVoidParams5(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8, float);
      CflatClassAddMethodVoidParams4(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawDirectionalArrow, const FVector&, const FVector&, float, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawCircle, const FVector&, const FVector&, const FVector&, FLinearColor, float, int32, uint8);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawSphere, const FVector&, float, int32, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams8(&gEnv, ULineBatchComponent, void, DrawCapsule, const FVector&, float, float, const FQuat&, FLinearColor, float, uint8, float);

      // Callbacks for manually registered types
      CallbackRegisterMethod(ULineBatchComponent, void DrawBox(const FVector& Center, const FVector& Box, FLinearColor Color, float LifeTime, uint8 DepthPriority, float Thickness));
      CallbackRegisterMethod(ULineBatchComponent, void DrawBox(const FVector& Center, const FVector& Box, const FQuat& Rotation, FLinearColor Color, float LifeTime, uint8 DepthPriority, float Thickness));
      CallbackRegisterMethod(ULineBatchComponent, void DrawLine(const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriority, float Thickness, float LifeTime));
      CallbackRegisterMethod(ULineBatchComponent, void DrawLine(const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriority, float Thickness));
      CallbackRegisterMethod(ULineBatchComponent, void DrawLine(const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriority));
   }
   {
      CflatRegisterTObjectPtr(&gEnv, ULineBatchComponent);
   }

   {
      CflatRegisterTypeAlias(&gEnv, uint8, FMaskFilter);
   }
   {
      CflatRegisterStruct(&gEnv, FCollisionObjectQueryParams);
      CflatStructAddConstructor(&gEnv, FCollisionObjectQueryParams);
      CflatStructAddMember(&gEnv, FCollisionObjectQueryParams, int32, ObjectTypesToQuery);
      CflatStructAddMember(&gEnv, FCollisionObjectQueryParams, FMaskFilter, IgnoreMask);
      CflatStructAddMethodVoidParams1(&gEnv, FCollisionObjectQueryParams, void, AddObjectTypesToQuery, ECollisionChannel);
      CflatStructAddMethodVoidParams1(&gEnv, FCollisionObjectQueryParams, void, RemoveObjectTypesToQuery, ECollisionChannel);

      // Callbacks for manually registered types
      CallbackRegisterType(FCollisionObjectQueryParams);
      CallbackRegisterMethod(FCollisionObjectQueryParams, void AddObjectTypesToQuery(ECollisionChannel QueryChannel));
      CallbackRegisterMethod(FCollisionObjectQueryParams, void RemoveObjectTypesToQuery(ECollisionChannel QueryChannel));
   }
   {
      CflatRegisterEnumClass(&gEnv, EQueryMobilityType);
      CflatEnumClassAddValue(&gEnv, EQueryMobilityType, Any);
      CflatEnumClassAddValue(&gEnv, EQueryMobilityType, Static);
      CflatEnumClassAddValue(&gEnv, EQueryMobilityType, Dynamic);
   }
   {
      CflatRegisterStruct(&gEnv, FCollisionQueryParams);
      CflatStructAddConstructor(&gEnv, FCollisionQueryParams);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, FName, TraceTag);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, FName, OwnerTag);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bTraceComplex);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bFindInitialOverlaps);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bReturnFaceIndex);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bReturnPhysicalMaterial);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bIgnoreBlocks);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bIgnoreTouches);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bSkipNarrowPhase);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, bool, bTraceIntoSubComponents);
      CflatStructAddMember(&gEnv, FCollisionQueryParams, EQueryMobilityType, MobilityType);
      CflatStructAddMethodVoidParams1(&gEnv, FCollisionQueryParams, void, AddIgnoredActor, const AActor*);
      CflatStructAddStaticMember(&gEnv, FCollisionQueryParams, FCollisionQueryParams, DefaultQueryParam);

      // Callbacks for manually registered types
      CallbackRegisterType(FCollisionQueryParams);
      CallbackRegisterMethod(FCollisionQueryParams, void AddIgnoredActor(const AActor* InIgnoreActor));
   }
   {
      // UWorld - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("UWorld"));
      CflatClassAddMember(&gEnv, UWorld, TObjectPtr<ULineBatchComponent>, LineBatcher);
      CflatClassAddMethodReturnParams4(&gEnv, UWorld, bool, LineTraceSingleByChannel, FHitResult&, const FVector&, const FVector&, ECollisionChannel);
      CflatClassAddMethodReturnParams5(&gEnv, UWorld, bool, LineTraceSingleByChannel, FHitResult&, const FVector&, const FVector&, ECollisionChannel, const FCollisionQueryParams&);
      CflatClassAddMethodReturnParams4(&gEnv, UWorld, bool, LineTraceSingleByObjectType, FHitResult&, const FVector&, const FVector&, const FCollisionObjectQueryParams&);
      CflatClassAddMethodReturnParams5(&gEnv, UWorld, bool, LineTraceSingleByObjectType, FHitResult&, const FVector&, const FVector&, const FCollisionObjectQueryParams&, const FCollisionQueryParams&);
      CflatClassAddMethodReturnParams4(&gEnv, UWorld, bool, LineTraceMultiByChannel, TArray<FHitResult>&, const FVector&, const FVector&, ECollisionChannel);
      CflatClassAddMethodReturnParams5(&gEnv, UWorld, bool, LineTraceMultiByChannel, TArray<FHitResult>&, const FVector&, const FVector&, ECollisionChannel, const FCollisionQueryParams&);
      CflatClassAddMethodReturnParams4(&gEnv, UWorld, bool, LineTraceMultiByObjectType, TArray<FHitResult>&, const FVector&, const FVector&, const FCollisionObjectQueryParams&);
      CflatClassAddMethodReturnParams5(&gEnv, UWorld, bool, LineTraceMultiByObjectType, TArray<FHitResult>&, const FVector&, const FVector&, const FCollisionObjectQueryParams&, const FCollisionQueryParams&);
      CflatClassAddMethodReturnParams1(&gEnv, UWorld, AActor*, SpawnActor, UClass*);
      CflatClassAddMethodReturnParams2(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FVector*);
      CflatClassAddMethodReturnParams3(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FVector*, const FRotator*);
      //CflatClassAddMethodReturnParams4(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FVector*, const FRotator*, const FActorSpawnParameters&);
      CflatClassAddMethodReturnParams2(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FTransform*);
      //CflatClassAddMethodReturnParams3(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FTransform*, const FActorSpawnParameters&);
      CflatClassAddMethodReturnParams2(&gEnv, UWorld, AActor*, SpawnActorAbsolute, UClass*, const FTransform&);
      //CflatClassAddMethodReturnParams3(&gEnv, UWorld, AActor*, SpawnActorAbsolute, UClass*, const FTransform&, const FActorSpawnParameters&);
      CflatClassAddTemplateMethodReturnParams2(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&);
      CflatClassAddTemplateMethodReturnParams3(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*);
      CflatClassAddTemplateMethodReturnParams4(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*);
      CflatClassAddTemplateMethodReturnParams5(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*, ESpawnActorCollisionHandlingMethod);
      CflatClassAddTemplateMethodReturnParams6(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*, ESpawnActorCollisionHandlingMethod, ESpawnActorScaleMethod);
      CflatClassAddMethodReturnParams1(&gEnv, UWorld, bool, DestroyActor, AActor*);


      // Callbacks for manually registered types
      CallbackRegisterMethod(UWorld, bool LineTraceSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel));
      CallbackRegisterMethod(UWorld, bool LineTraceSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params));
      CallbackRegisterMethod(UWorld, bool LineTraceSingleByObjectType(FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams));
      CallbackRegisterMethod(UWorld, bool LineTraceSingleByObjectType(FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params));
      CallbackRegisterMethod(UWorld, bool LineTraceMultiByChannel(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params));
      CallbackRegisterMethod(UWorld, bool LineTraceMultiByChannel(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel));
      CallbackRegisterMethod(UWorld, bool LineTraceMultiByObjectType(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams));
      CallbackRegisterMethod(UWorld, bool LineTraceMultiByObjectType(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params));
      CallbackRegisterMethod(UWorld, AActor* SpawnActor(UClass* Class));
      CallbackRegisterMethod(UWorld, AActor* SpawnActor(UClass* Class, const FVector* Location));
      CallbackRegisterMethod(UWorld, AActor* SpawnActor(UClass* Class, const FVector* Location, const FRotator* Rotation));
      CallbackRegisterMethod(UWorld, AActor* SpawnActor(UClass* Class, const FTransform* Transform));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorAbsolute(UClass* Class, const FTransform& AbsoluteTransform));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorDeferred(UClass* Class, const FTransform& Transform));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorDeferred(UClass* Class, const FTransform& Transform, AActor* Owner));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorDeferred(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorDeferred(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride));
      CallbackRegisterMethod(UWorld, AActor* SpawnActorDeferred(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, ESpawnActorScaleMethod TransformScaleMethod));
   }
}

void RegisterTArrays()
{
   CflatRegisterTArray(&gEnv, uint8);
   CflatRegisterTArray(&gEnv, uint16);
   CflatRegisterTArray(&gEnv, uint32);
   CflatRegisterTArray(&gEnv, uint64);
   CflatRegisterTArray(&gEnv, int8);
   CflatRegisterTArray(&gEnv, int16);
   CflatRegisterTArray(&gEnv, int32);
   CflatRegisterTArray(&gEnv, int64);
   CflatRegisterTArray(&gEnv, float);
   CflatRegisterTArray(&gEnv, double);
   CflatRegisterTArray(&gEnv, bool);

   CflatRegisterTArray(&gEnv, FVector);
   CflatRegisterTArray(&gEnv, FRotator);
   CflatRegisterTArray(&gEnv, FTransform);

   CflatRegisterTArray(&gEnv, FName);
   CflatRegisterTArray(&gEnv, FString);
   CflatRegisterTArray(&gEnv, FText);

   CallbackRegisterTArray(uint8);
   CallbackRegisterTArray(uint16);
   CallbackRegisterTArray(uint32);
   CallbackRegisterTArray(uint64);
   CallbackRegisterTArray(int8);
   CallbackRegisterTArray(int16);
   CallbackRegisterTArray(int32);
   CallbackRegisterTArray(int64);
   CallbackRegisterTArray(float);
   CallbackRegisterTArray(double);
   CallbackRegisterTArray(bool);

   CallbackRegisterTArray(FVector);
   CallbackRegisterTArray(FRotator);
   CallbackRegisterTArray(FTransform);

   CallbackRegisterTArray(FName);
   CallbackRegisterTArray(FString);
   CallbackRegisterTArray(FText);
}

void RegisterFGenericPlatformMath()
{
   CflatRegisterStruct(&gEnv, FGenericPlatformMath);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, AsUInt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, AsUInt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, AsFloat, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, AsFloat, uint64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, TruncToInt32, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, TruncToInt32, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, TruncToInt32, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, TruncToInt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, TruncToInt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, TruncToFloat, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, TruncToDouble, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, TruncToFloat, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, FloorToInt32, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, FloorToInt32, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, FloorToInt64, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, FloorToInt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, FloorToInt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, FloorToFloat, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, FloorToDouble, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, FloorToFloat, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, RoundToInt32, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, RoundToInt32, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, RoundToInt64, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, RoundToInt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, RoundToInt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, RoundToFloat, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, RoundToDouble, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, RoundToFloat, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, CeilToInt32, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, CeilToInt32, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, CeilToInt64, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, CeilToInt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, CeilToInt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, CeilToFloat, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, CeilToDouble, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, CeilToFloat, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, RoundToNearestTiesToEven, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Fractional, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Fractional, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Frac, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Frac, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Modf, const float, float*);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Modf, const double, double*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Exp, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Exp, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Exp2, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Exp2, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Loge, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Loge, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, LogX, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, LogX, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Log2, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Log2, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Fmod, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Fmod, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Sin, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Sin, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Asin, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Asin, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Sinh, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Sinh, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Cos, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Cos, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Acos, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Acos, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Cosh, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Cosh, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Tan, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Tan, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Atan, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Atan, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Tanh, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Tanh, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Atan2, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Atan2, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Sqrt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Sqrt, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Pow, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Pow, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, InvSqrt, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, InvSqrt, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, InvSqrtEst, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, InvSqrtEst, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsNaN, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsNaN, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsFinite, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsFinite, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsNegativeOrNegativeZero, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, bool, IsNegativeOrNegativeZero, double);
   CflatClassAddStaticMethodReturn(&gEnv, FGenericPlatformMath, int32, Rand);
   CflatClassAddStaticMethodVoidParams1(&gEnv, FGenericPlatformMath, void, RandInit, int32);
   CflatClassAddStaticMethodReturn(&gEnv, FGenericPlatformMath, float, FRand);
   CflatClassAddStaticMethodVoidParams1(&gEnv, FGenericPlatformMath, void, SRandInit, int32);
   CflatClassAddStaticMethodReturn(&gEnv, FGenericPlatformMath, int32, GetRandSeed);
   CflatClassAddStaticMethodReturn(&gEnv, FGenericPlatformMath, float, SRand);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, FloorLog2, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, FloorLog2_64, uint64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint8, CountLeadingZeros8, uint8);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, CountLeadingZeros, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, CountLeadingZeros64, uint64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, CountTrailingZeros, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, CountTrailingZeros64, uint64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, CeilLogTwo, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, CeilLogTwo64, uint64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint8, ConstExprCeilLogTwo, size_t);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint32, RoundUpToPowerOfTwo, uint32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, uint64, RoundUpToPowerOfTwo64, uint64);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FGenericPlatformMath, float, FloatSelect, float, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FGenericPlatformMath, double, FloatSelect, double, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, Abs, const int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, Abs, const int64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Abs, const float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Abs, const double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, Sign, const int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, Sign, const int64);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Sign, const float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Sign, const double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int32, Max, const int32, const int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int64, Max, const int64, const int64);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Max, const float, const float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Max, const double, const double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int32, Min, const int32, const int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int64, Min, const int64, const int64);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Min, const float, const float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Min, const double, const double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, Max, const TArray<int32>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int32, Max, const TArray<int32>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, Max, const TArray<int64>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int64, Max, const TArray<int64>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Max, const TArray<float>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Max, const TArray<float>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Max, const TArray<double>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Max, const TArray<double>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, Min, const TArray<int32>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int32, Min, const TArray<int32>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int64, Min, const TArray<int64>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, int64, Min, const TArray<int64>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, float, Min, const TArray<float>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, float, Min, const TArray<float>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, double, Min, const TArray<double>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FGenericPlatformMath, double, Min, const TArray<double>&, int32*);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FGenericPlatformMath, int32, CountBits, uint64);

   // Callbacks for manually registered types
   CallbackRegisterType(FGenericPlatformMath);
   CallbackRegisterFunction(FGenericPlatformMath, uint32 AsUInt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 AsUInt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float AsFloat(uint32 U));
   CallbackRegisterFunction(FGenericPlatformMath, double AsFloat(uint64 U));
   CallbackRegisterFunction(FGenericPlatformMath, int32 TruncToInt32(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 TruncToInt32(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 TruncToInt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 TruncToInt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float TruncToFloat(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double TruncToDouble(double F));
   CallbackRegisterFunction(FGenericPlatformMath, double TruncToFloat(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 FloorToInt32(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 FloorToInt32(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 FloorToInt64(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 FloorToInt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 FloorToInt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float FloorToFloat(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double FloorToDouble(double F));
   CallbackRegisterFunction(FGenericPlatformMath, double FloorToFloat(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 RoundToInt32(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 RoundToInt32(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 RoundToInt64(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 RoundToInt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 RoundToInt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float RoundToFloat(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double RoundToDouble(double F));
   CallbackRegisterFunction(FGenericPlatformMath, double RoundToFloat(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 CeilToInt32(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 CeilToInt32(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 CeilToInt64(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int32 CeilToInt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 CeilToInt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float CeilToFloat(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double CeilToDouble(double F));
   CallbackRegisterFunction(FGenericPlatformMath, double CeilToFloat(double F));
   CallbackRegisterFunction(FGenericPlatformMath, int64 RoundToNearestTiesToEven(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float Fractional(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Fractional(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Frac(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Frac(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Modf(const float InValue, float* OutIntPart));
   CallbackRegisterFunction(FGenericPlatformMath, double Modf(const double InValue, double* OutIntPart));
   CallbackRegisterFunction(FGenericPlatformMath, float Exp(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Exp(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Exp2(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Exp2(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Loge(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Loge(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float LogX(float Base, float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double LogX(double Base, double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Log2(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Log2(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Fmod(float X, float Y));
   CallbackRegisterFunction(FGenericPlatformMath, double Fmod(double X, double Y));
   CallbackRegisterFunction(FGenericPlatformMath, float Sin(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Sin(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Asin(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Asin(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Sinh(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Sinh(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Cos(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Cos(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Acos(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Acos(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Cosh(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Cosh(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Tan(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Tan(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Atan(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Atan(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Tanh(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Tanh(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Atan2(float Y, float X));
   CallbackRegisterFunction(FGenericPlatformMath, double Atan2(double Y, double X));
   CallbackRegisterFunction(FGenericPlatformMath, float Sqrt(float Value));
   CallbackRegisterFunction(FGenericPlatformMath, double Sqrt(double Value));
   CallbackRegisterFunction(FGenericPlatformMath, float Pow(float A, float B));
   CallbackRegisterFunction(FGenericPlatformMath, double Pow(double A, double B));
   CallbackRegisterFunction(FGenericPlatformMath, float InvSqrt(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double InvSqrt(double F));
   CallbackRegisterFunction(FGenericPlatformMath, float InvSqrtEst(float F));
   CallbackRegisterFunction(FGenericPlatformMath, double InvSqrtEst(double F));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsNaN(float A));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsNaN(double A));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsFinite(float A));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsFinite(double A));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsNegativeOrNegativeZero(float A));
   CallbackRegisterFunction(FGenericPlatformMath, bool IsNegativeOrNegativeZero(double A));
   CallbackRegisterFunction(FGenericPlatformMath, void RandInit(int32 Seed));
   CallbackRegisterFunction(FGenericPlatformMath, void SRandInit(int32 Seed));
   CallbackRegisterFunction(FGenericPlatformMath, uint32 FloorLog2(uint32 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 FloorLog2_64(uint64 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint8 CountLeadingZeros8(uint8 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint32 CountLeadingZeros(uint32 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 CountLeadingZeros64(uint64 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint32 CountTrailingZeros(uint32 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 CountTrailingZeros64(uint64 Value));
   CallbackRegisterFunction(FGenericPlatformMath, uint32 CeilLogTwo(uint32 Arg));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 CeilLogTwo64(uint64 Arg));
   CallbackRegisterFunction(FGenericPlatformMath, uint8 ConstExprCeilLogTwo(size_t Arg));
   CallbackRegisterFunction(FGenericPlatformMath, uint32 RoundUpToPowerOfTwo(uint32 Arg));
   CallbackRegisterFunction(FGenericPlatformMath, uint64 RoundUpToPowerOfTwo64(uint64 V));
   CallbackRegisterFunction(FGenericPlatformMath, float FloatSelect(float Comparand, float ValueGEZero, float ValueLTZero));
   CallbackRegisterFunction(FGenericPlatformMath, double FloatSelect(double Comparand, double ValueGEZero, double ValueLTZero));
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Abs(const T A)");
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Sign(const T A)");
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Max(const T A, const T B)");
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Min(const T A, const T B)");
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Min(const TArray<T>& Values, int32* MinIndex = NULL)");
   CallbackRegisterFunction(FGenericPlatformMath, "template<class T> T Max(const TArray<T>& Values, int32* MaxIndex = NULL)");
   CallbackRegisterFunction(FGenericPlatformMath, int32 CountBits(uint64 Bits));
}

void RegisterFMath()
{
   CflatRegisterStruct(&gEnv, FMath);
   CflatStructAddBaseType(&gEnv, FMath, FGenericPlatformMath);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, int32, RandHelper, int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, int64, RandHelper64, int64);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, RandRange, int32, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int64, RandRange, int64, int64);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, RandRange, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, RandRange, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, FRandRange, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, FRandRange, double, double);
   CflatClassAddStaticMethodReturn(&gEnv, FMath, bool, RandBool);
   CflatClassAddStaticMethodReturn(&gEnv, FMath, FVector, VRand);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, FVector, VRandCone, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, VRandCone, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, FVector2D, RandPointInCircle, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, FVector, GetReflectionVector, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithin, const int32&, const int32&, const int32&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithin, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithin, const double&, const double&, const double&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithinInclusive, const int32&, const int32&, const int32&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithinInclusive, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsWithinInclusive, const double&, const double&, const double&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, IsNearlyEqual, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsNearlyEqual, float, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, IsNearlyEqual, double, double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, bool, IsNearlyEqual, double, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, bool, IsNearlyZero, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, IsNearlyZero, float, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, bool, IsNearlyZero, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, IsNearlyZero, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, bool, IsPowerOfTwo, int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, Floor, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, Floor, double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, Max3, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, Max3, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Min3, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, Min3, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, Min3, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Max3Index, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, int32, Square, const int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, Square, const float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, Square, const double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, int32, Cube, const int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, Cube, const float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, Cube, const double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Clamp, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, Clamp, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, Clamp, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, int32, Wrap, const int32, const int32, const int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, Wrap, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, Wrap, const double, const double, const double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, GridSnap, int32, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, GridSnap, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, GridSnap, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, DivideAndRoundUp, int32, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, DivideAndRoundDown, int32, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, DivideAndRoundNearest, int32, int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, Log2, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, Log2, double);
   CflatClassAddStaticMethodVoidParams3(&gEnv, FMath, void, SinCos, double*, double*, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, FastAsin, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, FastAsin, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RadiansToDegrees, const float&);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RadiansToDegrees, const double&);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, DegreesToRadians, const float&);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, DegreesToRadians, const double&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, ClampAngle, float, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, ClampAngle, double, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, FindDeltaAngleDegrees, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, FindDeltaAngleDegrees, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, FindDeltaAngleRadians, float, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, FindDeltaAngleRadians, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, UnwindRadians, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, UnwindRadians, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, UnwindDegrees, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, UnwindDegrees, double);
   CflatClassAddStaticMethodVoidParams2(&gEnv, FMath, void, WindRelativeAnglesDegrees, float, float&);
   CflatClassAddStaticMethodVoidParams2(&gEnv, FMath, void, WindRelativeAnglesDegrees, double, double&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, FixedTurn, float, float, float);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, CartesianToPolar, const float, const float, float&, float&);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, CartesianToPolar, const double, const double, double&, double&);
   CflatClassAddStaticMethodVoidParams2(&gEnv, FMath, void, CartesianToPolar, const FVector2D, FVector2D&);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, PolarToCartesian, const float, const float, float&, float&);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, PolarToCartesian, const double, const double, double&, double&);
   CflatClassAddStaticMethodVoidParams2(&gEnv, FMath, void, PolarToCartesian, const FVector2D, FVector2D&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, bool, GetDotDistance, FVector2D&, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, GetAzimuthAndElevation, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, GetRangePct, float, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, GetRangePct, double, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, GetRangePct, const FVector2D&, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, GetRangeValue, const FVector2D&, double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, GetMappedRangeValueClamped, const FVector2D&, const FVector2D&, double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, GetMappedRangeValueUnclamped, const FVector2D&, const FVector2D&, double);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, Lerp, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, Lerp, const double&, const double&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, Lerp, const FVector&, const FVector&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, Lerp, const FVector2D&, const FVector2D&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FRotator, Lerp, const FRotator&, const FRotator&, const float&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, LerpStable, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, LerpStable, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, LerpStable, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, LerpStable, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FRotator, LerpStable, const FRotator&, const FRotator&, float);
   CflatClassAddStaticMethodReturnParams6(&gEnv, FMath, float, BiLerp, const float&, const float&, const float&, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams6(&gEnv, FMath, double, BiLerp, const double&, const double&, const double&, const double&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams6(&gEnv, FMath, FVector, BiLerp, const FVector&, const FVector&, const FVector&, const FVector&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams6(&gEnv, FMath, FVector2D, BiLerp, const FVector2D&, const FVector2D&, const FVector2D&, const FVector2D&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, float, CubicInterp, const float&, const float&, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, double, CubicInterp, const double&, const double&, const double&, const double&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector, CubicInterp, const FVector&, const FVector&, const FVector&, const FVector&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector2D, CubicInterp, const FVector2D&, const FVector2D&, const FVector2D&, const FVector2D&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, float, CubicInterpDerivative, const float&, const float&, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, double, CubicInterpDerivative, const double&, const double&, const double&, const double&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector, CubicInterpDerivative, const FVector&, const FVector&, const FVector&, const FVector&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector2D, CubicInterpDerivative, const FVector2D&, const FVector2D&, const FVector2D&, const FVector2D&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, float, CubicInterpSecondDerivative, const float&, const float&, const float&, const float&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, double, CubicInterpSecondDerivative, const double&, const double&, const double&, const double&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector, CubicInterpSecondDerivative, const FVector&, const FVector&, const FVector&, const FVector&, const float&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector2D, CubicInterpSecondDerivative, const FVector2D&, const FVector2D&, const FVector2D&, const FVector2D&, const float&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, InterpEaseIn, const float&, const float&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, InterpEaseIn, const double&, const double&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, InterpEaseIn, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, InterpEaseIn, const FVector2D&, const FVector2D&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, InterpEaseOut, const float&, const float&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, InterpEaseOut, const double&, const double&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, InterpEaseOut, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, InterpEaseOut, const FVector2D&, const FVector2D&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, InterpEaseInOut, const float&, const float&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, InterpEaseInOut, const double&, const double&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, InterpEaseInOut, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, InterpEaseInOut, const FVector2D&, const FVector2D&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, InterpStep, const float&, const float&, float, int32);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, InterpStep, const double&, const double&, float, int32);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, InterpStep, const FVector&, const FVector&, float, int32);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, InterpStep, const FVector2D&, const FVector2D&, float, int32);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpSinIn, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpSinIn, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpSinIn, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpSinIn, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpSinOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpSinOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpSinOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpSinOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpSinInOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpSinInOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpSinInOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpSinInOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpExpoIn, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpExpoIn, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpExpoIn, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpExpoIn, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpExpoOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpExpoOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpExpoOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpExpoOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpExpoInOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpExpoInOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpExpoInOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpExpoInOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpCircularIn, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpCircularIn, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpCircularIn, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpCircularIn, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpCircularOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpCircularOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpCircularOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpCircularOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, InterpCircularInOut, const float&, const float&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, InterpCircularInOut, const double&, const double&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, InterpCircularInOut, const FVector&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, InterpCircularInOut, const FVector2D&, const FVector2D&, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FRotator, LerpRange, const FRotator&, const FRotator&, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, VInterpNormalRotationTo, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, VInterpConstantTo, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, VInterpTo, const FVector&, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, Vector2DInterpConstantTo, const FVector2D&, const FVector2D&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector2D, Vector2DInterpTo, const FVector2D&, const FVector2D&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FRotator, RInterpConstantTo, const FRotator&, const FRotator&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FRotator, RInterpTo, const FRotator&, const FRotator&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, FInterpConstantTo, float, float, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, FInterpConstantTo, double, double, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, FInterpTo, float, float, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, double, FInterpTo, double, double, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FLinearColor, CInterpTo, const FLinearColor&, const FLinearColor&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FQuat, QInterpConstantTo, const FQuat&, const FQuat&, float, float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FQuat, QInterpTo, const FQuat&, const FQuat&, float, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, InvExpApprox, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, InvExpApprox, double);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, ExponentialSmoothingApprox, float&, const float&, const float, const float);
   CflatClassAddStaticMethodVoidParams4(&gEnv, FMath, void, ExponentialSmoothingApprox, double&, const double&, const float, const float);
   CflatClassAddStaticMethodVoidParams6(&gEnv, FMath, void, CriticallyDampedSmoothing, float&, float&, const float&, const float&, const float, const float);
   CflatClassAddStaticMethodVoidParams6(&gEnv, FMath, void, CriticallyDampedSmoothing, double&, double&, const double&, const double&, const float, const float);
   CflatClassAddStaticMethodVoidParams7(&gEnv, FMath, void, SpringDamper, float&, float&, const float&, const float&, const float, const float, const float);
   CflatClassAddStaticMethodVoidParams7(&gEnv, FMath, void, SpringDamper, double&, double&, const double&, const double&, const float, const float, const float);
   CflatClassAddStaticMethodVoidParams7(&gEnv, FMath, void, SpringDamperSmoothing, float&, float&, const float&, const float&, const float, const float, const float);
   CflatClassAddStaticMethodVoidParams7(&gEnv, FMath, void, SpringDamperSmoothing, double&, double&, const double&, const double&, const float, const float, const float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, MakePulsatingValue, const double, const float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, MakePulsatingValue, const double, const float, const float);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, LinePlaneIntersection, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, bool, LineSphereIntersection, const FVector&, const FVector&, double, const FVector&, double);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, bool, SphereConeIntersection, const FVector&, float, const FVector&, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, ClosestPointOnLine, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, ClosestPointOnInfiniteLine, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, float, PointDistToLine, const FVector&, const FVector&, const FVector&, FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, PointDistToLine, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector, ClosestPointOnSegment, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, FVector2D, ClosestPointOnSegment2D, const FVector2D&, const FVector2D&, const FVector2D&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, PointDistToSegment, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, PointDistToSegmentSquared, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodVoidParams6(&gEnv, FMath, void, SegmentDistToSegment, FVector, FVector, FVector, FVector, FVector&, FVector&);
   CflatClassAddStaticMethodVoidParams6(&gEnv, FMath, void, SegmentDistToSegmentSafe, FVector, FVector, FVector, FVector, FVector&, FVector&);
   CflatClassAddStaticMethodReturnParams7(&gEnv, FMath, bool, SegmentTriangleIntersection, const FVector&, const FVector&, const FVector&, const FVector&, const FVector&, FVector&, FVector&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, bool, SegmentIntersection2D, const FVector&, const FVector&, const FVector&, const FVector&, FVector&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, ClosestPointOnTriangleToPoint, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, FVector, ClosestPointOnTetrahedronToPoint, const FVector&, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodVoidParams5(&gEnv, FMath, void, SphereDistToLine, FVector, float, FVector, FVector, FVector&);
   CflatClassAddStaticMethodReturnParams6(&gEnv, FMath, bool, GetDistanceWithinConeSegment, FVector, FVector, FVector, float, float, float&);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, bool, PointsAreCoplanar, const TArray<FVector>&);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, PointsAreCoplanar, const TArray<FVector>&, const float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, TruncateToHalfIfClose, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, float, TruncateToHalfIfClose, float, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, TruncateToHalfIfClose, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, double, TruncateToHalfIfClose, double, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundHalfToEven, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundHalfToEven, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundHalfFromZero, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundHalfFromZero, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundHalfToZero, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundHalfToZero, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundFromZero, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundFromZero, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundToZero, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundToZero, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundToNegativeInfinity, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundToNegativeInfinity, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, RoundToPositiveInfinity, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, double, RoundToPositiveInfinity, double);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, FString, FormatIntToHumanReadable, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, Eval, FString, float&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, GetBaryCentric2D, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, GetBaryCentric2D, const FVector2D&, const FVector2D&, const FVector2D&, const FVector2D&);
   CflatClassAddStaticMethodReturnParams4(&gEnv, FMath, FVector, ComputeBaryCentric2D, const FVector&, const FVector&, const FVector&, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, SmoothStep, float, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, SmoothStep, double, double, double);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, bool, ExtractBoolFromBitfield, const uint8*, uint32);
   CflatClassAddStaticMethodVoidParams3(&gEnv, FMath, void, SetBoolInBitField, uint8*, uint32, bool);
   CflatClassAddStaticMethodVoidParams2(&gEnv, FMath, void, ApplyScaleToFloat, float&, const FVector&);
   CflatClassAddStaticMethodVoidParams3(&gEnv, FMath, void, ApplyScaleToFloat, float&, const FVector&, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, uint8, Quantize8UnsignedByte, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, uint8, Quantize8SignedByte, float);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, GreatestCommonDivisor, int32, int32);
   CflatClassAddStaticMethodReturnParams2(&gEnv, FMath, int32, LeastCommonMultiplier, int32, int32);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, PerlinNoise1D, float);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, PerlinNoise2D, const FVector2D&);
   CflatClassAddStaticMethodReturnParams1(&gEnv, FMath, float, PerlinNoise3D, const FVector&);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, float, WeightedMovingAverage, float, float, float);
   CflatClassAddStaticMethodReturnParams3(&gEnv, FMath, double, WeightedMovingAverage, double, double, double);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, float, DynamicWeightedMovingAverage, float, float, float, float, float);
   CflatClassAddStaticMethodReturnParams5(&gEnv, FMath, double, DynamicWeightedMovingAverage, double, double, double, double, double);

   // Callbacks for manually registered types
   CallbackRegisterType(FMath);
   CallbackRegisterFunction(FMath, int32 RandRange(int32 Min, int32 Max));
   CallbackRegisterFunction(FMath, int64 RandRange(int64 Min, int64 Max));
   CallbackRegisterFunction(FMath, float RandRange(float Min, float Max));
   CallbackRegisterFunction(FMath, double RandRange(double Min, double Max));
   CallbackRegisterFunction(FMath, float FRandRange(float Min, float Max));
   CallbackRegisterFunction(FMath, double FRandRange(double InMin, double InMax));
   CallbackRegisterFunction(FMath, FVector VRandCone(const FVector& Dir, float ConeHalfAngleRad));
   CallbackRegisterFunction(FMath, FVector VRandCone(const FVector& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad));
   CallbackRegisterFunction(FMath, FVector2D RandPointInCircle(float CircleRadius));
   CallbackRegisterFunction(FMath, FVector GetReflectionVector(const FVector& Direction, const FVector& SurfaceNormal));
   CallbackRegisterFunction(FMath, "template<class T, class U> bool IsWithin(const T& TestValue, const U& MinValue, const U& MaxValue)");
   CallbackRegisterFunction(FMath, "template<class T, class U> bool IsWithinInclusive(const T& TestValue, const U& MinValue, const U& MaxValue)");
   CallbackRegisterFunction(FMath, bool IsNearlyEqual(float A, float B, float ErrorTolerance = UE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, bool IsNearlyEqual(double A, double B, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, bool IsNearlyZero(float Value, float ErrorTolerance = UE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, bool IsNearlyZero(double Value, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, "template <typename T> bool IsPowerOfTwo(T Value)");
   CallbackRegisterFunction(FMath, float Floor(float F));
   CallbackRegisterFunction(FMath, double Floor(double F));
   CallbackRegisterFunction(FMath, "template <typename T> T Max3(const T A, const T B, const T C)");
   CallbackRegisterFunction(FMath, "template <typename T> T Min3(const T A, const T B, const T C)");
   CallbackRegisterFunction(FMath, "template <class T> int32 Max3Index(const T A, const T B, const T C)");
   CallbackRegisterFunction(FMath, "template <class T> int32 Min3Index(const T A, const T B, const T C)");
   CallbackRegisterFunction(FMath, "template <class T> T Square(const T A)");
   CallbackRegisterFunction(FMath, "template <class T> T Cube(const T A)");
   CallbackRegisterFunction(FMath, "template <class T> T Clamp(const T X, const T MinValue, const T MaxValue)");
   CallbackRegisterFunction(FMath, "template <class T> T Wrap(const T X, const T Min, const T Max)");
   CallbackRegisterFunction(FMath, "template <class T> T GridSnap(T Location, T Grid)");
   CallbackRegisterFunction(FMath, "template <class T> T DivideAndRoundUp(T Dividend, T Divisor)");
   CallbackRegisterFunction(FMath, "template <class T> T DivideAndRoundDown(T Dividend, T Divisor)");
   CallbackRegisterFunction(FMath, "template <class T> T DivideAndRoundNearest(T Dividend, T Divisor)");
   CallbackRegisterFunction(FMath, float Log2(float Value));
   CallbackRegisterFunction(FMath, double Log2(double Value));
   CallbackRegisterFunction(FMath, void SinCos(double* ScalarSin, double* ScalarCos, double Value));
   CallbackRegisterFunction(FMath, float FastAsin(float Value));
   CallbackRegisterFunction(FMath, double FastAsin(double Value));
   CallbackRegisterFunction(FMath, "template <class T> T RadiansToDegrees(const T& RadVal)");
   CallbackRegisterFunction(FMath, "template <class T> T DegreesToRadians(const T& DegVal)");
   CallbackRegisterFunction(FMath, "template <typename T> T ClampAngle(T AngleDegrees, T MinAngleDegrees, T MaxAngleDegrees)");
   CallbackRegisterFunction(FMath, "template <typename T, typename T2> T FindDeltaAngleDegrees(T A1, T2 A2)");
   CallbackRegisterFunction(FMath, "template <typename T, typename T2> T FindDeltaAngleRadians(T A1, T2 A2)");
   CallbackRegisterFunction(FMath, "template <typename T> T UnwindRadians(T A)");
   CallbackRegisterFunction(FMath, "template <typename T> T UnwindDegrees(T A)");
   CallbackRegisterFunction(FMath, void WindRelativeAnglesDegrees(float InAngle0, float& InOutAngle1));
   CallbackRegisterFunction(FMath, void WindRelativeAnglesDegrees(double InAngle0, double& InOutAngle1));
   CallbackRegisterFunction(FMath, float FixedTurn(float InCurrent, float InDesired, float InDeltaRate));
   CallbackRegisterFunction(FMath, "template <typename T> void CartesianToPolar(const T X, const T Y, T& OutRad, T& OutAng)");
   CallbackRegisterFunction(FMath, "template <typename T> T PolarToCartesian(const T Rad, const T Ang, T& OutX, T& OutY)");
   CallbackRegisterFunction(FMath, bool GetDotDistance(FVector2D& OutDotDist, const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ));
   CallbackRegisterFunction(FMath, FVector2D GetAzimuthAndElevation(const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ));
   CallbackRegisterFunction(FMath, "template <typename T, typename T2> T GetRangePct(T MinValue, T MaxValue, T2 Value)");
   CallbackRegisterFunction(FMath, "template <typename T, typename T2> T GetRangePct(T Range, T2 Value)");
   CallbackRegisterFunction(FMath, "template <typename T, typename T2> T GetRangeValue(const T& Range, T2 Pct)");
   CallbackRegisterFunction(FMath, "template <typename T> T GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, const T Value)");
   CallbackRegisterFunction(FMath, "template <typename T> T GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, const T Value)");
   CallbackRegisterFunction(FMath, "template <typename T1, typename T2, typename T3> T1 Lerp(const T1& A, const T2& B, const T3& Alpha)");
   CallbackRegisterFunction(FMath, "template <typename T> T LerpStable(const T& A, const T& B, double Alpha)");
   CallbackRegisterFunction(FMath, "template <typename T, typename U> T BiLerp(const T& P00,const T& P10,const T& P01,const T& P11, const U& FracX, const U& FracY)");
   CallbackRegisterFunction(FMath, "template <typename T, typename U> T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)");
   CallbackRegisterFunction(FMath, "template <typename T, typename U> T CubicInterpDerivative(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)");
   CallbackRegisterFunction(FMath, "template <typename T, typename U> T CubicInterpSecondDerivative(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpEaseIn(const T& A, const T& B, float Alpha, float Exp)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpEaseOut(const T& A, const T& B, float Alpha, float Exp)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpEaseInOut(const T& A, const T& B, float Alpha, float Exp)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpStep(const T& A, const T& B, float Alpha, int32 Steps)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpSinIn(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpSinOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpSinInOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpExpoIn(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpExpoOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpExpoInOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpCircularIn(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpCircularOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, "template <class T> T InterpCircularInOut(const T& A, const T& B, float Alpha)");
   CallbackRegisterFunction(FMath, FRotator LerpRange(const FRotator& A, const FRotator& B, float Alpha));
   CallbackRegisterFunction(FMath, FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees));
   CallbackRegisterFunction(FMath, FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FVector2D Vector2DInterpConstantTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FVector2D Vector2DInterpTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FRotator RInterpConstantTo( const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, "template <class T> T FInterpConstantTo(T Current, T Target, T DeltaTime, T InterpSpeed)");
   CallbackRegisterFunction(FMath, "template <class T> T FInterpTo(T  Current, T Target, T DeltaTime, T InterpSpeed)");
   CallbackRegisterFunction(FMath, FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FQuat QInterpConstantTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, FQuat QInterpTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed));
   CallbackRegisterFunction(FMath, "template <class T> T InvExpApprox(T X)");
   CallbackRegisterFunction(FMath, "template <class T> void ExponentialSmoothingApprox(T& InOutValue, const T& InTargetValue, const float InDeltaTime, const float InSmoothingTime)");
   CallbackRegisterFunction(FMath, "template <class T> void CriticallyDampedSmoothing(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate, const float InDeltaTime, const float InSmoothingTime)");
   CallbackRegisterFunction(FMath, "template <class T> void SpringDamper(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate, const float InDeltaTime, const float InUndampedFrequency, const float InDampingRatio)");
   CallbackRegisterFunction(FMath, "template <class T> void SpringDamperSmoothing(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate, const float InDeltaTime, const float InSmoothingTime, const float InDampingRatio)");
   CallbackRegisterFunction(FMath, float MakePulsatingValue(const double InCurrentTime, const float InPulsesPerSecond, const float InPhase = 0.0f));
   CallbackRegisterFunction(FMath, FVector LinePlaneIntersection(const FVector& Point1, const FVector& Point2, const FVector& PlaneOrigin, const FVector& PlaneNormal));
   CallbackRegisterFunction(FMath, bool LineSphereIntersection(const FVector& Start, const FVector& Dir, double Length, const FVector& Origin, double Radius));
   CallbackRegisterFunction(FMath, bool SphereConeIntersection(const FVector& SphereCenter, float SphereRadius, const FVector& ConeAxis, float ConeAngleSin, float ConeAngleCos));
   CallbackRegisterFunction(FMath, FVector ClosestPointOnLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point));
   CallbackRegisterFunction(FMath, FVector ClosestPointOnInfiniteLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point));
   CallbackRegisterFunction(FMath, float PointDistToLine(const FVector& Point, const FVector& Direction, const FVector& Origin, FVector& OutClosestPoint));
   CallbackRegisterFunction(FMath, float PointDistToLine(const FVector& Point, const FVector& Direction, const FVector& Origin));
   CallbackRegisterFunction(FMath, FVector ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint));
   CallbackRegisterFunction(FMath, FVector2D ClosestPointOnSegment2D(const FVector2D& Point, const FVector2D& StartPoint, const FVector2D& EndPoint));
   CallbackRegisterFunction(FMath, float PointDistToSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint));
   CallbackRegisterFunction(FMath, float PointDistToSegmentSquared(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint));
   CallbackRegisterFunction(FMath, void SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2));
   CallbackRegisterFunction(FMath, void SegmentDistToSegmentSafe(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2));
   CallbackRegisterFunction(FMath, bool SegmentTriangleIntersection(const FVector& StartPoint, const FVector& EndPoint, const FVector& A, const FVector& B, const FVector& C, FVector& OutIntersectPoint, FVector& OutTriangleNormal));
   CallbackRegisterFunction(FMath, bool SegmentIntersection2D(const FVector& SegmentStartA, const FVector& SegmentEndA, const FVector& SegmentStartB, const FVector& SegmentEndB, FVector& out_IntersectionPoint));
   CallbackRegisterFunction(FMath, FVector ClosestPointOnTriangleToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C));
   CallbackRegisterFunction(FMath, FVector ClosestPointOnTetrahedronToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C, const FVector& D));
   CallbackRegisterFunction(FMath, void SphereDistToLine(FVector SphereOrigin, float SphereRadius, FVector LineOrigin, FVector LineDir, FVector& OutClosestPoint));
   CallbackRegisterFunction(FMath, bool GetDistanceWithinConeSegment(FVector Point, FVector ConeStartPoint, FVector ConeLine, float RadiusAtStart, float RadiusAtEnd, float& PercentageOut));
   CallbackRegisterFunction(FMath, bool PointsAreCoplanar(const TArray<FVector>& Points, const float Tolerance = 0.1f));
   CallbackRegisterFunction(FMath, float TruncateToHalfIfClose(float F, float Tolerance = UE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, double TruncateToHalfIfClose(double F, double Tolerance = UE_SMALL_NUMBER));
   CallbackRegisterFunction(FMath, float RoundHalfToEven(float F));
   CallbackRegisterFunction(FMath, double RoundHalfToEven(double F));
   CallbackRegisterFunction(FMath, float RoundHalfFromZero(float F));
   CallbackRegisterFunction(FMath, double RoundHalfFromZero(double F));
   CallbackRegisterFunction(FMath, float RoundHalfToZero(float F));
   CallbackRegisterFunction(FMath, double RoundHalfToZero(double F));
   CallbackRegisterFunction(FMath, float RoundFromZero(float F));
   CallbackRegisterFunction(FMath, double RoundFromZero(double F));
   CallbackRegisterFunction(FMath, float RoundToZero(float F));
   CallbackRegisterFunction(FMath, double RoundToZero(double F));
   CallbackRegisterFunction(FMath, float RoundToNegativeInfinity(float F));
   CallbackRegisterFunction(FMath, double RoundToNegativeInfinity(double F));
   CallbackRegisterFunction(FMath, float RoundToPositiveInfinity(float F));
   CallbackRegisterFunction(FMath, double RoundToPositiveInfinity(double F));
   CallbackRegisterFunction(FMath, FString FormatIntToHumanReadable(int32 Val));
   CallbackRegisterFunction(FMath, bool Eval(FString Str, float& OutValue));
   CallbackRegisterFunction(FMath, FVector GetBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C));
   CallbackRegisterFunction(FMath, FVector GetBaryCentric2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C));
   CallbackRegisterFunction(FMath, FVector ComputeBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C));
   CallbackRegisterFunction(FMath, "template <typename T> T SmoothStep(T A, T B, T X)");
   CallbackRegisterFunction(FMath, bool ExtractBoolFromBitfield(const uint8* Ptr, uint32 Index));
   CallbackRegisterFunction(FMath, void SetBoolInBitField(uint8* Ptr, uint32 Index, bool bSet));
   CallbackRegisterFunction(FMath, void ApplyScaleToFloat(float& Dst, const FVector& DeltaScale, float Magnitude = 1.0f));
   CallbackRegisterFunction(FMath, uint8 Quantize8UnsignedByte(float x));
   CallbackRegisterFunction(FMath, uint8 Quantize8SignedByte(float x));
   CallbackRegisterFunction(FMath, int32 GreatestCommonDivisor(int32 a, int32 b));
   CallbackRegisterFunction(FMath, int32 LeastCommonMultiplier(int32 a, int32 b));
   CallbackRegisterFunction(FMath, float PerlinNoise1D(float Value));
   CallbackRegisterFunction(FMath, float PerlinNoise2D(const FVector2D& Location));
   CallbackRegisterFunction(FMath, float PerlinNoise3D(const FVector& Location));
   CallbackRegisterFunction(FMath, "template <typename T> T WeightedMovingAverage(T CurrentSample, T PreviousSample, T Weight)");
   CallbackRegisterFunction(FMath, "template <typename T> T DynamicWeightedMovingAverage(T CurrentSample, T PreviousSample, T MaxDistance, T MinWeight, T MaxWeight)");
}


void RegisterScriptDelegates()
{
   {
      CflatRegisterClass(&gEnv, FScriptDelegate);
      CflatClassAddConstructor(&gEnv, FScriptDelegate);
      CflatClassAddCopyConstructor(&gEnv, FScriptDelegate);
   }
   {
      CflatRegisterClass(&gEnv, FTimerDynamicDelegate);
      CflatClassAddBaseType(&gEnv, FTimerDynamicDelegate, FScriptDelegate);
      CflatClassAddConstructor(&gEnv, FTimerDynamicDelegate);
      CflatClassAddConstructorParams1(&gEnv, FTimerDynamicDelegate, const FScriptDelegate&);
   }
}

void UnrealModule::Init()
{
   {
      gEnv.defineMacro("TEXT(x)", "L##x");
   }

   {
      CflatRegisterTypeAlias(&gEnv, wchar_t, TCHAR);
   }

   {
      CflatRegisterBuiltInTypedef(&gEnv, uint8, uint8_t);
      CflatRegisterBuiltInTypedef(&gEnv, uint16, uint16_t);
      CflatRegisterBuiltInTypedef(&gEnv, uint32, uint32_t);
      CflatRegisterBuiltInTypedef(&gEnv, uint64, uint64_t);
      CflatRegisterBuiltInTypedef(&gEnv, int8, char);
      CflatRegisterBuiltInTypedef(&gEnv, int16, short);
      CflatRegisterBuiltInTypedef(&gEnv, int32, int);
      CflatRegisterBuiltInTypedef(&gEnv, int64, int64_t);
   }

   {
      CflatRegisterClass(&gEnv, FString);
      CflatClassAddConstructor(&gEnv, FString);
      CflatClassAddConstructorParams1(&gEnv, FString, const char*);
      CflatClassAddConstructorParams1(&gEnv, FString, const TCHAR*);
      CflatClassAddCopyConstructor(&gEnv, FString);
      CflatClassAddMethodReturnParams1(&gEnv, FString, FString&, Append, const char*);
      CflatClassAddMethodReturnParams1(&gEnv, FString, FString&, Append, const TCHAR*);
      CflatClassAddMethodReturnParams1(&gEnv, FString, FString&, Append, const FString&);
      CflatClassAddMethodReturn(&gEnv, FString, const TCHAR*, operator*) CflatMethodConst;
      CflatClassAddStaticMethodReturnParams1(&gEnv, FString, FString, FromInt, int32);
      CflatClassAddStaticMethodReturnParams1(&gEnv, FString, FString, SanitizeFloat, double);

      CflatRegisterFunctionReturnParams2(&gEnv, FString, operator+, const FString&, const FString&);

      // Callbacks for manually registered types
      CallbackRegisterType(FString);
      CallbackRegisterMethod(FString, (const char* String));
   }
   {
      CflatRegisterClass(&gEnv, FName);
      CflatClassAddConstructor(&gEnv, FName);
      CflatClassAddConstructorParams1(&gEnv, FName, const char*);
      CflatClassAddConstructorParams1(&gEnv, FName, FString);
      CflatClassAddCopyConstructor(&gEnv, FName);
      CflatClassAddMethodReturn(&gEnv, FName, FString, ToString) CflatMethodConst;
      CflatClassAddMethodVoidParams1(&gEnv, FName, void, ToString, FString&) CflatMethodConst;
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator==, FName) CflatMethodConst;
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator!=, FName) CflatMethodConst;

      // Callbacks for manually registered types
      CallbackRegisterType(FName);
      CallbackRegisterMethod(FName, (const char* Name));
      CallbackRegisterMethod(FName, (FString Name));
      CallbackRegisterMethod(FName, FString ToString() const);
      CallbackRegisterMethod(FName, void ToString(FString& Out) const);
   }
   {
      CflatRegisterClass(&gEnv, FText);
      CflatClassAddConstructor(&gEnv, FText);
      CflatClassAddCopyConstructor(&gEnv, FText);
      CflatClassAddStaticMethodReturnParams1(&gEnv, FText, FText, FromString, const FString&);
      CflatStructAddStaticMethodReturn(&gEnv, FText, const FText&, GetEmpty);

      // Callbacks for manually registered types
      CallbackRegisterType(FText);
      CallbackRegisterFunction(FText, FText FromString(const FString& String));
      CallbackRegisterFunction(FText, const FText& GetEmpty());
   }

   {
      CflatRegisterStruct(&gEnv, FVector);
      CflatStructAddConstructor(&gEnv, FVector);
      CflatStructAddConstructorParams3(&gEnv, FVector, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector);
      CflatStructAddMember(&gEnv, FVector, double, X);
      CflatStructAddMember(&gEnv, FVector, double, Y);
      CflatStructAddMember(&gEnv, FVector, double, Z);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, double, Dot, const FVector&);
      CflatStructAddMethodVoidParams3(&gEnv, FVector, void, Set, double, double, double);
      CflatStructAddMethodReturn(&gEnv, FVector, double, Length);
      CflatStructAddMethodReturn(&gEnv, FVector, double, SquaredLength);
      CflatStructAddMethodReturn(&gEnv, FVector, bool, IsZero);
      CflatStructAddMethodReturn(&gEnv, FVector, bool, IsNormalized);
      CflatStructAddMethodReturn(&gEnv, FVector, bool, Normalize);
      CflatStructAddMethodVoid(&gEnv, FVector, FVector, GetUnsafeNormal);
      CflatStructAddStaticMethodReturnParams2(&gEnv, FVector, double, Dist, const FVector&, const FVector&);
      CflatStructAddStaticMethodReturnParams2(&gEnv, FVector, double, Distance, const FVector&, const FVector&);
      CflatStructAddStaticMethodReturnParams2(&gEnv, FVector, double, DistSquared, const FVector&, const FVector&);

      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator+, const FVector&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator-, const FVector&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator*, double) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator/, double) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator+=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator-=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator*=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator/=, const FVector&);

      CflatStructAddStaticMember(&gEnv, FVector, FVector, ZeroVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, OneVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, UpVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, DownVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, ForwardVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, BackwardVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, RightVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, LeftVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, XAxisVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, YAxisVector);
      CflatStructAddStaticMember(&gEnv, FVector, FVector, ZAxisVector);

      CflatRegisterTypeAlias(&gEnv, FVector, FVector3d);
      CflatRegisterTypeAlias(&gEnv, FVector, FVector3f);
      CflatRegisterTypeAlias(&gEnv, FVector, FVector_NetQuantize); // @LB Maybe use concrect type?
      CflatRegisterTypeAlias(&gEnv, FVector, FVector_NetQuantizeNormal); // @LB Maybe use concrect type?

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FVector);
      CallbackRegisterMethod(FVector, (double X, double Y, double Z));
      CallbackRegisterMethod(FVector, void Set(double X, double Y, double Z));
      CallbackRegisterFunction(FVector, double Dist(const FVector& V1, const FVector& V2));
      CallbackRegisterFunction(FVector, double Distance(const FVector& V1, const FVector& V2));
      CallbackRegisterFunction(FVector, double DistSquared(const FVector& V1, const FVector& V2));
   }
   {
      CflatRegisterStruct(&gEnv, FVector2D);
      CflatStructAddConstructor(&gEnv, FVector2D);
      CflatStructAddConstructorParams2(&gEnv, FVector2D, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector2D);
      CflatStructAddMember(&gEnv, FVector2D, double, X);
      CflatStructAddMember(&gEnv, FVector2D, double, Y);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FVector2D);
      CallbackRegisterMethod(FVector2D, (double InX, double InY));
   }
   {
      CflatRegisterStruct(&gEnv, FQuat);
      CflatStructAddConstructor(&gEnv, FQuat);
      CflatStructAddConstructorParams4(&gEnv, FQuat, double, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FQuat);
      CflatStructAddMember(&gEnv, FQuat, double, X);
      CflatStructAddMember(&gEnv, FQuat, double, Y);
      CflatStructAddMember(&gEnv, FQuat, double, Z);
      CflatStructAddMember(&gEnv, FQuat, double, W);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FQuat);
      CallbackRegisterMethod(FQuat, (double InX, double InY, double InZ, double InW));
   }
   {
      CflatRegisterStruct(&gEnv, FRotator);
      CflatStructAddConstructor(&gEnv, FRotator);
      CflatStructAddConstructorParams3(&gEnv, FRotator, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FRotator);
      CflatStructAddMember(&gEnv, FRotator, double, Pitch);
      CflatStructAddMember(&gEnv, FRotator, double, Yaw);
      CflatStructAddMember(&gEnv, FRotator, double, Roll);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator+, const FRotator&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator-, const FRotator&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator*, double) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator*=, double);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, bool, operator==, const FRotator&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, bool, operator!=, const FRotator&) CflatMethodConst;
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator+=, const FRotator&);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator-=, const FRotator&);
      CflatStructAddMethodReturn(&gEnv, FRotator, bool, IsZero);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, bool, Equals, const FRotator&);
      CflatStructAddMethodVoidParams3(&gEnv, FRotator, void, Add, double, double, double);
      CflatStructAddMethodReturn(&gEnv, FRotator, FRotator, GetInverse);
      CflatStructAddMethodReturn(&gEnv, FRotator, FVector, Vector);
      CflatStructAddMethodReturn(&gEnv, FRotator, FQuat, Quaternion);
      CflatStructAddMethodReturn(&gEnv, FRotator, FVector, Euler);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FVector, RotateVector, const FVector&);
      CflatStructAddMethodReturn(&gEnv, FRotator, FRotator, GetNormalized);
      CflatStructAddMethodVoid(&gEnv, FRotator, void, Normalize);
      CflatStructAddStaticMethodReturnParams1(&gEnv, FRotator, FRotator, MakeFromEuler, const FVector&);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FRotator);
      CallbackRegisterMethod(FRotator, (double InPitch, double InYaw, double InRoll));
      CallbackRegisterMethod(FRotator, void Set(double DeltaPitch, double DeltaYaw, double DeltaRoll));
      CallbackRegisterMethod(FRotator, FVector RotateVector(const FVector& V));
      CallbackRegisterFunction(FRotator, FRotator MakeFromEuler(const FVector& Euler));
   }
   {
      CflatRegisterStruct(&gEnv, FTransform);
      CflatStructAddConstructor(&gEnv, FTransform);
      CflatStructAddMethodReturn(&gEnv, FTransform, FVector, GetTranslation);
      CflatStructAddMethodReturn(&gEnv, FTransform, FQuat, GetRotation);
      CflatStructAddMethodReturn(&gEnv, FTransform, FRotator, Rotator);
      CflatStructAddMethodReturn(&gEnv, FTransform, FVector, GetScale3D);
      CflatStructAddMethodVoidParams1(&gEnv, FTransform, void, SetTranslation, const FVector&);
      CflatStructAddMethodVoidParams1(&gEnv, FTransform, void, SetRotation, const FQuat&);
      CflatStructAddMethodVoidParams1(&gEnv, FTransform, void, SetScale3D, const FVector&);
      CflatStructAddStaticMember(&gEnv, FTransform, FTransform, Identity);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FTransform);
      CallbackRegisterMethod(FTransform, void SetTranslation(const FVector& NewTranslation));
      CallbackRegisterMethod(FTransform, void SetRotation(const FQuat& NewRotation));
      CallbackRegisterMethod(FTransform, void SetScale3D(const FVector& NewScale3D));
   }

   {
      CflatRegisterStruct(&gEnv, FColor);
      CflatStructAddConstructor(&gEnv, FColor);
      CflatStructAddConstructorParams3(&gEnv, FColor, uint8, uint8, uint8);
      CflatStructAddConstructorParams4(&gEnv, FColor, uint8, uint8, uint8, uint8);
      CflatStructAddCopyConstructor(&gEnv, FColor);
      CflatStructAddMember(&gEnv, FColor, uint8, R);
      CflatStructAddMember(&gEnv, FColor, uint8, G);
      CflatStructAddMember(&gEnv, FColor, uint8, B);
      CflatStructAddMember(&gEnv, FColor, uint8, A);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FColor);
      CallbackRegisterMethod(FColor, (uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255u));
   }
   {
      CflatRegisterStruct(&gEnv, FLinearColor);
      CflatStructAddConstructor(&gEnv, FLinearColor);
      CflatStructAddConstructorParams3(&gEnv, FLinearColor, float, float, float);
      CflatStructAddConstructorParams4(&gEnv, FLinearColor, float, float, float, float);
      CflatStructAddCopyConstructor(&gEnv, FLinearColor);
      CflatStructAddMember(&gEnv, FLinearColor, float, R);
      CflatStructAddMember(&gEnv, FLinearColor, float, G);
      CflatStructAddMember(&gEnv, FLinearColor, float, B);
      CflatStructAddMember(&gEnv, FLinearColor, float, A);

      // Callbacks for manually registered types
      CallbackRegisterBaseStructureType(FLinearColor);
      CallbackRegisterMethod(FLinearColor, (float InR, float InG, float InB, float InA = 1.0f));
   }


   RegisterTArrays();
   RegisterFGenericPlatformMath();
   RegisterFMath();
   RegisterScriptDelegates();


   {
      enum LOG_CATEGORY { LogTemp, LogText };
      enum LOG_VERBOSITY { NoLogging, Fatal, Error, Warning, Display, Log, Verbose, VeryVerbose, All, BreakOnLog };

      Function* function = gEnv.registerFunction("UE_LOG");
      CflatSetFlag(function->mFlags, FunctionFlags::Variadic);
      function->mParameters.push_back(gEnv.getTypeUsage("uint8_t"));
      function->mParameters.push_back(gEnv.getTypeUsage("uint8_t"));
      function->mParameters.push_back(gEnv.getTypeUsage("const wchar_t*"));
      function->execute = UELogExecute;

      {
         CflatRegisterEnum(&gEnv, LOG_CATEGORY);
         CflatEnumAddValue(&gEnv, LOG_CATEGORY, LogTemp);
         CflatEnumAddValue(&gEnv, LOG_CATEGORY, LogText);
      }

      {
         CflatRegisterEnum(&gEnv, LOG_VERBOSITY);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, NoLogging);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Fatal);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Error);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Warning);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Display);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Log);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, Verbose);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, VeryVerbose);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, All);
         CflatEnumAddValue(&gEnv, LOG_VERBOSITY, BreakOnLog);
      }
   }
}

void UnrealModule::LoadScripts(const FString& pFileExtension, ScriptFilterDelegate pFilterDelegate)
{
   // Load all scripts
   const FString scriptsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts/");

   TArray<FString> scriptFilenames;
   IFileManager::Get().FindFiles(scriptFilenames, *scriptsDir, *pFileExtension);

   TArray<FString> failedScripts;
   TArray<FString> errorMessages;

   for(int32 i = 0; i < scriptFilenames.Num(); i++)
   {
      if(pFilterDelegate && !pFilterDelegate(scriptFilenames[i]))
      {
         continue;
      }

      const FString scriptPath = scriptsDir + scriptFilenames[i];

      if(!LoadScript(scriptPath))
      {
         const FString errorMessage = gEnv.getErrorMessage();

         failedScripts.Add(scriptFilenames[i]);
         errorMessages.Add(errorMessage);
      }
   }
   if (!failedScripts.IsEmpty())
   {
      for (int32 i = 0; i < smOnScriptReloadFailedCallbacks.Num(); i++)
      {
         smOnScriptReloadFailedCallbacks[i].mCallback(failedScripts, errorMessages);
      }
   }
}

void UnrealModule::RegisterFileWatcher(const FString& pFileExtension)
{
   // Set up script watcher for hot reloading
   const FString scriptsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts/");
   FDirectoryWatcherModule& directoryWatcherModule =
      FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

   auto onDirectoryChanged =
      IDirectoryWatcher::FDirectoryChanged::CreateLambda([&](const TArray<FFileChangeData>& pFileChanges)
      {
         // Using a TSet to filter out duplicates
         TSet<FString> modifiedScriptPaths;

         for(const FFileChangeData& fileChange : pFileChanges)
         {
            if(fileChange.Action == FFileChangeData::FCA_Modified && fileChange.Filename.EndsWith(pFileExtension))
            {
               modifiedScriptPaths.Add(fileChange.Filename);
            }
         }

         bool anyScriptReloaded = false;

         TArray<FString> reloadedScriptPaths;
         TArray<FString> failedScripts;
         TArray<FString> errorMessages;

         for(const FString& modifiedScriptPath : modifiedScriptPaths)
         {
            const bool success = LoadScript(modifiedScriptPath);

            if(success)
            {
               anyScriptReloaded = true;
               const FString title = FString(TEXT("Script Reloaded"));
               ShowNotification(true, title, modifiedScriptPath);
               reloadedScriptPaths.Add(modifiedScriptPath);
            }
            else
            {
               failedScripts.Add(FPaths::GetCleanFilename(modifiedScriptPath));
               const FString title = FString(TEXT("Script Reload Failed"));

               if (gEnv.getErrorMessage())
               {
                  const FString errorMessage(gEnv.getErrorMessage());
                  const FString message = FString::Printf(TEXT("%s\n\n%s"), *modifiedScriptPath, *errorMessage);
                  errorMessages.Add(errorMessage);
                  ShowNotification(false, title, message);
               }
               else
               {
                  errorMessages.Add("Loading Error");
                  ShowNotification(false, title, modifiedScriptPath);
               }
            }
         }

         if(anyScriptReloaded)
         {
            for(int32 i = 0; i < smOnScriptReloadedCallbacks.Num(); i++)
            {
               smOnScriptReloadedCallbacks[i].mCallback(reloadedScriptPaths);
            }
         }

         if (!failedScripts.IsEmpty())
         {
            for(int32 i = 0; i < smOnScriptReloadFailedCallbacks.Num(); i++)
            {
               smOnScriptReloadFailedCallbacks[i].mCallback(failedScripts, errorMessages);
            }
         }
      });

   FDelegateHandle delegateHandle;
   directoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(scriptsDir, onDirectoryChanged, delegateHandle, 0u);
}

void UnrealModule::CallFunction(Cflat::Function* pFunction, const CflatArgsVector(Cflat::Value)& pArgs,
   Cflat::Value* pOutReturnValue, OnFunctionCallErrorCallback pOnErrorCallback, void* pOnErrorCallbackData)
{
   pFunction->execute(pArgs, pOutReturnValue);

   if(gEnv.getErrorMessage() && pOnErrorCallback)
   {
      pOnErrorCallback(&gEnv, pFunction, pOnErrorCallbackData);
   }
}

FString UnrealModule::GetTypeNameAsString(const Cflat::Type* pType)
{
   static const FString kSeparator("::");

   FString typeName(pType->mNamespace->getFullIdentifier().mName);

   if(!typeName.IsEmpty())
   {
      typeName.Append(kSeparator);
   }

   typeName.Append(pType->mIdentifier.mName);

   if(pType->mCategory == TypeCategory::StructOrClass)
   {
      const Cflat::Struct* typeStruct = static_cast<const Cflat::Struct*>(pType);

      if(!typeStruct->mTemplateTypes.empty())
      {
         typeName += "<";

         for(size_t i = 0u; i < typeStruct->mTemplateTypes.size(); i++)
         {
            if(i > 0u)
            {
               typeName += ", ";
            }

            const Cflat::TypeUsage& templatedTypeUsage = typeStruct->mTemplateTypes[i];
            typeName += GetTypeUsageAsString(templatedTypeUsage);
         }

         typeName += ">";
      }
   }

   return typeName;
}

FString UnrealModule::GetTypeUsageAsString(const Cflat::TypeUsage& pTypeUsage)
{
   FString typeStr = GetTypeNameAsString(pTypeUsage.mType);

   if(pTypeUsage.isConst() || pTypeUsage.isConstPointer())
   {
      typeStr = "const " + typeStr;
   }

   for(uint8_t i = 0u; i < pTypeUsage.mPointerLevel; i++)
   {
      typeStr.AppendChar('*');
   }

   if(pTypeUsage.isReference())
   {
      typeStr.AppendChar('&');
   }

   if(pTypeUsage.isArray())
   {
      typeStr += "[" + FString::FromInt((int32)pTypeUsage.mArraySize) + "]";
   }

   return typeStr;
}

template<typename T>
static void AppendEnumValueToString(const Cflat::Value* pValue, FString* pOutValueStr)
{
   const Cflat::Type* valueType = pValue->mTypeUsage.mType;
   CflatSTLVector(Cflat::Instance*) enumInstances;
   
   if (valueType->mCategory == Cflat::TypeCategory::Enum)
   {
      static_cast<const Cflat::Enum*>(valueType)->mInstancesHolder.getAllInstances(&enumInstances);
   }
   else if (valueType->mCategory == Cflat::TypeCategory::EnumClass)
   {
      static_cast<const Cflat::EnumClass*>(valueType)->mInstancesHolder.getAllInstances(&enumInstances);
   }

   const T value = CflatValueAs(pValue, T);

   for (size_t i = 0u; i < enumInstances.size(); i++)
   {
      if (CflatValueAs(&enumInstances[i]->mValue, T) == value)
      {
         *pOutValueStr = FString(enumInstances[i]->mIdentifier.mName);
         pOutValueStr->AppendChar(' ');
         break;
      }
   }

   pOutValueStr->Append(FString::Printf(TEXT("(%d)"), value));
}

FString UnrealModule::GetValueAsString(const Cflat::Value* pValue)
{
   if(!pValue || !pValue->mValueBuffer)
   {
      static const FString kInvalid("<Invalid>");
      return kInvalid;
   }

   Cflat::Type* valueType = pValue->mTypeUsage.mType;
   FString valueStr;

   // Pointer
   if(pValue->mTypeUsage.isPointer())
   {
      const void* ptrAddress = CflatValueAs(pValue, void*);
      valueStr = FString::Printf(TEXT("0x%016llx"), ptrAddress);

      if(ptrAddress)
      {
         Cflat::TypeUsage referencedValueTypeUsage = pValue->mTypeUsage;
         referencedValueTypeUsage.mPointerLevel--;

         Cflat::Value referencedValue;
         referencedValue.initOnHeap(referencedValueTypeUsage);
         referencedValue.set(ptrAddress);

         valueStr += " -> " + GetValueAsString(&referencedValue);
      }
   }
   // Built-in types
   else if(valueType->mCategory == Cflat::TypeCategory::BuiltIn)
   {
      static const Cflat::Identifier kchar("char");
      static const Cflat::Identifier kbool("bool");
      static const Cflat::Identifier kfloat("float");

      // char array
      if(valueType->mIdentifier == kchar && pValue->mTypeUsage.isArray())
      {
         valueStr = FString(CflatValueAs(pValue, const char*));
      }
      // bool
      else if(valueType->mIdentifier == kbool)
      {
         valueStr = CflatValueAs(pValue, bool) ? FString("true") : FString("false");
      }
      // Integer
      else if(valueType->isInteger())
      {
         // Unsigned
         if(valueType->mIdentifier.mName[0] == 'u')
         {
            if(valueType->mSize == 4u)
            {
               valueStr = FString::Printf(TEXT("%u"), CflatValueAs(pValue, uint32));
            }
            else if(valueType->mSize == 8u)
            {
               valueStr = FString::Printf(TEXT("%llu"), CflatValueAs(pValue, uint64));
            }
            else if(valueType->mSize == 1u)
            {
               valueStr = FString::Printf(TEXT("%u"), (uint32)CflatValueAs(pValue, uint8));
            }
            else if(valueType->mSize == 2u)
            {
               valueStr = FString::Printf(TEXT("%u"), (uint32)CflatValueAs(pValue, uint16));
            }
         }
         // Signed
         else
         {
            if(valueType->mSize == 4u)
            {
               valueStr = FString::Printf(TEXT("%d"), CflatValueAs(pValue, int32));
            }
            else if(valueType->mSize == 8u)
            {
               valueStr = FString::Printf(TEXT("%lld"), CflatValueAs(pValue, int64));
            }
            else if(valueType->mSize == 1u)
            {
               valueStr = FString::Printf(TEXT("%d"), (int32)CflatValueAs(pValue, int8));
            }
            else if(valueType->mSize == 2u)
            {
               valueStr = FString::Printf(TEXT("%d"), (int32)CflatValueAs(pValue, int16));
            }
         }
      }
      // Floating point
      else
      {
         // float
         if(valueType->mIdentifier == kfloat)
         {
            valueStr = FString::SanitizeFloat((double)CflatValueAs(pValue, float));
         }
         // double
         else
         {
            valueStr = FString::SanitizeFloat(CflatValueAs(pValue, double));
         }
      }
   }
   // Enumeration
   else if(
      valueType->mCategory == Cflat::TypeCategory::Enum ||
      valueType->mCategory == Cflat::TypeCategory::EnumClass)
   {
      if(valueType->mSize == 1u)
      {
         AppendEnumValueToString<uint8>(pValue, &valueStr);
      }
      else if(valueType->mSize == 2u)
      {
         AppendEnumValueToString<uint16>(pValue, &valueStr);
      }
      else if(valueType->mSize == 4u)
      {
         AppendEnumValueToString<uint32>(pValue, &valueStr);
      }
      else if(valueType->mSize == 8u)
      {
         AppendEnumValueToString<uint64>(pValue, &valueStr);
      }
      else
      {
         CflatAssert(false); // Unsupported
      }
   }
   // Struct or class
   else
   {
      valueStr = GetTypeNameAsString(valueType) + " { ";

      static const Cflat::Identifier kFName("FName");
      static const Cflat::Identifier kFString("FString");

      // FName
      if(valueType->mIdentifier == kFName)
      {
         const FName name = CflatValueAs(pValue, FName);
         valueStr += "\"" + name.ToString() + "\" (" + FString::FromInt(name.GetNumber()) + ")";
      }
      // FString
      else if(valueType->mIdentifier == kFString)
      {
         const FString& string = CflatValueAs(pValue, FString);
         valueStr += "\"" + string + "\"";
      }
      // Generic
      else
      {
         Cflat::Struct* valueStruct = static_cast<Cflat::Struct*>(valueType);

         for(size_t i = 0u; i < valueStruct->mMembers.size(); i++)
         {
            const Cflat::Member& member = valueStruct->mMembers[i];

            if(i > 0u)
            {
               valueStr += ", ";
            }

            Cflat::Value memberValue;
            memberValue.initExternal(member.mTypeUsage);
            memberValue.set(pValue->mValueBuffer + member.mOffset);

            valueStr += FString(member.mIdentifier.mName) + "=" + GetValueAsString(&memberValue);
         }
      }

      valueStr += " }";
   }

   return valueStr;
}

FString UnrealModule::GetMemberAsString(const Cflat::Member* pMember)
{
   FString memberStr = GetTypeUsageAsString(pMember->mTypeUsage);
   memberStr.Append(" ");
   memberStr.Append(pMember->mIdentifier.mName);
   return memberStr;
}

FString GetTemplateOrTypeUsageAsString(const CflatSTLVector(TypeUsage)& pTemplates, const TypeUsage& pTypeUsage)
{
   for (size_t i = 0; i < pTemplates.size(); ++i)
   {
      if (pTemplates[i].mType == pTypeUsage.mType)
      {
         FString typeStr = pTemplates.size() == 1 ? "T" : FString::Printf(TEXT("%s%d"), TEXT("T"), (int32)i);

         if (pTypeUsage.isConst() || pTypeUsage.isConstPointer())
         {
            typeStr = "const " + typeStr;
         }

         for (uint8_t pi = 0u; pi < pTypeUsage.mPointerLevel; pi++)
         {
            typeStr.AppendChar('*');
         }

         if (pTypeUsage.isReference())
         {
            typeStr.AppendChar('&');
         }

         return typeStr;
      }
   }
   return UnrealModule::GetTypeUsageAsString(pTypeUsage);
}

FString UnrealModule::GetMethodAsString(const Cflat::Method* pMethod)
{
   FString methodStr = "";

   if (pMethod->mTemplateTypes.size() > 0)
   {
      int32 templateTypeCount = pMethod->mTemplateTypes.size();
      FString typeT = "T";
      methodStr.Append("template<");
      for (size_t i = 0; i < pMethod->mTemplateTypes.size(); ++i)
      {
         if (i != 0)
         {
            methodStr.Append(", ");
         }
         methodStr.Append("typename ");
         if (templateTypeCount == 1)
         {
            methodStr.Append(typeT);
         }
         else
         {
            methodStr.Append(FString::Printf(TEXT("%s%d"), *typeT, (int32)i));
         }
      }
      methodStr.Append("> ");
   }

   if (pMethod->mReturnTypeUsage.mType)
   {
      methodStr.Append(GetTemplateOrTypeUsageAsString(pMethod->mTemplateTypes, pMethod->mReturnTypeUsage));
   }
   else
   {
      methodStr.Append("void");
   }
   methodStr.Append(" ");
   methodStr.Append(pMethod->mIdentifier.mName);
   methodStr.Append("(");
   for (size_t i = 0; i < pMethod->mParameters.size(); ++i)
   {
      const TypeUsage& typeUsage = pMethod->mParameters[i];
      if (i != 0)
      {
         methodStr.Append(", ");
      }
      methodStr.Append(GetTemplateOrTypeUsageAsString(pMethod->mTemplateTypes, typeUsage));
   }
   methodStr.Append(")");

   if (CflatHasFlag(pMethod->mFlags, Cflat::MethodFlags::Const))
   {
      methodStr.Append(" const");
   }

   return methodStr;
}

FString UnrealModule::GetFunctionAsString(const Cflat::Function* pFunction)
{
   FString functionStr = "";
   if (pFunction->mReturnTypeUsage.mType)
   {
      functionStr.Append(GetTypeUsageAsString(pFunction->mReturnTypeUsage));
   }
   else
   {
      functionStr.Append("void");
   }
   functionStr.Append(" ");
   functionStr.Append(pFunction->mIdentifier.mName);
   functionStr.Append("(");
   for (size_t i = 0; i < pFunction->mParameters.size(); ++i)
   {
      const TypeUsage& typeUsage = pFunction->mParameters[i];
      if (i != 0)
      {
         functionStr.Append(", ");
      }
      functionStr.Append(GetTypeUsageAsString(typeUsage));
   }
   functionStr.Append(")");

   return functionStr;
}

bool UnrealModule::LoadScript(const FString& pFilePath)
{
   FString scriptCode;
   const TCHAR* tcharFilePath = *pFilePath;
   const FFileHelper::EHashOptions verifyFlags = FFileHelper::EHashOptions::None;
   const uint32 readFlags = FILEREAD_AllowWrite;

   // There's no need to write anything in the file, but passing 'FILEREAD_AllowWrite' helps
   // preventing ERROR_SHARING_VIOLATION on Windows when calling 'CreateFileW' (error code 32)
 
   if(!FFileHelper::LoadFileToString(scriptCode, tcharFilePath, verifyFlags, readFlags))
   {
      UE_LOG(LogTemp, Error, TEXT("[Cflat] The script file ('%s') could not be read"), tcharFilePath);

      return false;
   }

   const FString fileName = FPaths::GetCleanFilename(pFilePath);
   UE_LOG(LogTemp, Display, TEXT("[Cflat] Loading script '%s'..."), *fileName);

   if(!gEnv.load(TCHAR_TO_ANSI(*fileName), TCHAR_TO_ANSI(*scriptCode)))
   {
      const FString errorMessage(gEnv.getErrorMessage());
      UE_LOG(LogTemp, Error, TEXT("[Cflat] %s"), *errorMessage);

      return false;
   }

   return true;
}
}

#endif


///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.60
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2024 Arturo Cepeda Pérez and contributors
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

void UnrealModule::AutoRegisterCflatTypes(const TSet<FName>& pModules, const TArray<FString>& pAidIncludes)
{
   AutoRegister::TypesRegister typeRegister(&gEnv);
   typeRegister.mTimeStarted = FPlatformTime::Seconds();
   typeRegister.mAllowedModules = pModules;
   // These are typedefd or manually registered
   typeRegister.mHeaderEnumsToIgnore =
   {
      FName("ETeleportType"),
      FName("ECollisionChannel"),
      FName("ESpawnActorCollisionHandlingMethod"),
      FName("ESpawnActorScaleMethod")
   };
   typeRegister.mHeaderStructsToIgnore =
   {
      FName("HitResult")
   };
   typeRegister.mHeaderClassesToIgnore =
   {
      FName("Object"),
      FName("Field"),
      FName("Struct"),
      FName("Class"),
      FName("Actor"),
      FName("Pawn"),
      FName("World"),
      FName("SceneComponent"),
      FName("ActorComponent"),
      FName("ULineBatchComponent")
   };

   typeRegister.RegisterEnums();
   typeRegister.RegisterStructs();
   typeRegister.RegisterClasses();
   typeRegister.RegisterProperties();
   typeRegister.RegisterFunctions();

   {
      const FString aidFileDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts");
      typeRegister.GenerateAidHeader(aidFileDir, pAidIncludes);
   }

   const bool printDebug = false;
   if (printDebug)
   {
      typeRegister.PrintDebugStats();
   }
}

void UnrealModule::RegisterTypes()
{
   {
      CflatRegisterTArray(&gEnv, FHitResult);
   }
   {
      // AActor - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("AActor"));
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorLocation);
      CflatClassAddMethodReturn(&gEnv, AActor, FRotator, GetActorRotation);
      CflatClassAddMethodReturn(&gEnv, AActor, FQuat, GetActorQuat);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorScale3D);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorForwardVector);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorUpVector);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorRightVector);
      // @LB TODO Remap K2_GetRootComponent to this function
      CflatClassAddMethodReturn(&gEnv, AActor, USceneComponent*, GetRootComponent);
      // @LB TODO Support TSubclassOf<T>
      CflatClassAddMethodReturnParams1(&gEnv, AActor, UActorComponent*, GetComponentByClass, UClass*);
      // @LB TODO Remap K2_SetActorLocation to this function, using default parameters
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorLocation, const FVector&);
      // @LB TODO Remap K2_SetActorRotation to this function, using default parameters
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorRotation, FRotator);
   }
   {
      CflatRegisterClass(&gEnv, ULineBatchComponent);
      CflatClassAddBaseType(&gEnv, ULineBatchComponent, USceneComponent);
      CflatClassAddMethodVoidParams6(&gEnv, ULineBatchComponent, void, DrawBox, const FVector&, const FVector&, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawBox, const FVector&, const FVector&, const FQuat&, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams6(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8, float, float);
      CflatClassAddMethodVoidParams5(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8, float);
      CflatClassAddMethodVoidParams4(&gEnv, ULineBatchComponent, void, DrawLine, const FVector&, const FVector&, const FLinearColor&, uint8);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawDirectionalArrow, const FVector&, const FVector&, float, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawCircle, const FVector&, const FVector&, const FVector&, FLinearColor, float, int32, uint8);
      CflatClassAddMethodVoidParams7(&gEnv, ULineBatchComponent, void, DrawSphere, const FVector&, float, int32, FLinearColor, float, uint8, float);
      CflatClassAddMethodVoidParams8(&gEnv, ULineBatchComponent, void, DrawCapsule, const FVector&, float, float, const FQuat&, FLinearColor, float, uint8, float);
   }
   {
      CflatRegisterTObjectPtr(&gEnv, ULineBatchComponent);
   }

   {
      CflatRegisterEnum(&gEnv, ECollisionChannel);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_WorldStatic);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_WorldDynamic);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_Pawn);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_Visibility);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_Camera);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_PhysicsBody);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_Vehicle);
      CflatEnumAddValue(&gEnv, ECollisionChannel, ECC_Destructible);
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

  CflatRegisterTArray(&gEnv, FVector);
  CflatRegisterTArray(&gEnv, FRotator);
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
      CflatRegisterBuiltInType(&gEnv, uint8);
      CflatRegisterBuiltInType(&gEnv, uint16);
      CflatRegisterBuiltInType(&gEnv, uint32);
      CflatRegisterBuiltInType(&gEnv, uint64);
      CflatRegisterBuiltInType(&gEnv, int8);
      CflatRegisterBuiltInType(&gEnv, int16);
      CflatRegisterBuiltInType(&gEnv, int32);
      CflatRegisterBuiltInType(&gEnv, int64);
   }

   {
      CflatRegisterClass(&gEnv, FString);
      CflatClassAddConstructor(&gEnv, FString);
      CflatClassAddConstructorParams1(&gEnv, FString, const char*);
      CflatClassAddCopyConstructor(&gEnv, FString);
      CflatClassAddMethodReturn(&gEnv, FString, const TCHAR*, operator*);
   }
   {
      CflatRegisterClass(&gEnv, FName);
      CflatClassAddConstructor(&gEnv, FName);
      CflatClassAddConstructorParams1(&gEnv, FName, const char*);
      CflatClassAddCopyConstructor(&gEnv, FName);
      CflatClassAddMethodReturn(&gEnv, FName, FString, ToString);
      CflatClassAddMethodVoidParams1(&gEnv, FName, void, ToString, FString&);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator==, FName);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator!=, FName);
   }
   {
      CflatRegisterClass(&gEnv, FText);
      CflatClassAddConstructor(&gEnv, FText);
      CflatClassAddCopyConstructor(&gEnv, FText);
      CflatClassAddStaticMethodReturnParams1(&gEnv, FText, FText, FromString, const FString&);
      CflatStructAddStaticMethodReturn(&gEnv, FText, const FText&, GetEmpty);
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

      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator+, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator-, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator*, double);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator/, double);
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
   }
   {
      CflatRegisterStruct(&gEnv, FVector2D);
      CflatStructAddConstructor(&gEnv, FVector2D);
      CflatStructAddConstructorParams2(&gEnv, FVector2D, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector2D);
      CflatStructAddMember(&gEnv, FVector2D, double, X);
      CflatStructAddMember(&gEnv, FVector2D, double, Y);
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
   }
   {
      CflatRegisterStruct(&gEnv, FRotator);
      CflatStructAddConstructor(&gEnv, FRotator);
      CflatStructAddConstructorParams3(&gEnv, FRotator, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FRotator);
      CflatStructAddMember(&gEnv, FRotator, double, Pitch);
      CflatStructAddMember(&gEnv, FRotator, double, Yaw);
      CflatStructAddMember(&gEnv, FRotator, double, Roll);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator+, const FRotator&);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator-, const FRotator&);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator*, double);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, FRotator, operator*=, double);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, bool, operator==, const FRotator&);
      CflatStructAddMethodReturnParams1(&gEnv, FRotator, bool, operator!=, const FRotator&);
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
   }


   RegisterTArrays();
   RegisterFGenericPlatformMath();
   RegisterFMath();


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

void UnrealModule::LoadScripts()
{
   // Load all scripts
   const FString scriptsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts/");
   const FString scriptsExt = TEXT("cpp");

   TArray<FString> scriptFilenames;
   IFileManager::Get().FindFiles(scriptFilenames, *scriptsDir, *scriptsExt);

   for(int32 i = 0; i < scriptFilenames.Num(); i++)
   {
      const FString scriptPath = scriptsDir + scriptFilenames[i];

      if(!LoadScript(scriptPath))
      {
         const FText errorTitle = FText::FromString(TEXT("Cflat Error"));
         const FText errorMessage = FText::FromString(gEnv.getErrorMessage());
         FMessageDialog::Open(EAppMsgType::Ok, errorMessage, &errorTitle);
         abort();
      }
   }
}

void UnrealModule::RegisterFileWatcher()
{
   // Set up script watcher for hot reloading
   const FString scriptsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Scripts/");
   FDirectoryWatcherModule& directoryWatcherModule =
      FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

   auto onDirectoryChanged =
      IDirectoryWatcher::FDirectoryChanged::CreateLambda([&](const TArray<FFileChangeData>& pFileChanges)
      {
         const FString scriptsExt = TEXT("cpp");
         // Using a TSet to filter out duplicates
         TSet<FString> modifiedScriptPaths;

         for(const FFileChangeData& fileChange : pFileChanges)
         {
            if(fileChange.Action == FFileChangeData::FCA_Modified && fileChange.Filename.EndsWith(scriptsExt))
            {
               modifiedScriptPaths.Add(fileChange.Filename);
            }
         }

         for(const FString& modifiedScriptPath : modifiedScriptPaths)
         {
            const bool success = LoadScript(modifiedScriptPath);
            if (success)
            {
               const FString title = FString(TEXT("Script Reloaded"));
               ShowNotification(true, title, modifiedScriptPath);
            }
            else
            {
               const FString title = FString(TEXT("Script Reload Failed"));

               if(gEnv.getErrorMessage())
               {
                  const FString errorMessage(gEnv.getErrorMessage());
                  const FString message = FString::Printf(TEXT("%s\n\n%s"), *modifiedScriptPath, *errorMessage);
                  ShowNotification(false, title, message);
               }
               else
               {
                  ShowNotification(false, title, modifiedScriptPath);
               }
            }
         }
      });

   FDelegateHandle delegateHandle;
   directoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(scriptsDir, onDirectoryChanged, delegateHandle, 0u);
}

void UnrealModule::CallFunction(Cflat::Function* pFunction,
   const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
{
   pFunction->execute(pArgs, pOutReturnValue);

   if(gEnv.getErrorMessage())
   {
      const FString errorMessage(gEnv.getErrorMessage());
      UE_LOG(LogTemp, Error, TEXT("[Cflat] %s"), *errorMessage);
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

   return typeName;
}

FString UnrealModule::GetTypeUsageAsString(const Cflat::TypeUsage& pTypeUsage)
{
   FString typeStr = GetTypeNameAsString(pTypeUsage.mType);

   if(pTypeUsage.isConst())
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
   const CflatSTLVector(Cflat::Instance*)& enumInstances = valueType->mCategory == Cflat::TypeCategory::Enum
      ? static_cast<const Cflat::Enum*>(valueType)->mInstances
      : static_cast<const Cflat::EnumClass*>(valueType)->mInstances;
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
   Cflat::Type* valueType = pValue->mTypeUsage.mType;
   FString valueStr;

   // Pointer
   if(pValue->mTypeUsage.isPointer())
   {
      const void* ptrAddress = CflatValueAs(pValue, void*);

      Cflat::TypeUsage referencedValueTypeUsage = pValue->mTypeUsage;
      referencedValueTypeUsage.mPointerLevel--;

      Cflat::Value referencedValue;
      referencedValue.initOnHeap(referencedValueTypeUsage);
      referencedValue.set(ptrAddress);

      valueStr = FString::Printf(TEXT("0x%016llx"), ptrAddress);
      valueStr += " -> " + GetValueAsString(&referencedValue);
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

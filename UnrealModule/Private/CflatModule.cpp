
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
      CflatClassAddConstructorParams1(&gEnv, FString, const char*);
      CflatClassAddCopyConstructor(&gEnv, FString);
      CflatClassAddMethodReturn(&gEnv, FString, const TCHAR*, operator*);
   }
   {
      CflatRegisterClass(&gEnv, FName);
      CflatClassAddConstructorParams1(&gEnv, FName, const char*);
      CflatClassAddCopyConstructor(&gEnv, FName);
      CflatClassAddMethodReturn(&gEnv, FName, FString, ToString);
      CflatClassAddMethodVoidParams1(&gEnv, FName, void, ToString, FString&);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator==, FName);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator!=, FName);
   }
   {
      CflatRegisterClass(&gEnv, FText);
      CflatClassAddCopyConstructor(&gEnv, FText);
      CflatClassAddStaticMethodReturnParams1(&gEnv, FText, FText, FromString, const FString&);
      CflatStructAddStaticMethodReturn(&gEnv, FText, const FText&, GetEmpty);
   }

   {
      CflatRegisterStruct(&gEnv, FVector);
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
   }
   {
      CflatRegisterStruct(&gEnv, FVector2D);
      CflatStructAddConstructorParams2(&gEnv, FVector2D, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector2D);
      CflatStructAddMember(&gEnv, FVector2D, double, X);
      CflatStructAddMember(&gEnv, FVector2D, double, Y);
   }
   {
      CflatRegisterStruct(&gEnv, FQuat);
      CflatStructAddConstructorParams4(&gEnv, FQuat, double, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FQuat);
      CflatStructAddMember(&gEnv, FQuat, double, X);
      CflatStructAddMember(&gEnv, FQuat, double, Y);
      CflatStructAddMember(&gEnv, FQuat, double, Z);
      CflatStructAddMember(&gEnv, FQuat, double, W);
   }
   {
      CflatRegisterStruct(&gEnv, FRotator);
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
      CflatStructAddConstructorParams3(&gEnv, FLinearColor, float, float, float);
      CflatStructAddConstructorParams4(&gEnv, FLinearColor, float, float, float, float);
      CflatStructAddCopyConstructor(&gEnv, FLinearColor);
      CflatStructAddMember(&gEnv, FLinearColor, float, R);
      CflatStructAddMember(&gEnv, FLinearColor, float, G);
      CflatStructAddMember(&gEnv, FLinearColor, float, B);
      CflatStructAddMember(&gEnv, FLinearColor, float, A);
   }

   {
      // UClass - forward declaration
      CflatRegisterClass(&gEnv, UClass);
   }
   {
      // UWorld - forward declaration
      CflatRegisterClass(&gEnv, UWorld)
   }
   {
      CflatRegisterClass(&gEnv, UObject);
      // UObjectBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, UClass*, GetClass);
      // UObjectBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, FName, GetFName);
      // UObjectUtilityBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, FString, GetName);
      CflatClassAddMethodReturn(&gEnv, UObject, UWorld*, GetWorld);
   }
   {
      CflatRegisterClass(&gEnv, UField);
      CflatClassAddBaseType(&gEnv, UField, UObject);
   }
   {
      CflatRegisterClass(&gEnv, UStruct);
      CflatClassAddBaseType(&gEnv, UStruct, UField);
   }
   {
      // UClass - type definition
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("UClass"));
      CflatClassAddBaseType(&gEnv, UClass, UField);
   }
   {
      CflatRegisterClass(&gEnv, AActor);
      CflatClassAddBaseType(&gEnv, AActor, UObject);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorLocation);
      CflatClassAddMethodReturn(&gEnv, AActor, FRotator, GetActorRotation);
      CflatClassAddMethodReturn(&gEnv, AActor, FQuat, GetActorQuat);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorScale3D);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorForwardVector);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorUpVector);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorRightVector);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorLocation, const FVector&);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorRotation, FRotator);
      CflatClassAddMethodReturnParams2(&gEnv, AActor, bool, SetActorLocationAndRotation, FVector, FRotator);
      CflatClassAddMethodVoidParams1(&gEnv, AActor, void, SetActorScale3D, FVector);
   }
   {
      CflatRegisterClass(&gEnv, APawn);
      CflatClassAddBaseType(&gEnv, APawn, AActor);
   }
   {
      CflatRegisterClass(&gEnv, UActorComponent);
      CflatClassAddBaseType(&gEnv, UActorComponent, UObject);
      CflatClassAddMethodReturn(&gEnv, UActorComponent, AActor*, GetOwner);
   }
   {
      CflatRegisterClass(&gEnv, USceneComponent);
      CflatClassAddBaseType(&gEnv, USceneComponent, UActorComponent);
      CflatClassAddStaticMethodReturn(&gEnv, USceneComponent, UClass*, StaticClass);
      CflatClassAddMethodVoidParams1(&gEnv, USceneComponent, void, SetVisibility, bool);
      CflatClassAddMethodVoidParams2(&gEnv, USceneComponent, void, SetVisibility, bool, bool);
   }
   {
      // AActor - type extension
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("AActor"));
      CflatClassAddMethodReturn(&gEnv, AActor, USceneComponent*, GetRootComponent);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, UActorComponent*, GetComponentByClass, UClass*);
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
      CflatRegisterStruct(&gEnv, FHitResult);
      CflatStructAddConstructor(&gEnv, FHitResult);
      CflatStructAddMember(&gEnv, FHitResult, int32, FaceIndex);
      CflatStructAddMember(&gEnv, FHitResult, float, Time);
      CflatStructAddMember(&gEnv, FHitResult, float, Distance);
      CflatStructAddMember(&gEnv, FHitResult, FVector, Location);
      CflatStructAddMember(&gEnv, FHitResult, FVector, ImpactPoint);
      CflatStructAddMember(&gEnv, FHitResult, FVector, Normal);
      CflatStructAddMember(&gEnv, FHitResult, FVector, ImpactNormal);
      CflatStructAddMember(&gEnv, FHitResult, FVector, TraceStart);
      CflatStructAddMember(&gEnv, FHitResult, FVector, TraceEnd);
      CflatStructAddMethodReturn(&gEnv, FHitResult, AActor*, GetActor);
   }
   {
      CflatRegisterTArray(&gEnv, FHitResult);
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
      CflatRegisterEnumClass(&gEnv, ESpawnActorCollisionHandlingMethod);
      CflatEnumClassAddValue(&gEnv, ESpawnActorCollisionHandlingMethod, Undefined);
      CflatEnumClassAddValue(&gEnv, ESpawnActorCollisionHandlingMethod, AlwaysSpawn);
      CflatEnumClassAddValue(&gEnv, ESpawnActorCollisionHandlingMethod, AdjustIfPossibleButAlwaysSpawn);
      CflatEnumClassAddValue(&gEnv, ESpawnActorCollisionHandlingMethod, AdjustIfPossibleButDontSpawnIfColliding);
      CflatEnumClassAddValue(&gEnv, ESpawnActorCollisionHandlingMethod, DontSpawnIfColliding);
   }
   {
      CflatRegisterEnumClass(&gEnv, ESpawnActorScaleMethod);
      CflatEnumClassAddValue(&gEnv, ESpawnActorScaleMethod, OverrideRootScale);
      CflatEnumClassAddValue(&gEnv, ESpawnActorScaleMethod, MultiplyWithRoot);
      CflatEnumClassAddValue(&gEnv, ESpawnActorScaleMethod, SelectDefaultAtRuntime);
   }
   {
      CflatRegisterStruct(&gEnv, FActorSpawnParameters);
      CflatStructAddConstructor(&gEnv, FActorSpawnParameters);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, FName, Name);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, AActor*, Template);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, AActor*, Owner);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, APawn*, Instigator);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, ESpawnActorCollisionHandlingMethod, SpawnCollisionHandlingOverride);
      CflatStructAddMember(&gEnv, FActorSpawnParameters, ESpawnActorScaleMethod, TransformScaleMethod);
   }
   {
      // UWorld - type definition
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("UWorld"));
      CflatClassAddBaseType(&gEnv, UWorld, UObject);
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
      CflatClassAddMethodReturnParams4(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FVector*, const FRotator*, const FActorSpawnParameters&);
      CflatClassAddMethodReturnParams2(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FTransform*);
      CflatClassAddMethodReturnParams3(&gEnv, UWorld, AActor*, SpawnActor, UClass*, const FTransform*, const FActorSpawnParameters&);
      CflatClassAddMethodReturnParams2(&gEnv, UWorld, AActor*, SpawnActorAbsolute, UClass*, const FTransform&);
      CflatClassAddMethodReturnParams3(&gEnv, UWorld, AActor*, SpawnActorAbsolute, UClass*, const FTransform&, const FActorSpawnParameters&);
      CflatClassAddTemplateMethodReturnParams2(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&);
      CflatClassAddTemplateMethodReturnParams3(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*);
      CflatClassAddTemplateMethodReturnParams4(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*);
      CflatClassAddTemplateMethodReturnParams5(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*, ESpawnActorCollisionHandlingMethod);
      CflatClassAddTemplateMethodReturnParams6(&gEnv, UWorld, AActor, AActor*, SpawnActorDeferred, UClass*, const FTransform&, AActor*, APawn*, ESpawnActorCollisionHandlingMethod, ESpawnActorScaleMethod);
      CflatClassAddMethodReturnParams1(&gEnv, UWorld, bool, DestroyActor, AActor*);
   }

   {
      CflatRegisterClass(&gEnv, UGameplayStatics);
      CflatClassAddStaticMethodReturnParams2(&gEnv, UGameplayStatics, AActor*, FinishSpawningActor, AActor*, const FTransform&);
      CflatClassAddStaticMethodReturnParams3(&gEnv, UGameplayStatics, AActor*, FinishSpawningActor, AActor*, const FTransform&, ESpawnActorScaleMethod);
   }

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

      CflatRegisterTArray(&gEnv, FVector);
      CflatRegisterTArray(&gEnv, FRotator);
   }

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

   // Set up script watcher for hot reloading
   FDirectoryWatcherModule& directoryWatcherModule =
      FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

   auto onDirectoryChanged =
      IDirectoryWatcher::FDirectoryChanged::CreateLambda([&](const TArray<FFileChangeData>& pFileChanges)
      {
         // Using a TSet to filter out duplicates
         TSet<FString> modifiedScriptPaths;

         for(const FFileChangeData& fileChange : pFileChanges)
         {
            if(fileChange.Action == FFileChangeData::FCA_Modified)
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

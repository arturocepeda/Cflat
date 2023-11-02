
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.50
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2023 Arturo Cepeda P�rez
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


#include "CflatModule.h"

// Cflat source code
#include "../../Cflat.cpp"

// Standard includes
#include <mutex>

// UE includes - Module manager
#include "Modules/ModuleManager.h"

// UE includes - File watcher
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"

// UE includes - Engine types
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// UE includes - Misc
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"


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
void UnrealModule::Init()
{
   {
      gEnv.defineMacro("TEXT(x)", "L##x");
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
      CflatRegisterClass(&gEnv, FName);
      CflatClassAddConstructorParams1(&gEnv, FName, const char*);
   }

   {
      CflatRegisterStruct(&gEnv, FVector);
      CflatStructAddConstructorParams3(&gEnv, FVector, float, float, float);
   }
   {
      CflatRegisterStruct(&gEnv, FVector2D);
      CflatStructAddConstructorParams2(&gEnv, FVector2D, float, float);
   }
   {
      CflatRegisterStruct(&gEnv, FRotator);
      CflatStructAddConstructorParams3(&gEnv, FRotator, float, float, float);
   }
   {
      CflatRegisterStruct(&gEnv, FColor);
      CflatStructAddConstructorParams3(&gEnv, FColor, uint8, uint8, uint8);
      CflatStructAddConstructorParams4(&gEnv, FColor, uint8, uint8, uint8, uint8);
      CflatStructAddMember(&gEnv, FColor, uint8, R);
      CflatStructAddMember(&gEnv, FColor, uint8, G);
      CflatStructAddMember(&gEnv, FColor, uint8, B);
      CflatStructAddMember(&gEnv, FColor, uint8, A);
   }
   {
      CflatRegisterStruct(&gEnv, FLinearColor);
      CflatStructAddConstructorParams3(&gEnv, FLinearColor, float, float, float);
      CflatStructAddConstructorParams4(&gEnv, FLinearColor, float, float, float, float);
      CflatStructAddMember(&gEnv, FLinearColor, float, R);
      CflatStructAddMember(&gEnv, FLinearColor, float, G);
      CflatStructAddMember(&gEnv, FLinearColor, float, B);
      CflatStructAddMember(&gEnv, FLinearColor, float, A);
   }

   {
      CflatRegisterClass(&gEnv, UClass);
   }
   {
      CflatRegisterClass(&gEnv, UActorComponent);
   }
   {
      CflatRegisterClass(&gEnv, USceneComponent);
      CflatClassAddBaseType(&gEnv, USceneComponent, UActorComponent);
      CflatClassAddStaticMethodReturn(&gEnv, USceneComponent, UClass*, StaticClass);
      CflatClassAddMethodVoidParams1(&gEnv, USceneComponent, void, SetVisibility, bool);
      CflatClassAddMethodVoidParams2(&gEnv, USceneComponent, void, SetVisibility, bool, bool);
   }

   {
      CflatRegisterClass(&gEnv, AActor);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorLocation, const FVector&);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorRotation, FRotator);
      CflatClassAddMethodReturnParams2(&gEnv, AActor, bool, SetActorLocationAndRotation, FVector, FRotator);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, UActorComponent*, GetComponentByClass, UClass*);
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

      if(!gEnv.load(TCHAR_TO_ANSI(*scriptPath)))
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
            const FString filename = FPaths::GetCleanFilename(modifiedScriptPath);
            UE_LOG(LogTemp, Display, TEXT("[Cflat] Hot reloading script '%s'..."), *filename);

            if(!gEnv.load(TCHAR_TO_ANSI(*modifiedScriptPath)))
            {
               const FString errorMessage(gEnv.getErrorMessage());
               UE_LOG(LogTemp, Error, TEXT("[Cflat] %s"), *errorMessage);
            }
         }
      });

   FDelegateHandle delegateHandle;
   directoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(scriptsDir, onDirectoryChanged, delegateHandle, 0u);
}
}

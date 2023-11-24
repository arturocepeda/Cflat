
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.50
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2023 Arturo Cepeda Pérez
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
      CflatClassAddCopyConstructor(&gEnv, FName);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator==, FName);
      CflatClassAddMethodReturnParams1(&gEnv, FName, bool, operator!=, FName);
   }
   {
      CflatRegisterClass(&gEnv, FString);
      CflatClassAddConstructorParams1(&gEnv, FString, const char*);
      CflatClassAddCopyConstructor(&gEnv, FString);
   }

   {
      CflatRegisterStruct(&gEnv, FVector);
      CflatStructAddConstructorParams3(&gEnv, FVector, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector);
      CflatStructAddMember(&gEnv, FVector, double, X);
      CflatStructAddMember(&gEnv, FVector, double, Y);
      CflatStructAddMember(&gEnv, FVector, double, Z);
      CflatStructAddMethodVoid(&gEnv, FVector, FVector, GetUnsafeNormal);
      CflatStructAddMethodReturn(&gEnv, FVector, bool, Normalize);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, bool, Normalize, double);
      CflatStructAddMethodReturn(&gEnv, FVector, double, Length);
      CflatStructAddMethodReturn(&gEnv, FVector, double, SquaredLength);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator+, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator-, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator*, double);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator/, double);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator+=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator-=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator*=, const FVector&);
      CflatStructAddMethodReturnParams1(&gEnv, FVector, FVector, operator/=, const FVector&);
   }
   {
      CflatRegisterStruct(&gEnv, FVector2D);
      CflatStructAddConstructorParams2(&gEnv, FVector2D, double, double);
      CflatStructAddCopyConstructor(&gEnv, FVector2D);
      CflatStructAddMember(&gEnv, FVector2D, double, X);
      CflatStructAddMember(&gEnv, FVector2D, double, Y);
   }
   {
      CflatRegisterStruct(&gEnv, FRotator);
      CflatStructAddConstructorParams3(&gEnv, FRotator, double, double, double);
      CflatStructAddCopyConstructor(&gEnv, FRotator);
      CflatStructAddMember(&gEnv, FRotator, double, Pitch);
      CflatStructAddMember(&gEnv, FRotator, double, Yaw);
      CflatStructAddMember(&gEnv, FRotator, double, Roll);
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
      CflatRegisterClass(&gEnv, UClass);
   }
   {
      CflatRegisterClass(&gEnv, UObject);
      // UObjectBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, UClass*, GetClass);
      // UObjectBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, FName, GetFName);
      // UObjectUtilityBase method, added to UObject for simplicity
      CflatClassAddMethodReturn(&gEnv, UObject, FString, GetName);
   }
   {
      CflatRegisterClass(&gEnv, AActor);
      CflatClassAddBaseType(&gEnv, AActor, UObject);
      CflatClassAddMethodReturn(&gEnv, AActor, FVector, GetActorLocation);
      CflatClassAddMethodReturn(&gEnv, AActor, FRotator, GetActorRotation);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorLocation, const FVector&);
      CflatClassAddMethodReturnParams1(&gEnv, AActor, bool, SetActorRotation, FRotator);
      CflatClassAddMethodReturnParams2(&gEnv, AActor, bool, SetActorLocationAndRotation, FVector, FRotator);
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
      Cflat::Class* type = static_cast<Cflat::Class*>(gEnv.getGlobalNamespace()->getType("AActor"));
      CflatClassAddMethodReturn(&gEnv, AActor, USceneComponent*, GetRootComponent);
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
            LoadScript(modifiedScriptPath);
         }
      });

   FDelegateHandle delegateHandle;
   directoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(scriptsDir, onDirectoryChanged, delegateHandle, 0u);
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

FString UnrealModule::GetValueAsString(const Cflat::Value* pValue)
{
   Cflat::Type* valueType = pValue->mTypeUsage.mType;
   FString valueStr;

   // Pointer
   if(pValue->mTypeUsage.isPointer())
   {
      const uint64 ptrAddress = CflatValueAs(pValue, uint64);
      valueStr = FString::Printf(TEXT("0x%llx"), ptrAddress);
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
      const CflatSTLVector(Cflat::Instance*)& enumInstances = valueType->mCategory == Cflat::TypeCategory::Enum
         ? static_cast<Cflat::Enum*>(valueType)->mInstances
         : static_cast<Cflat::EnumClass*>(valueType)->mInstances;
      const int value = CflatValueAs(pValue, int);

      for(size_t i = 0u; i < enumInstances.size(); i++)
      {
         if(CflatValueAs(&enumInstances[i]->mValue, int) == value)
         {
            valueStr = FString(enumInstances[i]->mIdentifier.mName);
            valueStr.AppendChar(' ');
            break;
         }
      }

      valueStr += FString::Printf(TEXT("(%d)"), value);
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

   if(!FFileHelper::LoadFileToString(scriptCode, tcharFilePath))
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

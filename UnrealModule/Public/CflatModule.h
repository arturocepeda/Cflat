
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


#pragma once


#if defined (CFLAT_ENABLED)

// MSVC specifics
#if defined (_MSC_VER)
# define CflatAPI CFLAT_API   // __declspec(dllexport) / __declspec(dllimport)
# pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS
#endif

// Use Unreal's check as assert
#define CflatAssert check

// Cflat includes
#include "../../CflatGlobal.h"
#include "../../Cflat.h"


//
//  UnrealModule static class
//
namespace Cflat
{
class CFLAT_API UnrealModule
{
public:
   typedef std::function<void()> OnScriptReloadedCallback;

   typedef void (*OnFunctionCallErrorCallback)(Cflat::Environment* pEnv, Cflat::Function* pFunction, void* pData);

   struct RegisteringCallbacks
   {
      void (*RegisteredType)(FName pTypeName, const TArray<FName>& pBaseTypes);
      void (*RegisteredMethod)(
          FName pOwnerName,
          FName pFunctionName,
          const TArray<FName>& pParameterTypes,
          const TArray<FName>& pParameterNames);
      void (*RegisteredFunction)(
          FName pOwnerName,
          FName pFunctionName,
          const TArray<FName>& pParameterTypes,
          const TArray<FName>& pParameterNames);
      void (*ManuallyRegisteredMethod)(
          FName pOwnerName, const char* pDeclaration);
      void (*ManuallyRegisteredFunction)(
          FName pOwnerName, const char* pDeclaration);
   };

   static void Init();
   static void LoadScripts();
   static void RegisterTypes();
   static void RegisterFileWatcher();
   static void AutoRegisterCflatTypes(const TSet<FName>& pModules, const TSet<FName>& pIgnoredTypes);
   static void SetRegisteringCallbacks(const RegisteringCallbacks& pRegisteringCallbacks);
   static void GenerateAidHeaderFile();

   static void RegisterOnScriptReloadedCallback(UObject* pOwner, OnScriptReloadedCallback pCallback);
   static void DeregisterOnScriptReloadedCallbacks(UObject* pOwner);

   static void CallFunction(Cflat::Function* pFunction,
      const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue,
      OnFunctionCallErrorCallback pOnErrorCallback, void* pOnErrorCallbackData = nullptr);

   static FString GetTypeNameAsString(const Cflat::Type* pType);
   static FString GetTypeUsageAsString(const Cflat::TypeUsage& pTypeUsage);
   static FString GetValueAsString(const Cflat::Value* pValue);
   static FString GetMemberAsString(const Cflat::Member* pMember);
   static FString GetMethodAsString(const Cflat::Method* pMethod);
   static FString GetFunctionAsString(const Cflat::Function* pFunction);

private:
   struct OnScriptReloadedCallbackEntry
   {
      UObject* mOwner;
      OnScriptReloadedCallback mCallback;
   };
   static TArray<OnScriptReloadedCallbackEntry> smOnScriptReloadedCallbacks;

   static bool LoadScript(const FString& pFilePath);
};
}



//
//  Macros for registering Unreal shared pointers
//
#define CflatRegisterTObjectPtr(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, TObjectPtr, T); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TObjectPtr<T>, T*, Get); \
   }

//
//  Macros for registering Unreal containers
//
#define CflatRegisterTArray(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, TArray, T); \
      CflatClassAddConstructor(pEnvironmentPtr, TArray<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TArray<T>, bool, IsEmpty) CflatMethodConst; \
      CflatClassAddMethodReturn(pEnvironmentPtr, TArray<T>, int32, Num) CflatMethodConst; \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, Reserve, int32); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, SetNum, int32); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, SetNumZeroed, int32); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, SetNumUninitialized, int32); \
      CflatClassAddMethodVoid(pEnvironmentPtr, TArray<T>, void, Empty); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, Add, T&); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, RemoveAt, int32); \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("operator[]"); \
         method.mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#T "&"); \
         method.mParameters.push_back((pEnvironmentPtr)->getTypeUsage("int")); \
         Cflat::Environment* environment = pEnvironmentPtr; \
         method.execute = [environment, type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            TArray<T>* thisArray = CflatValueAs(&pThis, TArray<T>*); \
            const int elementIndex = CflatValueAs(&pArguments[0], int); \
            if(elementIndex >= thisArray->Num() || elementIndex < 0) \
            { \
               char errorMessage[256]; \
               snprintf \
               ( \
                  errorMessage, \
                  sizeof(errorMessage), \
                  "invalid TArray index (size %d, index %d)", \
                  thisArray->Num(), \
                  elementIndex \
               ); \
               environment->throwCustomRuntimeError(errorMessage); \
               return; \
            } \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            CflatAssert(method->mParameters.size() == pArguments.size()); \
            T& result = thisArray->operator[](elementIndex); \
            Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("begin"); \
         method.mReturnTypeUsage = templateTypes.back(); \
         method.mReturnTypeUsage.mPointerLevel = 1u; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            T* result = CflatValueAs(&pThis, TArray<T>*)->GetData(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("end"); \
         method.mReturnTypeUsage = templateTypes.back(); \
         method.mReturnTypeUsage.mPointerLevel = 1u; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            T* result = CflatValueAs(&pThis, TArray<T>*)->GetData() + CflatValueAs(&pThis, TArray<T>*)->Num(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
   }

#define CflatRegisterTSet(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, TSet, T); \
      using TRangedForIterator = TSet<T>::TRangedForIterator; \
      Cflat::Class* setType = type; \
      Cflat::Class* rangedForIteratorType = nullptr; \
      { \
         rangedForIteratorType = type->registerType<Cflat::Class>("TRangedForIterator"); \
         rangedForIteratorType->mSize = sizeof(TRangedForIterator); \
         type = rangedForIteratorType; \
         TypeUsage elementRefTypeUsage = setType->mTemplateTypes[0]; \
         elementRefTypeUsage.mFlags |= (uint8_t)Cflat::TypeUsageFlags::Reference; \
         TypeUsage rangedForIteratorRefTypeUsage; \
         rangedForIteratorRefTypeUsage.mType = rangedForIteratorType; \
         rangedForIteratorRefTypeUsage.mFlags |= (uint8_t)Cflat::TypeUsageFlags::Reference; \
         TypeUsage rangedForIteratorConstRefTypeUsage = rangedForIteratorRefTypeUsage; \
         rangedForIteratorConstRefTypeUsage.mFlags |= (uint8_t)Cflat::TypeUsageFlags::Const; \
         CflatClassAddCopyConstructor(pEnvironmentPtr, TRangedForIterator); \
         { \
            type->mMethods.push_back(Cflat::Method("operator++")); \
            const size_t methodIndex = type->mMethods.size() - 1u; \
            Cflat::Method* method = &type->mMethods.back(); \
            method->mReturnTypeUsage = rangedForIteratorRefTypeUsage; \
            method->execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               TRangedForIterator& result = CflatValueAs(&pThis, TRangedForIterator*)->operator++(); \
               pOutReturnValue->set(&result); \
            }; \
         } \
         { \
            type->mMethods.push_back(Cflat::Method("operator*")); \
            const size_t methodIndex = type->mMethods.size() - 1u; \
            Cflat::Method* method = &type->mMethods.back(); \
            method->mReturnTypeUsage = elementRefTypeUsage; \
            method->execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               T& result = CflatValueAs(&pThis, TRangedForIterator*)->operator*(); \
               pOutReturnValue->set(&result); \
            }; \
         } \
         { \
            type->mMethods.push_back(Cflat::Method("operator!=")); \
            const size_t methodIndex = type->mMethods.size() - 1u; \
            Cflat::Method* method = &type->mMethods.back(); \
            method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage("bool"); \
            method->mParameters.push_back(rangedForIteratorConstRefTypeUsage); \
            method->execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               bool result = CflatValueAs(&pThis, TRangedForIterator*)->operator!= \
               ( \
                  CflatValueAs(&pArguments[0], const TRangedForIterator&) \
               ); \
               pOutReturnValue->set(&result); \
            }; \
         } \
         type = setType; \
      } \
      CflatClassAddConstructor(pEnvironmentPtr, TSet<T>); \
      CflatClassAddCopyConstructor(pEnvironmentPtr, TSet<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TSet<T>, bool, IsEmpty) CflatMethodConst; \
      CflatClassAddMethodReturn(pEnvironmentPtr, TSet<T>, int32, Num) CflatMethodConst; \
      CflatClassAddMethodVoid(pEnvironmentPtr, TSet<T>, void, Empty); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TSet<T>, void, Add, T&); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, TSet<T>, bool, Contains, const T&) CflatMethodConst; \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, TSet<T>, T*, Find, const T&); \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("begin"); \
         method.mReturnTypeUsage.mType = rangedForIteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            TRangedForIterator result = CflatValueAs(&pThis, TSet<T>*)->begin(); \
            Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("end"); \
         method.mReturnTypeUsage.mType = rangedForIteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            TRangedForIterator result = CflatValueAs(&pThis, TSet<T>*)->end(); \
            Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
         }; \
         type->mMethods.push_back(method); \
      } \
   }

#define CflatRegisterTSubclassOf(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, TSubclassOf, T); \
      CflatClassAddCopyConstructor(pEnvironmentPtr, TSubclassOf<T>); \
      CflatClassAddConstructorParams1(pEnvironmentPtr, TSubclassOf<T>, UClass*); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TSubclassOf<T>, UClass*, Get); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, TSubclassOf<T>, TSubclassOf<T>&, operator=, UClass*); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TSubclassOf<T>, UClass*, operator*); CflatMethodConst; \
   }


#endif

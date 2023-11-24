
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


#pragma once


// MSVC specifics
#if defined _MSC_VER
# define CflatAPI CFLAT_API     // __declspec(dllexport)
# pragma warning(disable:4996)  // _CRT_SECURE_NO_WARNINGS
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
   static void Init();
   static void LoadScripts();

   static FString GetTypeNameAsString(const Cflat::Type* pType);
   static FString GetTypeUsageAsString(const Cflat::TypeUsage& pTypeUsage);
   static FString GetValueAsString(const Cflat::Value* pValue);

private:
   static bool LoadScript(const FString& pFilePath);
};
}


//
//  Macros for registering Unreal containers
//
#define CflatRegisterTArray(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, TArray, T); \
      CflatClassAddConstructor(pEnvironmentPtr, TArray<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, TArray<T>, int32, Num); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, Reserve, int32); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, SetNum, int32); \
      CflatClassAddMethodVoid(pEnvironmentPtr, TArray<T>, void, Empty); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, TArray<T>, T&, operator[], int); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, TArray<T>, void, Add, const T&); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, TArray<T>, int32, Remove, const T&); \
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

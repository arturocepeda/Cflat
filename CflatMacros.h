
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

#pragma once


//
//  Value retrieval
//
#define CflatValueAs(pValuePtr, pType) \
   (*reinterpret_cast \
      <std::conditional<std::is_lvalue_reference<pType>::value, typename std::remove_reference<pType>::type, pType>::type*> \
      ((pValuePtr)->mValueBuffer))
#define CflatValueAsArray(pValuePtr, pElementType) \
   (reinterpret_cast<pElementType*>((pValuePtr)->mValueBuffer))
#define CflatValueAsArrayElement(pValuePtr, pArrayElementIndex, pElementType) \
   (*reinterpret_cast<pElementType*>((pValuePtr)->mValueBuffer + (pArrayElementIndex * sizeof(pElementType))))


//
//  Type related utilities
//
#define CflatValidateType(pType)  CflatAssert(pType)
#define CflatValidateTypeUsage(pTypeUsage)  CflatAssert(pTypeUsage.mType)

#define CflatMakeTypeUsagePointer(pTypeUsage) \
   pTypeUsage.mPointerLevel++;
#define CflatMakeTypeUsageConst(pTypeUsage) \
   if(pTypeUsage.isPointer()) \
   { \
      CflatSetFlag(pTypeUsage.mFlags, Cflat::TypeUsageFlags::ConstPointer); \
   } \
   else \
   { \
      CflatSetFlag(pTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
   }
#define CflatMakeTypeUsageConstPointer(pTypeUsage) \
   CflatMakeTypeUsagePointer(pTypeUsage); \
   CflatMakeTypeUsageConst(pTypeUsage);


//
//  Function definition
//
#define CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams2(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams3(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams4(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams5(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams6(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams7(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams8(pEnvironmentPtr, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnType, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams2(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams3(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams4(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams5(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams6(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams7(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterFunctionReturnParams8(pEnvironmentPtr, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }

#define CflatRegisterTemplateFunctionVoid(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType>(); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams1(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams2(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams3(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams4(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams5(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams6(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams7(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionVoidParams8(pEnvironmentPtr, pTemplateType, pVoid, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturn(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType>(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams1(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams2(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams3(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams4(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams5(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams6(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams7(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatRegisterTemplateFunctionReturnParams8(pEnvironmentPtr, pTemplateType, pReturnType, pFunctionName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pFunctionName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }



//
//  Type definition: Built-in types
//
#define CflatRegisterBuiltInType(pEnvironmentPtr, pType) \
   { \
      Cflat::BuiltInType* type = (pEnvironmentPtr)->registerType<Cflat::BuiltInType>(#pType); \
      type->mSize = sizeof(pType); \
   }
#define CflatRegisterBuiltInTypedef(pEnvironmentPtr, pTypedefType, pType) \
   { \
      CflatAssert(sizeof(pTypedefType) == sizeof(pType)); \
      Cflat::BuiltInType* typedefType = (pEnvironmentPtr)->registerType<Cflat::BuiltInType>(#pTypedefType); \
      typedefType->mSize = sizeof(pTypedefType); \
      Cflat::Type* type = (pEnvironmentPtr)->getType(#pType); CflatValidateType(type); \
      Cflat::TypeHelper::registerCustomPerfectMatch(typedefType, type); \
   }


//
//  Type definition: Enums
//
#define CflatRegisterEnum(pOwnerPtr, pType) \
   Cflat::Enum* type = (pOwnerPtr)->registerType<Cflat::Enum>(#pType); \
   type->mSize = sizeof(pType);
#define CflatRegisterNestedEnum(pOwnerPtr, pParentType, pType) \
   using pType = pParentType::pType; \
   CflatRegisterEnum(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)), pType);

#define CflatEnumAddValue(pOwnerPtr, pType, pValueName) \
   { \
      const pType enumValueInstance = pType::pValueName; \
      Cflat::TypeUsage enumTypeUsage; \
      enumTypeUsage.mType = type; \
      CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Instance* instance = type->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      instance->mValue.initOnHeap(enumTypeUsage); \
      instance->mValue.set(&enumValueInstance); \
      CflatSetFlag(instance->mFlags, Cflat::InstanceFlags::EnumValue); \
      Cflat::Instance* ownerInstance = (pOwnerPtr)->registerInstance(enumTypeUsage, identifier); \
      ownerInstance->mValue = instance->mValue; \
      CflatSetFlag(ownerInstance->mFlags, Cflat::InstanceFlags::EnumValue); \
   }
#define CflatNestedEnumAddValue(pOwnerPtr, pParentType, pType, pValueName) \
   { \
      const pParentType::pType enumValueInstance = pParentType::pValueName; \
      Cflat::TypeUsage enumTypeUsage; \
      enumTypeUsage.mType = type; \
      CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Struct* parentType = static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)); \
      Cflat::Instance* instance = type->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      instance->mValue.initOnHeap(enumTypeUsage); \
      instance->mValue.set(&enumValueInstance); \
      CflatSetFlag(instance->mFlags, Cflat::InstanceFlags::EnumValue); \
      Cflat::Instance* parentInstance = parentType->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      parentInstance->mValue = instance->mValue; \
      CflatSetFlag(parentInstance->mFlags, Cflat::InstanceFlags::EnumValue); \
   }


//
//  Type definition: EnumClasses
//
#define CflatRegisterEnumClass(pOwnerPtr, pType) \
   Cflat::EnumClass* type = (pOwnerPtr)->registerType<Cflat::EnumClass>(#pType); \
   type->mSize = sizeof(pType);
#define CflatRegisterNestedEnumClass(pOwnerPtr, pParentType, pType) \
   using pType = pParentType::pType; \
   CflatRegisterEnumClass(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)), pType);

#define CflatEnumClassAddValue(pOwnerPtr, pType, pValueName) \
   { \
      const pType enumValueInstance = pType::pValueName; \
      Cflat::TypeUsage enumTypeUsage; \
      enumTypeUsage.mType = type; \
      CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Instance* instance = type->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      instance->mValue.initOnHeap(enumTypeUsage); \
      instance->mValue.set(&enumValueInstance); \
      CflatSetFlag(instance->mFlags, Cflat::InstanceFlags::EnumValue); \
   }
#define CflatNestedEnumClassAddValue(pOwnerPtr, pParentType, pType, pValueName) \
   { \
      const pParentType::pType enumValueInstance = pParentType::pType::pValueName; \
      Cflat::TypeUsage enumTypeUsage; \
      enumTypeUsage.mType = type; \
      CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Instance* instance = type->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      instance->mValue.initOnHeap(enumTypeUsage); \
      instance->mValue.set(&enumValueInstance); \
      CflatSetFlag(instance->mFlags, Cflat::InstanceFlags::EnumValue); \
   }


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pOwnerPtr, pType) \
   Cflat::Struct* type = (pOwnerPtr)->registerType<Cflat::Struct>(#pType); \
   type->mSize = sizeof(pType); \
   type->mAlignment = alignof(pType);
#define CflatRegisterNestedStruct(pOwnerPtr, pParentType, pType) \
   using pType = pParentType::pType; \
   CflatRegisterStruct(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)), pType);

#define CflatStructAddBaseType(pEnvironmentPtr, pType, pBaseType) \
   { \
      Cflat::BaseType baseType; \
      baseType.mType = (pEnvironmentPtr)->getType(#pBaseType); CflatValidateType(baseType.mType); \
      pType* derivedTypePtr = reinterpret_cast<pType*>(0x1); \
      pBaseType* baseTypePtr = static_cast<pBaseType*>(derivedTypePtr); \
      baseType.mOffset = (uint16_t)((char*)baseTypePtr - (char*)derivedTypePtr); \
      type->mBaseTypes.push_back(baseType); \
   }
#define CflatStructAddMember(pEnvironmentPtr, pStructType, pMemberType, pMemberName) \
   { \
      Cflat::Member member(#pMemberName); \
      member.mTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberType); CflatValidateTypeUsage(member.mTypeUsage); \
      member.mTypeUsage.mArraySize = (uint16_t)(sizeof(pStructType::pMemberName) / sizeof(pMemberType)); \
      member.mOffset = (uint16_t)offsetof(pStructType, pMemberName); \
      type->mMembers.push_back(member); \
   }
#define CflatStructAddStaticMember(pEnvironmentPtr, pStructType, pMemberType, pMemberName) \
   { \
      Cflat::TypeUsage typeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberType); CflatValidateTypeUsage(typeUsage); \
      typeUsage.mArraySize = (uint16_t)(sizeof(pStructType::pMemberName) / sizeof(pMemberType)); \
      Cflat::Value value; \
      value.initExternal(typeUsage); \
      value.set(&pStructType::pMemberName); \
      static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->setStaticMember(typeUsage, #pMemberName, value); \
   }
#define CflatStructAddConstructor(pEnvironmentPtr, pStructType) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefine(pEnvironmentPtr, pStructType); \
   }
#define CflatStructAddCopyConstructor(pEnvironmentPtr, pStructType) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      { \
         const size_t methodIndex = type->mMethods.size() - 1u; \
         type->mCachedMethodIndexCopyConstructor = (int8_t)methodIndex; \
         Cflat::Method* method = &type->mMethods.back(); \
         Cflat::TypeUsage refTypeUsage; \
         refTypeUsage.mType = type; \
         refTypeUsage.mFlags |= (uint8_t)Cflat::TypeUsageFlags::Reference; \
         method->mParameters.push_back(refTypeUsage); \
         method->execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(method->mParameters.size() == pArguments.size()); \
            new (CflatValueAs(&pThis, pStructType*)) pStructType \
            ( \
               CflatValueAs(&pArguments[0], pStructType) \
            ); \
         }; \
      } \
   }
#define CflatStructAddConstructorParams1(pEnvironmentPtr, pStructType, \
   pParam0Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructType, \
         pParam0Type); \
   }
#define CflatStructAddConstructorParams2(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams2(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type); \
   }
#define CflatStructAddConstructorParams3(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams3(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type); \
   }
#define CflatStructAddConstructorParams4(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams4(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type); \
   }
#define CflatStructAddConstructorParams5(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams5(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type); \
   }
#define CflatStructAddConstructorParams6(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams6(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type); \
   }
#define CflatStructAddConstructorParams7(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams7(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type); \
   }
#define CflatStructAddConstructorParams8(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructType); \
      _CflatStructConstructorDefineParams8(pEnvironmentPtr, pStructType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type); \
   }
#define CflatStructAddDestructor(pEnvironmentPtr, pStructType) \
   { \
      _CflatStructAddDestructor(pEnvironmentPtr, pStructType); \
      _CflatStructDestructorDefine(pEnvironmentPtr, pStructType); \
   }
#define CflatStructAddMethodVoid(pEnvironmentPtr, pStructType, pVoid, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructType, pMethodName); \
   }
#define CflatStructAddMethodVoidParams1(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type); \
   }
#define CflatStructAddMethodVoidParams2(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type); \
   }
#define CflatStructAddMethodVoidParams3(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type); \
   }
#define CflatStructAddMethodVoidParams4(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type); \
   }
#define CflatStructAddMethodVoidParams5(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams5(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type); \
   }
#define CflatStructAddMethodVoidParams6(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams6(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type); \
   }
#define CflatStructAddMethodVoidParams7(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams7(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type); \
   }
#define CflatStructAddMethodVoidParams8(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineVoidParams8(pEnvironmentPtr, pStructType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type); \
   }
#define CflatStructAddMethodReturn(pEnvironmentPtr, pStructType, pReturnType, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructType, pReturnType, pMethodName); \
   }
#define CflatStructAddMethodReturnParams1(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type); \
   }
#define CflatStructAddMethodReturnParams2(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type); \
   }
#define CflatStructAddMethodReturnParams3(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type); \
   }
#define CflatStructAddMethodReturnParams4(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type); \
   }
#define CflatStructAddMethodReturnParams5(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams5(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type); \
   }
#define CflatStructAddMethodReturnParams6(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams6(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type); \
   }
#define CflatStructAddMethodReturnParams7(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams7(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type); \
   }
#define CflatStructAddMethodReturnParams8(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineReturnParams8(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type); \
   }

#define CflatStructAddTemplateMethodVoid(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>); \
   }
#define CflatStructAddTemplateMethodVoidParams1(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type); \
   }
#define CflatStructAddTemplateMethodVoidParams2(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type); \
   }
#define CflatStructAddTemplateMethodVoidParams3(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type); \
   }
#define CflatStructAddTemplateMethodVoidParams4(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type); \
   }
#define CflatStructAddTemplateMethodVoidParams5(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams5(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type); \
   }
#define CflatStructAddTemplateMethodVoidParams6(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams6(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type); \
   }
#define CflatStructAddTemplateMethodVoidParams7(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams7(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type); \
   }
#define CflatStructAddTemplateMethodVoidParams8(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineVoidParams8(pEnvironmentPtr, pStructType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type); \
   }
#define CflatStructAddTemplateMethodReturn(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>); \
   }
#define CflatStructAddTemplateMethodReturnParams1(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type); \
   }
#define CflatStructAddTemplateMethodReturnParams2(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type); \
   }
#define CflatStructAddTemplateMethodReturnParams3(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type); \
   }
#define CflatStructAddTemplateMethodReturnParams4(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type); \
   }
#define CflatStructAddTemplateMethodReturnParams5(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams5(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type); \
   }
#define CflatStructAddTemplateMethodReturnParams6(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams6(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type); \
   }
#define CflatStructAddTemplateMethodReturnParams7(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams7(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type); \
   }
#define CflatStructAddTemplateMethodReturnParams8(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType); \
      _CflatStructMethodDefineReturnParams8(pEnvironmentPtr, pStructType, pReturnType, pMethodName<pTemplateType>, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type); \
   }

#define CflatStructAddStaticMethodVoid(pEnvironmentPtr, pStructType, pVoid, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName(); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams2(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams3(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams4(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams5(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams6(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams7(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams8(pEnvironmentPtr, pStructType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodReturn(pEnvironmentPtr, pStructType, pReturnType, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams2(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams3(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams4(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams5(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams6(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams7(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams8(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }

#define CflatStructAddStaticTemplateMethodVoid(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType>(); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams1(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams2(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams3(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams4(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams5(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams6(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams7(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodVoidParams8(pEnvironmentPtr, pStructType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturn(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType>(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams1(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams2(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams3(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams4(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams5(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams6(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams7(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define CflatStructAddStaticTemplateMethodReturnParams8(pEnvironmentPtr, pStructType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructType))->registerStaticMethod(#pMethodName); \
      function->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(function->mTemplateTypes.back()); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(function->mReturnTypeUsage); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(function->mParameters.back()); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnType result = pStructType::pMethodName<pTemplateType> \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(function->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pOwnerPtr, pType) \
   Cflat::Class* type = (pOwnerPtr)->registerType<Cflat::Class>(#pType); \
   type->mSize = sizeof(pType); \
   type->mAlignment = alignof(pType);
#define CflatRegisterNestedClass(pOwnerPtr, pParentType, pType) \
   using pType = pParentType::pType; \
   CflatRegisterClass(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)), pType);

#define CflatClassAddBaseType(pEnvironmentPtr, pType, pBaseType) \
   { \
      CflatStructAddBaseType(pEnvironmentPtr, pType, pBaseType) \
   }
#define CflatClassAddMember(pEnvironmentPtr, pClassType, pMemberType, pMemberName) \
   { \
      CflatStructAddMember(pEnvironmentPtr, pClassType, pMemberType, pMemberName) \
   }
#define CflatClassAddStaticMember(pEnvironmentPtr, pClassType, pMemberType, pMemberName) \
   { \
      CflatStructAddStaticMember(pEnvironmentPtr, pClassType, pMemberType, pMemberName) \
   }
#define CflatClassAddConstructor(pEnvironmentPtr, pClassType) \
   { \
      CflatStructAddConstructor(pEnvironmentPtr, pClassType) \
   }
#define CflatClassAddCopyConstructor(pEnvironmentPtr, pClassType) \
   { \
      CflatStructAddCopyConstructor(pEnvironmentPtr, pClassType) \
   }
#define CflatClassAddConstructorParams1(pEnvironmentPtr, pClassType, \
   pParam0Type) \
   { \
      CflatStructAddConstructorParams1(pEnvironmentPtr, pClassType, \
         pParam0Type) \
   }
#define CflatClassAddConstructorParams2(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddConstructorParams2(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddConstructorParams3(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddConstructorParams3(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddConstructorParams4(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddConstructorParams4(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddConstructorParams5(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddConstructorParams5(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddConstructorParams6(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddConstructorParams6(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddConstructorParams7(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddConstructorParams7(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddConstructorParams8(pEnvironmentPtr, pClassType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddConstructorParams8(pEnvironmentPtr, pClassType, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }
#define CflatClassAddDestructor(pEnvironmentPtr, pClassType) \
   { \
      CflatStructAddDestructor(pEnvironmentPtr, pClassType) \
   }
#define CflatClassAddMethodVoid(pEnvironmentPtr, pClassType, pVoid, pMethodName) \
   { \
      CflatStructAddMethodVoid(pEnvironmentPtr, pClassType, pVoid, pMethodName) \
   }
#define CflatClassAddMethodVoidParams1(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddMethodVoidParams1(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddMethodVoidParams2(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddMethodVoidParams2(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddMethodVoidParams3(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddMethodVoidParams3(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddMethodVoidParams4(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddMethodVoidParams4(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddMethodVoidParams5(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddMethodVoidParams5(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddMethodVoidParams6(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddMethodVoidParams6(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddMethodVoidParams7(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddMethodVoidParams7(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddMethodVoidParams8(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddMethodVoidParams8(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }
#define CflatClassAddMethodReturn(pEnvironmentPtr, pClassType, pReturnType, pMethodName) \
   { \
      CflatStructAddMethodReturn(pEnvironmentPtr, pClassType, pReturnType, pMethodName) \
   }
#define CflatClassAddMethodReturnParams1(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddMethodReturnParams1(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddMethodReturnParams2(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddMethodReturnParams2(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddMethodReturnParams3(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddMethodReturnParams3(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddMethodReturnParams4(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddMethodReturnParams4(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddMethodReturnParams5(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddMethodReturnParams5(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddMethodReturnParams6(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddMethodReturnParams6(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddMethodReturnParams7(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddMethodReturnParams7(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddMethodReturnParams8(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddMethodReturnParams8(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }

#define CflatClassAddTemplateMethodVoid(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName) \
   { \
      CflatStructAddTemplateMethodVoid(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName) \
   }
#define CflatClassAddTemplateMethodVoidParams1(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddTemplateMethodVoidParams1(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddTemplateMethodVoidParams2(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddTemplateMethodVoidParams2(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddTemplateMethodVoidParams3(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddTemplateMethodVoidParams3(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddTemplateMethodVoidParams4(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddTemplateMethodVoidParams4(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddTemplateMethodVoidParams5(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddTemplateMethodVoidParams5(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddTemplateMethodVoidParams6(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddTemplateMethodVoidParams6(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddTemplateMethodVoidParams7(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddTemplateMethodVoidParams7(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddTemplateMethodVoidParams8(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddTemplateMethodVoidParams8(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }
#define CflatClassAddTemplateMethodReturn(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName) \
   { \
      CflatStructAddTemplateMethodReturn(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName) \
   }
#define CflatClassAddTemplateMethodReturnParams1(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddTemplateMethodReturnParams1(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddTemplateMethodReturnParams2(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddTemplateMethodReturnParams2(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddTemplateMethodReturnParams3(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddTemplateMethodReturnParams3(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddTemplateMethodReturnParams4(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddTemplateMethodReturnParams4(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddTemplateMethodReturnParams5(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddTemplateMethodReturnParams5(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddTemplateMethodReturnParams6(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddTemplateMethodReturnParams6(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddTemplateMethodReturnParams7(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddTemplateMethodReturnParams7(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddTemplateMethodReturnParams8(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddTemplateMethodReturnParams8(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }

#define CflatClassAddStaticMethodVoid(pEnvironmentPtr, pClassType, pVoid, pMethodName) \
   { \
      CflatStructAddStaticMethodVoid(pEnvironmentPtr, pClassType, pVoid, pMethodName) \
   }
#define CflatClassAddStaticMethodVoidParams1(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddStaticMethodVoidParams2(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddStaticMethodVoidParams2(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddStaticMethodVoidParams3(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddStaticMethodVoidParams3(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddStaticMethodVoidParams4(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddStaticMethodVoidParams4(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddStaticMethodVoidParams5(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddStaticMethodVoidParams5(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddStaticMethodVoidParams6(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddStaticMethodVoidParams6(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddStaticMethodVoidParams7(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddStaticMethodVoidParams7(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddStaticMethodVoidParams8(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddStaticMethodVoidParams8(pEnvironmentPtr, pClassType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }
#define CflatClassAddStaticMethodReturn(pEnvironmentPtr, pClassType, pReturnType, pMethodName) \
   { \
      CflatStructAddStaticMethodReturn(pEnvironmentPtr, pClassType, pReturnType, pMethodName) \
   }
#define CflatClassAddStaticMethodReturnParams1(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddStaticMethodReturnParams2(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddStaticMethodReturnParams2(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddStaticMethodReturnParams3(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddStaticMethodReturnParams3(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddStaticMethodReturnParams4(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddStaticMethodReturnParams4(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddStaticMethodReturnParams5(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddStaticMethodReturnParams5(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddStaticMethodReturnParams6(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddStaticMethodReturnParams6(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddStaticMethodReturnParams7(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddStaticMethodReturnParams7(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddStaticMethodReturnParams8(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddStaticMethodReturnParams8(pEnvironmentPtr, pClassType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }

#define CflatClassAddStaticTemplateMethodVoid(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName) \
   { \
      CflatStructAddStaticTemplateMethodVoid(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams1(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams1(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams2(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams2(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams3(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams3(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams4(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams4(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams5(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams5(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams6(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams6(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams7(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams7(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddStaticTemplateMethodVoidParams8(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddStaticTemplateMethodVoidParams8(pEnvironmentPtr, pClassType, pTemplateType, pVoid, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }
#define CflatClassAddStaticTemplateMethodReturn(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName) \
   { \
      CflatStructAddStaticTemplateMethodReturn(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams1(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams1(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams2(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams2(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams3(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams3(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams4(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams4(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type) \
   }   
#define CflatClassAddStaticTemplateMethodReturnParams5(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams5(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams6(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams6(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams7(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams7(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type) \
   }
#define CflatClassAddStaticTemplateMethodReturnParams8(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      CflatStructAddStaticTemplateMethodReturnParams8(pEnvironmentPtr, pClassType, pTemplateType, pReturnType, pMethodName, \
         pParam0Type, \
         pParam1Type, \
         pParam2Type, \
         pParam3Type, \
         pParam4Type, \
         pParam5Type, \
         pParam6Type, \
         pParam7Type) \
   }


//
//  Type definition: Structs/Classes
//
#define CflatMethodConst \
   CflatSetFlag(type->mMethods.back().mFlags, Cflat::MethodFlags::Const)


//
//  Type definition: Templates
//
#define CflatRegisterTemplateStructTypes1(pEnvironmentPtr, pType, pTemplateType) \
   CflatArgsVector(Cflat::TypeUsage) templateTypes; \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(templateTypes.back()); \
   Cflat::Struct* type = (pEnvironmentPtr)->registerTemplate<Cflat::Struct>(#pType, templateTypes); \
   type->mSize = sizeof(pType<pTemplateType>);
#define CflatRegisterTemplateStructTypes2(pEnvironmentPtr, pType, pTemplateType1, pTemplateType2) \
   CflatArgsVector(Cflat::TypeUsage) templateTypes; \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType1)); CflatValidateTypeUsage(templateTypes.back()); \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType2)); CflatValidateTypeUsage(templateTypes.back()); \
   Cflat::Struct* type = (pEnvironmentPtr)->registerTemplate<Cflat::Struct>(#pType, templateTypes); \
   type->mSize = sizeof(pType<pTemplateType1, pTemplateType2>);

#define CflatRegisterTemplateClassTypes1(pEnvironmentPtr, pType, pTemplateType) \
   CflatArgsVector(Cflat::TypeUsage) templateTypes; \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(templateTypes.back()); \
   Cflat::Class* type = (pEnvironmentPtr)->registerTemplate<Cflat::Class>(#pType, templateTypes); \
   type->mSize = sizeof(pType<pTemplateType>);
#define CflatRegisterTemplateClassTypes2(pEnvironmentPtr, pType, pTemplateType1, pTemplateType2) \
   CflatArgsVector(Cflat::TypeUsage) templateTypes; \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType1)); CflatValidateTypeUsage(templateTypes.back()); \
   templateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType2)); CflatValidateTypeUsage(templateTypes.back()); \
   Cflat::Class* type = (pEnvironmentPtr)->registerTemplate<Cflat::Class>(#pType, templateTypes); \
   type->mSize = sizeof(pType<pTemplateType1, pTemplateType2>);


//
//  Type definition: Aliases
//
#define CflatRegisterTypeAlias(pEnvironmentPtr, pType, pAlias) \
   { \
      const Cflat::TypeUsage typeUsage = (pEnvironmentPtr)->getTypeUsage(#pType); CflatValidateTypeUsage(typeUsage); \
      (pEnvironmentPtr)->registerTypeAlias(#pAlias, typeUsage); \
   }



//
//  Internal macros - helpers for the user macros
//
#define _CflatStructAddConstructor(pEnvironmentPtr, pStructType) \
   { \
      Cflat::Method method(""); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddDestructor(pEnvironmentPtr, pStructType) \
   { \
      Cflat::Method method("~"); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddMethod(pEnvironmentPtr, pStructType, pMethodName) \
   { \
      Cflat::Method method(#pMethodName); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructConstructorDefine(pEnvironmentPtr, pStructType) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      type->mCachedMethodIndexDefaultConstructor = (int8_t)methodIndex; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         new (CflatValueAs(&pThis, pStructType*)) pStructType(); \
      }; \
   }
#define _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructType, \
   pParam0Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams2(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams3(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams4(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams5(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams6(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams7(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams8(pEnvironmentPtr, pStructType, \
   pParam0Type, \
   pParam1Type, \
   pParam2Type, \
   pParam3Type, \
   pParam4Type, \
   pParam5Type, \
   pParam6Type, \
   pParam7Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructType*)) pStructType \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define _CflatStructDestructorDefine(pEnvironmentPtr, pStructType) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      type->mCachedMethodIndexDestructor = (int8_t)methodIndex; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         typedef pStructType CflatDtorType; \
         CflatValueAs(&pThis, pStructType*)->~CflatDtorType(); \
      }; \
   }
#define _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructType, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatValueAs(&pThis, pStructType*)->pMethodName(); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams5(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams6(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams7(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type, \
      pParam6Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams8(pEnvironmentPtr, pStructType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type, \
      pParam6Type, \
      pParam7Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructType, pReturnType, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName(); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams5(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams6(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams7(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type, \
      pParam6Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams8(pEnvironmentPtr, pStructType, pReturnType, pMethodName, \
      pParam0Type, \
      pParam1Type, \
      pParam2Type, \
      pParam3Type, \
      pParam4Type, \
      pParam5Type, \
      pParam6Type, \
      pParam7Type) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnType); CflatValidateTypeUsage(method->mReturnTypeUsage); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam4Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam5Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam6Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam7Type)); CflatValidateTypeUsage(method->mParameters.back()); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnType result = CflatValueAs(&pThis, pStructType*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0Type), \
            CflatValueAs(&pArguments[1], pParam1Type), \
            CflatValueAs(&pArguments[2], pParam2Type), \
            CflatValueAs(&pArguments[3], pParam3Type), \
            CflatValueAs(&pArguments[4], pParam4Type), \
            CflatValueAs(&pArguments[5], pParam5Type), \
            CflatValueAs(&pArguments[6], pParam6Type), \
            CflatValueAs(&pArguments[7], pParam7Type) \
         ); \
         Cflat::Environment::assignReturnValueFromFunctionCall(method->mReturnTypeUsage, &result, pOutReturnValue); \
      }; \
   }
#define _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType) \
   { \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(method->mTemplateTypes.back()); \
   }

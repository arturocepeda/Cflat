
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.20
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2021 Arturo Cepeda Pérez
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

#include "Cflat.h"

#include <iostream>

namespace Cflat
{
   class Helper
   {
   public:
      static void registerStdString(Environment* pEnv)
      {
         CflatRegisterClass(pEnv, std::string);
         CflatClassAddConstructor(pEnv, std::string);
         CflatClassAddConstructorParams1(pEnv, std::string, const char*);
         CflatClassAddMethodReturn(pEnv, std::string, bool, empty);
         CflatClassAddMethodReturn(pEnv, std::string, size_t, size);
         CflatClassAddMethodReturn(pEnv, std::string, size_t, length);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string&, assign, const std::string&);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string&, assign, const char*);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string&, append, const std::string&);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string&, append, const char*);
         CflatClassAddMethodReturn(pEnv, std::string, const char*, c_str);
         CflatClassAddMethodReturnParams1(pEnv, std::string, char&, at, size_t);

         CflatRegisterFunctionReturnParams2(pEnv, std::string, operator+, const std::string&, const std::string&);
         CflatRegisterFunctionReturnParams2(pEnv, std::string, operator+, const std::string&, const char*);
      }

      static void registerStdOut(Environment* pEnv)
      {
         CflatRegisterClass(pEnv, std::ostream);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, int);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, bool);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, size_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, uint32_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, int32_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, uint16_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, int16_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, uint8_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, int8_t);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, float);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream&, operator<<, double);

         CflatRegisterFunctionReturnParams2(pEnv, std::ostream&, operator<<, std::ostream&, const char*);

         EnvironmentStack stack;
         TypeUsage coutTypeUsage = pEnv->getTypeUsage("std::ostream");
         Value coutValue;
         coutValue.initOnStack(coutTypeUsage, &stack);
         coutValue.set(&std::cout);
         pEnv->setVariable(coutTypeUsage, "std::cout", coutValue);
      }
   };
}


//
//  Macros for registering STL types
//
#define CflatRegisterSTLVector(pEnvironmentPtr, T) \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, std::vector, T); \
      CflatClassAddConstructor(pEnvironmentPtr, std::vector<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, std::vector<T>, bool, empty); \
      CflatClassAddMethodReturn(pEnvironmentPtr, std::vector<T>, size_t, size); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void, reserve, size_t); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void, resize, size_t); \
      CflatClassAddMethodVoid(pEnvironmentPtr, std::vector<T>, void, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, std::vector<T>, T&, operator[], int); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void, push_back, const T&); \
      Cflat::Class* iteratorType = nullptr; \
      { \
         iteratorType = type->mTypesHolder.registerType<Cflat::Class>("iterator", type->mNamespace, type); \
         iteratorType->mSize = sizeof(std::vector<T>::iterator); \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator!="); \
            method.mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage("bool"); \
            Cflat::TypeUsage parameter; \
            parameter.mType = iteratorType; \
            parameter.mFlags = (uint8_t)Cflat::TypeUsageFlags::Const | (uint8_t)Cflat::TypeUsageFlags::Reference; \
            method.mParameters.push_back(parameter); \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               bool result = CflatValueAs(&pThis, std::vector<T>::iterator*)->operator!= \
               ( \
                  CflatValueAs(&pArguments[0], const std::vector<T>::iterator&) \
               ); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator*"); \
            method.mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#T); CflatValidateTypeUsage(method.mReturnTypeUsage); \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               T result = CflatValueAs(&pThis, std::vector<T>::iterator*)->operator*(); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator++"); \
            method.mReturnTypeUsage.mType = iteratorType; \
            method.mReturnTypeUsage.mFlags = (uint8_t)Cflat::TypeUsageFlags::Reference; \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               std::vector<T>::iterator& result = CflatValueAs(&pThis, std::vector<T>::iterator*)->operator++(); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("begin"); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            std::vector<T>::iterator result = CflatValueAs(&pThis, std::vector<T>*)->begin(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("end"); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            std::vector<T>::iterator result = CflatValueAs(&pThis, std::vector<T>*)->end(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
   }
#define CflatRegisterSTLMap(pEnvironmentPtr, K, V) \
   { \
      CflatRegisterTemplateClassTypes2(pEnvironmentPtr, std::map, K, V); \
      typedef std::map<K, V> MapType; \
      CflatClassAddConstructor(pEnvironmentPtr, MapType); \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, bool, empty); \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, size_t, size); \
      CflatClassAddMethodVoid(pEnvironmentPtr, MapType, void, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, MapType, V&, operator[], const K&); \
   }

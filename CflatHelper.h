
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

      static void registerPrintfFamily(Environment* pEnv)
      {
         // snprintf
         {
            Function* function = pEnv->registerFunction("snprintf");
            CflatSetFlag(function->mFlags, FunctionFlags::Variadic);
            function->mParameters.push_back(pEnv->getTypeUsage("char*"));
            function->mParameters.push_back(pEnv->getTypeUsage("size_t"));
            function->mParameters.push_back(pEnv->getTypeUsage("const char*"));
            function->execute = snprintfExecute;
         }
         // sprintf
         {
            Function* function = pEnv->registerFunction("sprintf");
            CflatSetFlag(function->mFlags, FunctionFlags::Variadic);
            function->mParameters.push_back(pEnv->getTypeUsage("char*"));
            function->mParameters.push_back(pEnv->getTypeUsage("const char*"));
            function->execute = sprintfExecute;
         }
         // printf
         {
            Function* function = pEnv->registerFunction("printf");
            CflatSetFlag(function->mFlags, FunctionFlags::Variadic);
            function->mParameters.push_back(pEnv->getTypeUsage("const char*"));
            function->execute = printfExecute;
         }
      }

      static void snprintfFunction(char* pBuffer, size_t pBufferSize, const char* pFormat,
         const Cflat::Value* pVariadicArgs, size_t pVariadicArgsCount)
      {
         char formatSpecifierBuffer[32];

         size_t variadicArgIndex = 0u;

         size_t bufferCursor = 0u;
         size_t formatCursor = 0u;

         for(; pFormat[formatCursor] != '\0' && bufferCursor < pBufferSize; formatCursor++)
         {
            if(pFormat[formatCursor] != '%')
            {
               pBuffer[bufferCursor++] = pFormat[formatCursor];
               continue;
            }

            const size_t formatSpecifierIndexFirst = formatCursor;

            formatCursor++;

            if(pFormat[formatCursor] == '%')
            {
               pBuffer[bufferCursor++] = '%';
               continue;
            }

            size_t formatSpecifierIndexLast = formatCursor + 1u;

            while(pFormat[formatCursor] != '\0')
            {
               if(pFormat[formatCursor] != 'l' && isalpha(pFormat[formatCursor]))
               {
                  formatSpecifierIndexLast = formatCursor;
                  break;
               }

               formatCursor++;
            }

            if(pFormat[formatCursor] == '\0')
            {
               break;
            }

            if(variadicArgIndex < pVariadicArgsCount)
            {
               const Value& variadicArg = pVariadicArgs[variadicArgIndex];

               const size_t formatSpecifierLength = formatSpecifierIndexLast - formatSpecifierIndexFirst + 1u;
               CflatAssert(formatSpecifierLength < sizeof(formatSpecifierBuffer));
               memcpy(formatSpecifierBuffer, pFormat + formatSpecifierIndexFirst, formatSpecifierLength);
               formatSpecifierBuffer[formatSpecifierLength] = '\0';

               char* buffer = pBuffer + bufferCursor;
               const size_t charLimit = pBufferSize - bufferCursor;

               int returnValue = 0;

               if(variadicArg.mTypeUsage.isPointer())
               {
                  returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                     *reinterpret_cast<void**>(variadicArg.mValueBuffer));
               }
               else if(variadicArg.mTypeUsage.mType->isInteger())
               {
                  if(variadicArg.mTypeUsage.mType->mIdentifier.mName[0] == 'u')
                  {
                     if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint32_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint32_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint64_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint64_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint8_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint8_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint16_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint16_t*>(variadicArg.mValueBuffer));
                     }
                  }
                  else
                  {
                     if(variadicArg.mTypeUsage.mType->mSize == sizeof(int32_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int32_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int64_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int64_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int8_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int8_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int16_t))
                     {
                        returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int16_t*>(variadicArg.mValueBuffer));
                     }
                  }
               }
               else if(variadicArg.mTypeUsage.mType->isDecimal())
               {
                  if(variadicArg.mTypeUsage.mType->mSize == sizeof(float))
                  {
                     returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                        *reinterpret_cast<float*>(variadicArg.mValueBuffer));
                  }
                  else if(variadicArg.mTypeUsage.mType->mSize == sizeof(double))
                  {
                     returnValue = snprintf(buffer, charLimit, formatSpecifierBuffer,
                        *reinterpret_cast<double*>(variadicArg.mValueBuffer));
                  }
               }

               if(returnValue > 0)
               {
                  bufferCursor += returnValue;
               }

               variadicArgIndex++;
            }
         }

         pBuffer[bufferCursor] = '\0';
      }

      static void snwprintfFunction(wchar_t* pBuffer, size_t pBufferSize, const wchar_t* pFormat,
         const Cflat::Value* pVariadicArgs, size_t pVariadicArgsCount)
      {
         wchar_t formatSpecifierBuffer[32];

         size_t variadicArgIndex = 0u;

         size_t bufferCursor = 0u;
         size_t formatCursor = 0u;

         for(; pFormat[formatCursor] != L'\0' && bufferCursor < pBufferSize; formatCursor++)
         {
            if(pFormat[formatCursor] != L'%')
            {
               pBuffer[bufferCursor++] = pFormat[formatCursor];
               continue;
            }

            const size_t formatSpecifierIndexFirst = formatCursor;

            formatCursor++;

            if(pFormat[formatCursor] == L'%')
            {
               pBuffer[bufferCursor++] = L'%';
               continue;
            }

            size_t formatSpecifierIndexLast = formatCursor + 1u;

            while(pFormat[formatCursor] != L'\0')
            {
               if(pFormat[formatCursor] != L'l' && isalpha(pFormat[formatCursor]))
               {
                  formatSpecifierIndexLast = formatCursor;
                  break;
               }

               formatCursor++;
            }

            if(pFormat[formatCursor] == L'\0')
            {
               break;
            }

            if(variadicArgIndex < pVariadicArgsCount)
            {
               const Value& variadicArg = pVariadicArgs[variadicArgIndex];

               const size_t formatSpecifierLength = formatSpecifierIndexLast - formatSpecifierIndexFirst + 1u;
               CflatAssert(formatSpecifierLength < sizeof(formatSpecifierBuffer));
               memcpy(formatSpecifierBuffer, pFormat + formatSpecifierIndexFirst, formatSpecifierLength * sizeof(wchar_t));
               formatSpecifierBuffer[formatSpecifierLength] = L'\0';

               wchar_t* buffer = pBuffer + bufferCursor;
               const size_t charLimit = pBufferSize - bufferCursor;

               int returnValue = 0;

               if(variadicArg.mTypeUsage.isPointer())
               {
                  returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                     *reinterpret_cast<void**>(variadicArg.mValueBuffer));
               }
               else if(variadicArg.mTypeUsage.mType->isInteger())
               {
                  if(variadicArg.mTypeUsage.mType->mIdentifier.mName[0] == L'u')
                  {
                     if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint32_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint32_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint64_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint64_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint8_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint8_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(uint16_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<uint16_t*>(variadicArg.mValueBuffer));
                     }
                  }
                  else
                  {
                     if(variadicArg.mTypeUsage.mType->mSize == sizeof(int32_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int32_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int64_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int64_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int8_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int8_t*>(variadicArg.mValueBuffer));
                     }
                     else if(variadicArg.mTypeUsage.mType->mSize == sizeof(int16_t))
                     {
                        returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                           *reinterpret_cast<int16_t*>(variadicArg.mValueBuffer));
                     }
                  }
               }
               else if(variadicArg.mTypeUsage.mType->isDecimal())
               {
                  if(variadicArg.mTypeUsage.mType->mSize == sizeof(float))
                  {
                     returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                        *reinterpret_cast<float*>(variadicArg.mValueBuffer));
                  }
                  else if(variadicArg.mTypeUsage.mType->mSize == sizeof(double))
                  {
                     returnValue = swprintf(buffer, charLimit, formatSpecifierBuffer,
                        *reinterpret_cast<double*>(variadicArg.mValueBuffer));
                  }
               }

               if(returnValue > 0)
               {
                  bufferCursor += returnValue;
               }

               variadicArgIndex++;
            }
         }

         pBuffer[bufferCursor] = L'\0';
      }

   private:
      static void snprintfExecute(const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
      {
         (void)pOutReturnValue;

         static const size_t kFixedArgsCount = 3u;
         const size_t variadicArgsCount = pArgs.size() - kFixedArgsCount;

         snprintfFunction
         (
            CflatValueAs(&pArgs[0], char*),
            CflatValueAs(&pArgs[1], size_t),
            CflatValueAs(&pArgs[2], const char*),
            &pArgs[kFixedArgsCount],
            variadicArgsCount
         );
      }
      static void sprintfExecute(const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
      {
         (void)pOutReturnValue;

         static const size_t kFixedArgsCount = 2u;
         const size_t variadicArgsCount = pArgs.size() - kFixedArgsCount;

         snprintfFunction
         (
            CflatValueAs(&pArgs[0], char*),
            SIZE_MAX,
            CflatValueAs(&pArgs[1], const char*),
            &pArgs[kFixedArgsCount],
            variadicArgsCount
         );
      }
      static void printfExecute(const CflatArgsVector(Cflat::Value)& pArgs, Cflat::Value* pOutReturnValue)
      {
         (void)pOutReturnValue;

         static const size_t kFixedArgsCount = 1u;
         const size_t variadicArgsCount = pArgs.size() - kFixedArgsCount;

         static const size_t kPrintfBufferSize = 8192u;
         char buffer[kPrintfBufferSize];

         snprintfFunction
         (
            buffer,
            kPrintfBufferSize,
            CflatValueAs(&pArgs[0], const char*),
            &pArgs[kFixedArgsCount],
            variadicArgsCount
         );

         printf("%s", buffer);
      }
   };
}


//
//  Macro for registering initializer lists
//
#define CflatRequestInitializerListType(pEnvironmentPtr, T) \
   { \
      Cflat::Namespace* ns = (pEnvironmentPtr)->requestNamespace("std"); \
      CflatArgsVector(Cflat::TypeUsage) templateArgs; \
      templateArgs.push_back((pEnvironmentPtr)->getTypeUsage(#T)); CflatValidateTypeUsage(templateArgs.back()); \
      Cflat::Type* elementType = ns->getType("initializer_list", templateArgs); \
      if(!elementType) \
      { \
         CflatRegisterTemplateClassTypes1(pEnvironmentPtr, std::initializer_list, T); \
         CflatClassAddConstructor(pEnvironmentPtr, std::initializer_list<T>); \
         { \
            const size_t methodIndex = type->mMethods.size(); \
            Cflat::Method method(""); \
            Cflat::TypeUsage paramTypeUsage = (pEnvironmentPtr)->getTypeUsage(#T); CflatValidateTypeUsage(paramTypeUsage); \
            CflatMakeTypeUsageConstPointer(paramTypeUsage); \
            method.execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(method->mParameters.size() == pArguments.size()); \
               new (CflatValueAs(&pThis, std::initializer_list<T>*)) std::initializer_list<T> \
               ( \
                  CflatValueAs(&pArguments[0], T const*), \
                  CflatValueAs(&pArguments[1], T const*) \
               ); \
            }; \
            method.mParameters.push_back(paramTypeUsage); \
            method.mParameters.push_back(paramTypeUsage); \
            type->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = type->mMethods.size(); \
            Cflat::Method method("begin"); \
            method.mReturnTypeUsage = templateTypes.back(); \
            CflatMakeTypeUsageConstPointer(method.mReturnTypeUsage); \
            CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
            method.execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               auto result = CflatValueAs(&pThis, std::initializer_list<T>*)->begin(); \
               pOutReturnValue->set(&result); \
            }; \
            type->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = type->mMethods.size(); \
            Cflat::Method method("end"); \
            method.mReturnTypeUsage = templateTypes.back(); \
            CflatMakeTypeUsageConstPointer(method.mReturnTypeUsage); \
            CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
            method.execute = [type, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &type->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               auto result = CflatValueAs(&pThis, std::initializer_list<T>*)->end(); \
               pOutReturnValue->set(&result); \
            }; \
            type->mMethods.push_back(method); \
         } \
         CflatClassAddMethodReturn(pEnvironmentPtr, std::initializer_list<T>, size_t, size) CflatMethodConst; \
      } \
   }


//
//  Macros for registering STL types
//
#define CflatRegisterSTLVector(pEnvironmentPtr, T) \
   CflatRegisterSTLVectorCustom(pEnvironmentPtr, std::vector, T)
#define CflatRegisterSTLVectorCustom(pEnvironmentPtr, pContainer, T) \
   { \
      CflatRequestInitializerListType(pEnvironmentPtr, T); \
   } \
   { \
      CflatRegisterTemplateClassTypes1(pEnvironmentPtr, pContainer, T); \
      CflatClassAddConstructor(pEnvironmentPtr, pContainer<T>); \
      CflatClassAddConstructorParams1(pEnvironmentPtr, pContainer<T>, std::initializer_list<T >); \
      CflatClassAddCopyConstructor(pEnvironmentPtr, pContainer<T>); \
      CflatClassAddDestructor(pEnvironmentPtr, pContainer<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, pContainer<T>, bool, empty) CflatMethodConst; \
      CflatClassAddMethodReturn(pEnvironmentPtr, pContainer<T>, size_t, size) CflatMethodConst; \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, pContainer<T>, void, reserve, size_t); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, pContainer<T>, void, resize, size_t); \
      CflatClassAddMethodVoid(pEnvironmentPtr, pContainer<T>, void, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, pContainer<T>, T&, operator[], int); \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("push_back"); \
         Cflat::TypeUsage paramTypeUsage = (pEnvironmentPtr)->getTypeUsage(#T); CflatValidateTypeUsage(paramTypeUsage); \
         CflatMakeTypeUsageConst(paramTypeUsage); \
         method.mParameters.push_back(paramTypeUsage); \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            CflatValueAs(&pThis, pContainer<T>*)->push_back \
            ( \
               CflatValueAs(&pArguments[0], T const&) \
            ); \
         }; \
         type->mMethods.push_back(method); \
      } \
      Cflat::Class* iteratorType = nullptr; \
      { \
         iteratorType = type->mTypesHolder.registerType<Cflat::Class>("iterator", type->mNamespace, type); \
         iteratorType->mSize = sizeof(pContainer<T>::iterator); \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator=="); \
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
               bool result = *CflatValueAs(&pThis, pContainer<T>::iterator*) == CflatValueAs(&pArguments[0], const pContainer<T>::iterator&); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
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
               bool result = *CflatValueAs(&pThis, pContainer<T>::iterator*) != CflatValueAs(&pArguments[0], const pContainer<T>::iterator&); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator*"); \
            method.mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#T"&"); CflatValidateTypeUsage(method.mReturnTypeUsage); \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               T& result = **CflatValueAs(&pThis, pContainer<T>::iterator*); \
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
               pContainer<T>::iterator& result = ++(*CflatValueAs(&pThis, pContainer<T>::iterator*)); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator+"); \
            CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
            method.mReturnTypeUsage.mType = iteratorType; \
            method.mParameters.push_back((pEnvironmentPtr)->getTypeUsage("int")); \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               pContainer<T>::iterator result = *CflatValueAs(&pThis, pContainer<T>::iterator*) + \
               ( \
                  CflatValueAs(&pArguments[0], int) \
               ); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("begin"); \
         CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            pContainer<T>::iterator result = CflatValueAs(&pThis, pContainer<T>*)->begin(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("end"); \
         CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            pContainer<T>::iterator result = CflatValueAs(&pThis, pContainer<T>*)->end(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("erase"); \
         method.mReturnTypeUsage.mType = iteratorType; \
         Cflat::TypeUsage parameter; \
         parameter.mType = iteratorType; \
         method.mParameters.push_back(parameter); \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            pContainer<T>::iterator result = CflatValueAs(&pThis, pContainer<T>*)->erase \
            ( \
               CflatValueAs(&pArguments[0], const pContainer<T>::iterator&) \
            ); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
   }
#define CflatRegisterSTLMap(pEnvironmentPtr, K, V) \
   CflatRegisterSTLMapCustom(pEnvironmentPtr, std::map, std::pair, K, V)
#define CflatRegisterSTLMapCustom(pEnvironmentPtr, pContainer, pPair, K, V) \
   { \
      CflatRegisterTemplateClassTypes2(pEnvironmentPtr, pContainer, K, V); \
      typedef pContainer<K, V> MapType; \
      typedef pPair<const K, V> PairType; \
      CflatClassAddConstructor(pEnvironmentPtr, MapType); \
      CflatClassAddCopyConstructor(pEnvironmentPtr, MapType); \
      CflatClassAddDestructor(pEnvironmentPtr, MapType); \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, bool, empty) CflatMethodConst; \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, size_t, size) CflatMethodConst; \
      CflatClassAddMethodVoid(pEnvironmentPtr, MapType, void, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, MapType, V&, operator[], const K&); \
      CflatArgsVector(Cflat::TypeUsage) mapTemplateTypes; \
      mapTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#K)); CflatValidateTypeUsage(mapTemplateTypes.back()); \
      mapTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#V)); CflatValidateTypeUsage(mapTemplateTypes.back()); \
      Cflat::Class* pairType = nullptr; \
      { \
         pairType = (pEnvironmentPtr)->registerTemplate<Cflat::Class>(#pPair, mapTemplateTypes); \
         pairType->mSize = sizeof(PairType); \
         { \
            Cflat::Member member("first"); \
            member.mTypeUsage = mapTemplateTypes[0]; \
            member.mOffset = (uint16_t)offsetof(PairType, first); \
            pairType->mMembers.push_back(member); \
         } \
         { \
            Cflat::Member member("second"); \
            member.mTypeUsage = mapTemplateTypes[1]; \
            member.mOffset = (uint16_t)offsetof(PairType, second); \
            pairType->mMembers.push_back(member); \
         } \
      } \
      Cflat::Class* iteratorType = nullptr; \
      { \
         iteratorType = type->mTypesHolder.registerType<Cflat::Class>("iterator", type->mNamespace, type); \
         iteratorType->mSize = sizeof(MapType::iterator); \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator=="); \
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
               bool result = *CflatValueAs(&pThis, MapType::iterator*) == CflatValueAs(&pArguments[0], const MapType::iterator&); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
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
               bool result = *CflatValueAs(&pThis, MapType::iterator*) != CflatValueAs(&pArguments[0], const MapType::iterator&); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
         { \
            const size_t methodIndex = iteratorType->mMethods.size(); \
            Cflat::Method method("operator*"); \
            method.mReturnTypeUsage.mType = pairType; \
            method.mReturnTypeUsage.mFlags = (uint8_t)Cflat::TypeUsageFlags::Const | (uint8_t)Cflat::TypeUsageFlags::Reference; \
            method.execute = [iteratorType, methodIndex] \
               (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
            { \
               Cflat::Method* method = &iteratorType->mMethods[methodIndex]; \
               CflatAssert(pOutReturnValue); \
               CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
               const PairType& result = CflatValueAs(&pThis, MapType::iterator*)->operator*(); \
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
               MapType::iterator& result = CflatValueAs(&pThis, MapType::iterator*)->operator++(); \
               pOutReturnValue->set(&result); \
            }; \
            iteratorType->mMethods.push_back(method); \
         } \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("find"); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.mParameters.push_back(mapTemplateTypes[0]); \
         method.mParameters.back().mFlags = (uint8_t)Cflat::TypeUsageFlags::Const | (uint8_t)Cflat::TypeUsageFlags::Reference; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            MapType::iterator result = CflatValueAs(&pThis, MapType*)->find \
            ( \
               CflatValueAs(&pArguments[0], const K&) \
            ); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("begin"); \
         CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            MapType::iterator result = CflatValueAs(&pThis, MapType*)->begin(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("end"); \
         CflatSetFlag(method.mFlags, Cflat::MethodFlags::Const); \
         method.mReturnTypeUsage.mType = iteratorType; \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            MapType::iterator result = CflatValueAs(&pThis, MapType*)->end(); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
      { \
         const size_t methodIndex = type->mMethods.size(); \
         Cflat::Method method("erase"); \
         method.mReturnTypeUsage.mType = iteratorType; \
         Cflat::TypeUsage parameter; \
         parameter.mType = iteratorType; \
         method.mParameters.push_back(parameter); \
         method.execute = [type, methodIndex] \
            (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
         { \
            Cflat::Method* method = &type->mMethods[methodIndex]; \
            CflatAssert(pOutReturnValue); \
            CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
            MapType::iterator result = CflatValueAs(&pThis, MapType*)->erase \
            ( \
               CflatValueAs(&pArguments[0], const MapType::iterator&) \
            ); \
            pOutReturnValue->set(&result); \
         }; \
         type->mMethods.push_back(method); \
      } \
   }


////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.10
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2020 Arturo Cepeda Pérez
//
//  --------------------------------------------------------------------
//
//  This file is part of Cflat. Permission is hereby granted, free 
//  of charge, to any person obtaining a copy of this software and 
//  associated documentation files (the "Software"), to deal in the 
//  Software without restriction, including without limitation the 
//  rights to use, copy, modify, merge, publish, distribute, 
//  sublicense, and/or sell copies of the Software, and to permit 
//  persons to whom the Software is furnished to do so, subject to 
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be 
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
//  KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
//  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
//  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
//  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////

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
         CflatClassAddConstructorParams1(pEnv, std::string, const char*,);
         CflatClassAddMethodReturn(pEnv, std::string, bool,, empty);
         CflatClassAddMethodReturn(pEnv, std::string, size_t,, size);
         CflatClassAddMethodReturn(pEnv, std::string, size_t,, length);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string,&, assign, const std::string,&);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string,&, assign, const char*,);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string,&, append, const std::string,&);
         CflatClassAddMethodReturnParams1(pEnv, std::string, std::string,&, append, const char*,);
         CflatClassAddMethodReturn(pEnv, std::string, const char*,, c_str);
         CflatClassAddMethodReturnParams1(pEnv, std::string, char&,, at, size_t,);

         CflatRegisterFunctionReturnParams2(pEnv, std::string,, operator+, const std::string,&, const std::string,&);
         CflatRegisterFunctionReturnParams2(pEnv, std::string,, operator+, const std::string,&, const char*,);
      }

      static void registerStdOut(Environment* pEnv)
      {
         CflatRegisterClass(pEnv, std::ostream);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, int,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, bool,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, size_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, uint32_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, int32_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, uint16_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, int16_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, uint8_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, int8_t,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, float,);
         CflatClassAddMethodReturnParams1(pEnv, std::ostream, std::ostream,&, operator<<, double,);

         CflatRegisterFunctionReturnParams2(pEnv, std::ostream,&, operator<<, std::ostream,&, const char*,);

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
      CflatRegisterTemplateClassTypeNames1(pEnvironmentPtr, std::vector, T); \
      CflatClassAddConstructor(pEnvironmentPtr, std::vector<T>); \
      CflatClassAddMethodReturn(pEnvironmentPtr, std::vector<T>, bool,, empty); \
      CflatClassAddMethodReturn(pEnvironmentPtr, std::vector<T>, size_t,, size); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void,, reserve, size_t,); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void,, resize, size_t,); \
      CflatClassAddMethodVoid(pEnvironmentPtr, std::vector<T>, void,, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, std::vector<T>, T,&, operator[], int,); \
      CflatClassAddMethodVoidParams1(pEnvironmentPtr, std::vector<T>, void,, push_back, const T,&); \
   }
#define CflatRegisterSTLMap(pEnvironmentPtr, K, V) \
   { \
      CflatRegisterTemplateClassTypeNames2(pEnvironmentPtr, std::map, K, V); \
      typedef std::map<K, V> MapType; \
      CflatClassAddConstructor(pEnvironmentPtr, MapType); \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, bool,, empty); \
      CflatClassAddMethodReturn(pEnvironmentPtr, MapType, size_t,, size); \
      CflatClassAddMethodVoid(pEnvironmentPtr, MapType, void,, clear); \
      CflatClassAddMethodReturnParams1(pEnvironmentPtr, MapType, V,&, operator[], const K,&); \
   }

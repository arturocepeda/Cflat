
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

#if !defined (CflatAssert)
# include <cassert>
# define CflatAssert  assert
#endif

namespace Cflat
{
  // Maximum number of arguments in a function call
  static const size_t kArgsVectorSize = 16u;
  // Maximum number of nested function calls in an execution context
  static const size_t kMaxNestedFunctionCalls = 16u;

  // Size in bytes for the strings pool used to hold identifiers
  static const size_t kIdentifierStringsPoolSize = 32768u;
  // Size in bytes for the strings pool used to hold literals
  static const size_t kLiteralStringsPoolSize = 4096u;

  // Size in bytes for the environment stack
  static const size_t kEnvironmentStackSize = 8192u;

  // Maximum number of global variables/constants per namespace
  static const size_t kMaxGlobalInstancesPerNamespace = 32u;
  // Maximum number of local variables/constants in an execution context
  static const size_t kMaxLocalInstances = 32u;
  // Maximum number of static members in a struct or class
  static const size_t kMaxStaticMembers = 8u;

  // Size in bytes for local string buffers
  static const size_t kDefaultLocalStringBufferSize = 256u;
}

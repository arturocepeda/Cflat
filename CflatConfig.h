
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.30
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2022 Arturo Cepeda Pérez
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

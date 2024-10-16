
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


namespace Cflat
{
   const char* kPreprocessorErrorStrings[] = 
   {
      "invalid preprocessor directive",
      "invalid number of arguments for the '%s' macro"
   };
   const size_t kPreprocessorErrorStringsCount = sizeof(kPreprocessorErrorStrings) / sizeof(const char*);

   const char* kCompileErrorStrings[] = 
   {
      "unexpected symbol ('%s')",
      "'%s' expected",
      "undefined type ('%s')",
      "undefined variable ('%s')",
      "undefined function ('%s') or invalid arguments in call",
      "variable redefinition ('%s')",
      "parameter redefinition ('%s')",
      "uninitialized reference ('%s')",
      "array initialization expected",
      "no default constructor defined for the '%s' type",
      "no copy constructor defined for the '%s' type",
      "invalid numeric value ('%s')",
      "invalid type ('%s')",
      "invalid assignment",
      "invalid member access operator ('%s' is a pointer)",
      "invalid member access operator ('%s' is not a pointer)",
      "invalid operator for the '%s' type (%s)",
      "invalid conditional expression",
      "invalid cast",
      "invalid escape sequence ('%s')",
      "no member named '%s' in the '%s' type",
      "no static member named '%s' in the '%s' type",
      "no constructor in the '%s' type matches the given list of arguments",
      "no method named '%s' in the '%s' type matches the given list of arguments",
      "no static method named '%s' in the '%s' type matches the given list of arguments",
      "'%s' must be an integer value",
      "unknown namespace ('%s')",
      "cannot modify constant expression",
      "cannot call a non-const method on a const instance, reference or pointer",
      "not all code paths return a value in the '%s' function",
      "missing return expression",
      "incompatible return expression type for the '%s' function",
      "static pointers are not allowed in this environment",
      "dynamic_cast is not allowed in this environment"
   };
   const size_t kCompileErrorStringsCount = sizeof(kCompileErrorStrings) / sizeof(const char*);

   const char* kRuntimeErrorStrings[] = 
   {
      "null pointer access ('%s')",
      "invalid array index (%s)",
      "division by zero",
      "missing implementation for the '%s' function"
   };
   const size_t kRuntimeErrorStringsCount = sizeof(kRuntimeErrorStrings) / sizeof(const char*);
}

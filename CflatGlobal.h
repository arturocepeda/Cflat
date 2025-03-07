
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

#include "CflatGlobalConfig.h"

#if defined CFLAT_ENABLED

# if !defined (CflatAPI)
#  define CflatAPI
# endif


//
//  Forward declarations
//
namespace Cflat
{
   class Environment;
   struct Function;
}


//
//  Function declarations (must be implemented on the client side)
//
namespace CflatGlobal
{
   extern CflatAPI Cflat::Environment* getEnvironment();

   extern CflatAPI void lockEnvironment();
   extern CflatAPI void unlockEnvironment();

   extern CflatAPI void onError(const char* pErrorMessage);
}


//
//  Script inclusion
//
# define CflatScript(pScript)  <CflatHeadersPath/Cflat.h>


//
//  Thread-safety
//
# define CflatLock()  CflatGlobal::lockEnvironment();
# define CflatUnlock()  CflatGlobal::unlockEnvironment();

//
//  Value retrieval
//
# define CflatGet(pType, pIdentifier) \
   CflatValueAs(CflatGlobal::getEnvironment()->getVariable(#pIdentifier), pType)
# define CflatGetArray(pElementType, pIdentifier) \
   CflatValueAsArray(CflatGlobal::getEnvironment()->getVariable(#pIdentifier), pElementType)

//
//  Function calls
//
# define CflatArg(pArg) &pArg
# define CflatVoidCall(pFunction, ...) \
   { \
      Cflat::Environment* env = CflatGlobal::getEnvironment(); \
      Cflat::Function* function = env->getFunction(#pFunction); \
      if(function) \
      { \
         env->voidFunctionCall(function, __VA_ARGS__); \
         if(env->getErrorMessage()) \
         { \
            CflatGlobal::onError(env->getErrorMessage()); \
         } \
      } \
   }
# define CflatReturnCall(pLValue, pReturnType, pFunction, ...) \
   { \
      Cflat::Environment* env = CflatGlobal::getEnvironment(); \
      Cflat::Function* function = env->getFunction(#pFunction); \
      if(function) \
      { \
         pLValue = env->returnFunctionCall<pReturnType>(function, __VA_ARGS__); \
         if(env->getErrorMessage()) \
         { \
            CflatGlobal::onError(env->getErrorMessage()); \
         } \
      } \
   }

#else

//
//  Script inclusion
//
# define CflatScript(pScript)  <CflatScriptsPath/pScript>

//
//  Thread-safety
//
# define CflatLock()
# define CflatUnlock()

//
//  Value retrieval
//
# define CflatGet(pType, pIdentifier)  pIdentifier
# define CflatGetArray(pElementType, pIdentifier)  pIdentifier

//
//  Function calls
//
# define CflatArg(pArg)  pArg
# define CflatVoidCall(pFunction, ...)  pFunction(__VA_ARGS__)
# define CflatReturnCall(pLValue, pReturnType, pFunction, ...)  pLValue = pFunction(__VA_ARGS__)

#endif

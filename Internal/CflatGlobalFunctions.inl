
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
   Hash hash(const char* pString)
   {
      static const Hash kOffsetBasis = 2166136261u;
      static const Hash kFNVPrime = 16777619u;

      uint32_t charIndex = 0u;
      Hash hash = kOffsetBasis;

      while(pString[charIndex] != '\0')
      {
         hash ^= pString[charIndex++];
         hash *= kFNVPrime;
      }

      return hash;
   }

   Hash hash(const wchar_t* pString)
   {
      static const Hash kOffsetBasis = 2166136261u;
      static const Hash kFNVPrime = 16777619u;

      uint32_t charIndex = 0u;
      Hash hash = kOffsetBasis;

      while(pString[charIndex] != L'\0')
      {
         hash ^= pString[charIndex++];
         hash *= kFNVPrime;
      }

      return hash;
   }

   template<typename T>
   void toArgsVector(const CflatSTLVector(T) pSTLVector, CflatArgsVector(T)& pArgsVector)
   {
      const size_t elementsCount = pSTLVector.size();
      pArgsVector.resize(elementsCount);

      if(elementsCount > 0u)
      {
         memcpy(&pArgsVector[0], &pSTLVector[0], elementsCount * sizeof(T));
      }
   }
}

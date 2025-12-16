
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

#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <string>

#include "CflatConfig.h"
#include "CflatMacros.h"

#define CflatMalloc  Cflat::Memory::malloc()
#define CflatFree  Cflat::Memory::free()

#define CflatHasFlag(pBitMask, pFlag)  ((pBitMask & (int)pFlag) > 0)
#define CflatSetFlag(pBitMask, pFlag)  (pBitMask |= (int)pFlag)
#define CflatResetFlag(pBitMask, pFlag)  (pBitMask &= ~((int)pFlag))

#define CflatInvokeCtor(pClassName, pPtr)  new (pPtr) pClassName
#define CflatInvokeDtor(pClassName, pPtr)  (pPtr)->~pClassName()

#define CflatSTLVector(T)  std::vector<T, Cflat::Memory::STLAllocator<T>>
#define CflatSTLDeque(T)  std::deque<T, Cflat::Memory::STLAllocator<T>>
#define CflatSTLSet(T)  std::set<T, std::less<T>, Cflat::Memory::STLAllocator<T>>
#define CflatSTLMap(T, U)  std::map<T, U, std::less<T>, Cflat::Memory::STLAllocator<std::pair<const T, U>>>
#define CflatSTLString  std::basic_string<char, std::char_traits<char>, Cflat::Memory::STLAllocator<char>>

#define CflatArgsVector(T)  Cflat::Memory::StackVector<T, Cflat::kArgsVectorSize>

#if !defined (CflatAPI)
# define CflatAPI
#endif

namespace Cflat
{
   typedef uint32_t Hash;

   class CflatAPI Memory
   {
   public:
      typedef void* (*mallocFunction)(size_t pSize);
      typedef void (*freeFunction)(void* pPtr);

   private:
      static mallocFunction smmalloc;
      static freeFunction smfree;

   public:
      static void setFunctions(mallocFunction pmalloc, freeFunction pfree);

      static mallocFunction malloc();
      static freeFunction free();

      template<typename T>
      class STLAllocator
      {
      public:
         typedef T value_type;
         typedef T* pointer;
         typedef const T* const_pointer;
         typedef T& reference;
         typedef const T& const_reference;
         typedef std::size_t size_type;
         typedef std::ptrdiff_t difference_type;

         template<typename U>
         struct rebind { typedef STLAllocator<U> other; };

         pointer address(reference pRef) const { return &pRef; }
         const_pointer address(const_reference pRef) const { return &pRef; }

         STLAllocator() {}
         STLAllocator(const STLAllocator&) {}
         template<typename U>
         STLAllocator(const STLAllocator<U>&) {}
         ~STLAllocator() {}

         size_type max_size() const { return SIZE_MAX / sizeof(T); }

         pointer allocate(size_type pNum, const void* = nullptr)
         {
            return (pointer)CflatMalloc(pNum * sizeof(T));
         }
         void construct(pointer pPtr, const T& pValue)
         {
            CflatInvokeCtor(T, pPtr)(pValue);
         }
         void destroy(pointer pPtr)
         {
            CflatInvokeDtor(T, pPtr);
         }
         void deallocate(pointer pPtr, size_type)
         {
            CflatFree(pPtr);
         }
      };

      template<typename T, size_t Capacity>
      class StackVector
      {
      public:
         typedef T* iterator;
         typedef const T* const_iterator;

      private:
         iterator mFirst;
         iterator mEnd;
         size_t mSize;
         T mData[Capacity];

         void destroy(T* pElement)
         {
            CflatInvokeDtor(T, pElement);
            memset(pElement, 0, sizeof(T));
         }

      public:
         StackVector()
            : mFirst(&mData[0])
            , mEnd(mFirst)
            , mSize(0u)
         {
         }

         inline T& at(size_t pIndex)
         {
            CflatAssert(pIndex < mSize);
            return mData[pIndex];
         }
         inline const T& at(size_t pIndex) const
         {
            CflatAssert(pIndex < mSize);
            return mData[pIndex];
         }
         inline T& operator[](size_t pIndex)
         {
            CflatAssert(pIndex < mSize);
            return mData[pIndex];
         }
         inline const T& operator[](size_t pIndex) const
         {
            CflatAssert(pIndex < mSize);
            return mData[pIndex];
         }
         T& front()
         {
            CflatAssert(mSize > 0u);
            return mData[0u];
         }
         const T& front() const
         {
            CflatAssert(mSize > 0u);
            return mData[0u];
         }
         T& back()
         {
            CflatAssert(mSize > 0u);
            return mData[mSize - 1u];
         }
         const T& back() const
         {
            CflatAssert(mSize > 0u);
            return mData[mSize - 1u];
         }
         T* data() noexcept
         {
            return mSize > 0u ? &mData[0] : nullptr;
         }
         const T* data() const noexcept
         {
            return mSize > 0u ? &mData[0] : nullptr;
         }

         inline iterator begin()
         {
            return mFirst;
         }
         inline const_iterator begin() const
         {
            return mFirst;
         }
         inline iterator end()
         {
            return mEnd;
         }
         inline const_iterator end() const
         {
            return mEnd;
         }

         inline bool empty() const
         {
            return mSize == 0u;
         }
         inline size_t size() const
         {
            return mSize;
         }
         inline size_t capacity() const
         {
            return Capacity;
         }

         void clear()
         {
            mSize = 0u;
            mEnd = mFirst;
         }
         void push_back(const T& pElement)
         {
            CflatAssert(mSize < Capacity);
            mData[mSize++] = pElement;
            mEnd++;
         }
         void emplace_back()
         {
            CflatAssert(mSize < Capacity);
            mData[mSize++] = T();
            mEnd++;
         }
         void pop_back()
         {
            CflatAssert(mSize > 0u);
            destroy(&mData[mSize - 1u]);
            mSize--;
            mEnd--;
         }
         void resize(size_t pSize)
         {
            CflatAssert(pSize <= Capacity);
            for(size_t i = pSize; i < mSize; i++)
            {
               destroy(&mData[i]);
            }
            mSize = pSize;
            mEnd = &mData[mSize];
         }

         iterator insert(const_iterator pIterator, const T& pElement)
         {
            CflatAssert(pIterator >= mFirst && pIterator <= mEnd);
            const size_t insertionIndex = pIterator - mFirst;
            memmove(mFirst + insertionIndex + 1, mFirst + insertionIndex, (mEnd - pIterator) * sizeof(T));
            mData[insertionIndex] = pElement;
            mSize++;
            mEnd++;
            return mFirst + insertionIndex;
         }
         iterator erase(const_iterator pIterator)
         {
            CflatAssert(pIterator >= mFirst && pIterator <= mEnd);
            destroy(pIterator);
            const size_t deletionIndex = pIterator - mFirst;
            memmove(mFirst + deletionIndex, mFirst + deletionIndex + 1, (mEnd - pIterator - 1) * sizeof(T));
            mSize--;
            mEnd--;
            return mFirst + deletionIndex;
         }

         inline bool operator==(const StackVector<T, Capacity>& pOther) const
         {
            if(mSize != pOther.mSize)
            {
               return false;
            }
            for(size_t i = 0u; i < mSize; i++)
            {
               if(mData[i] != pOther[i])
               {
                  return false;
               }
            }
            return true;
         }
         inline bool operator==(const CflatSTLVector(T)& pSTLVector) const
         {
            if(mSize != pSTLVector.size())
            {
               return false;
            }
            for(size_t i = 0u; i < mSize; i++)
            {
               if(mData[i] != pSTLVector[i])
               {
                  return false;
               }
            }
            return true;
         }
      };

      template<size_t Size>
      struct StackPool
      {
         char mMemory[Size];
         char* mPointer;

         StackPool()
            : mPointer(mMemory)
         {
         }

         void reset()
         {
            mPointer = mMemory;
         }

         const char* push(size_t pSize)
         {
            CflatAssert((mPointer - mMemory + pSize) < Size);

            const char* dataPtr = mPointer;
            mPointer += pSize;

            return dataPtr;
         }
         const char* push(const char* pData, size_t pSize)
         {
            CflatAssert((mPointer - mMemory + pSize) < Size);
            memcpy(mPointer, pData, pSize);

            const char* dataPtr = mPointer;
            mPointer += pSize;

            return dataPtr;
         }
         void pop(size_t pSize)
         {
            mPointer -= pSize;
            CflatAssert(mPointer >= mMemory);
         }
      };

      template<size_t Size>
      struct StringsRegistry
      {
         char mMemory[Size];
         char* mPointer;

         typedef CflatSTLMap(Hash, const char*) Registry;
         Registry mRegistry;

         StringsRegistry()
            : mPointer(mMemory + 1)
         {
            mMemory[0] = '\0';
            mRegistry[0u] = mMemory;
         }

         const char* registerString(Hash pHash, const char* pString)
         {
            Registry::const_iterator it = mRegistry.find(pHash);

            if(it != mRegistry.end())
            {
               return it->second;
            }

            char* ptr = mPointer;
            mRegistry[pHash] = ptr;

            const size_t stringLength = strlen(pString);
            CflatAssert((mPointer + stringLength) < (mMemory + Size));

            memcpy(ptr, pString, stringLength);
            ptr[stringLength] = '\0';

            mPointer += stringLength + 1;

            return ptr;
         }
         const char* retrieveString(Hash pHash)
         {
            Registry::const_iterator it = mRegistry.find(pHash);

            if(it != mRegistry.end())
            {
               return it->second;
            }

            return mMemory;
         }
      };

      template<size_t Size>
      struct WideStringsRegistry
      {
         wchar_t mMemory[Size];
         wchar_t* mPointer;

         typedef CflatSTLMap(Hash, const wchar_t*) Registry;
         Registry mRegistry;

         WideStringsRegistry()
            : mPointer(mMemory + 1)
         {
            mMemory[0] = L'\0';
            mRegistry[0u] = mMemory;
         }

         const wchar_t* registerString(Hash pHash, const char* pString)
         {
            Registry::const_iterator it = mRegistry.find(pHash);

            if(it != mRegistry.end())
            {
               return it->second;
            }

            wchar_t* ptr = mPointer;
            mRegistry[pHash] = ptr;

            const size_t wStrSize = mbstowcs(nullptr, pString, 0);
            const size_t availableSize = Size - 1 - (mPointer - mMemory);

            CflatAssert(wStrSize < availableSize - 1);

            mbstowcs(ptr, pString, availableSize);
            ptr[wStrSize] = L'\0';

            mPointer += wStrSize + 1;

            return ptr;
         }
         const wchar_t* retrieveString(Hash pHash)
         {
            Registry::const_iterator it = mRegistry.find(pHash);

            if(it != mRegistry.end())
            {
               return it->second;
            }

            return mMemory;
         }
      };
   };

   template<typename T1, typename T2>
   bool operator==(const Memory::STLAllocator<T1>&, const Memory::STLAllocator<T2>&) { return true; }
   template<typename T1, typename T2>
   bool operator!=(const Memory::STLAllocator<T1>&, const Memory::STLAllocator<T2>&) { return false; }

   template<typename T>
   bool operator==(const CflatSTLVector(T)& pSTLVector, const CflatArgsVector(T)& pArgsVector)
   {
      return pArgsVector == pSTLVector;
   }


   enum class TypeCategory : uint8_t
   {
      BuiltIn,
      Enum,
      EnumClass,
      StructOrClass
   };

   enum class TypeUsageFlags : uint8_t
   {
      Const         = 1 << 0,
      ConstPointer  = 1 << 1,
      Reference     = 1 << 2,
      Array         = 1 << 3,
      PureRValue    = 1 << 4
   };
   

   CflatAPI Hash hash(const char* pString);


   struct Program;
   class Namespace;


   struct CflatAPI Identifier
   {
      typedef Memory::StringsRegistry<kIdentifierStringsPoolSize> NamesRegistry;
      static NamesRegistry* smNames;

      static NamesRegistry* getNamesRegistry();
      static void releaseNamesRegistry();

      const char* mName;
      uint32_t mNameLength;
      Hash mHash;

      Identifier();
      Identifier(const char* pName);

      const char* findFirstSeparator() const;
      const char* findLastSeparator() const;

      bool operator==(const Identifier& pOther) const;
      bool operator!=(const Identifier& pOther) const;
   };

   struct CflatAPI Type
   {
      Namespace* mNamespace;
      Type* mParent;
      Identifier mIdentifier;
      size_t mSize;
      TypeCategory mCategory;

      virtual ~Type();

   protected:
      Type(Namespace* pNamespace, const Identifier& pIdentifier);

   public:
      virtual Hash getHash() const;

      bool isVoid() const;
      bool isDecimal() const;
      bool isInteger() const;

      bool compatibleWith(const Type& pOther) const;
   };

   struct CflatAPI TypeUsage
   {
      static const CflatArgsVector(TypeUsage)& kEmptyList();

      Type* mType;
      uint16_t mArraySize;
      uint8_t mPointerLevel;
      uint8_t mFlags;

      TypeUsage();

      size_t getSize() const;

      bool isPointer() const;
      bool isConst() const;
      bool isConstPointer() const;
      bool isReference() const;
      bool isArray() const;
      bool isPureRValue() const;

      bool compatibleWith(const TypeUsage& pOther) const;

      bool operator==(const TypeUsage& pOther) const;
      bool operator!=(const TypeUsage& pOther) const;
   };

   struct CflatAPI TypeAlias
   {
     Identifier mIdentifier;
     TypeUsage mTypeUsage;
     uint32_t mScopeLevel;

     TypeAlias();
     TypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
   };

   struct CflatAPI Member
   {
      Identifier mIdentifier;
      TypeUsage mTypeUsage;
      uint16_t mOffset;

      Member(const Identifier& pIdentifier);
   };


   enum class ValueBufferType : uint8_t
   {
      Uninitialized, // uninitialized
      Stack,         // owned, allocated on the stack
      Heap,          // owned, allocated on the heap
      External       // not owned
   };

   enum class ValueInitializationHint : uint8_t
   {
      None, // no hint
      Stack // to be allocated on the stack
   };

   typedef Memory::StackPool<kEnvironmentStackSize> EnvironmentStack;

   struct CflatAPI Value
   {
      static const CflatArgsVector(Value)& kEmptyList();

      TypeUsage mTypeUsage;
      ValueBufferType mValueBufferType;
      ValueInitializationHint mValueInitializationHint;
      char* mValueBuffer;
      EnvironmentStack* mStack;

      Value();
      Value(const Value& pOther);
      ~Value();

      void reset();

      void initOnStack(const TypeUsage& pTypeUsage, EnvironmentStack* pStack);
      void initOnHeap(const TypeUsage& pTypeUsage);
      void initExternal(const TypeUsage& pTypeUsage);

      void set(const void* pDataSource);
      void assign(const void* pDataSource);

      Value& operator=(const Value& pOther);
   };

   struct CflatAPI UsingDirective
   {
      Namespace* mNamespace;
      uint32_t mBlockLevel;

      UsingDirective(Namespace* pNamespace);
   };

   enum class FunctionFlags : uint16_t
   {
      Static = 1 << 0,
      Variadic = 1 << 1
   };

   struct CflatAPI Function
   {
      Namespace* mNamespace;
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      const Program* mProgram;
      uint16_t mLine;
      uint16_t mFlags;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(TypeUsage) mParameters;
      CflatSTLVector(Identifier) mParameterIdentifiers;
      CflatSTLVector(UsingDirective) mUsingDirectives;

      std::function<void(const CflatArgsVector(Value)& pArgs, Value* pOutReturnValue)> execute;

      Function(const Identifier& pIdentifier);
      ~Function();
   };

   enum class MethodFlags : uint16_t
   {
      Const = 1 << 0
   };

   struct CflatAPI Method
   {
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      uint16_t mFlags;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(TypeUsage) mParameters;

      std::function<void(const Value& pThis, const CflatArgsVector(Value)& pArgs, Value* pOutReturnValue)> execute;

      Method(const Identifier& pIdentifier);
      ~Method();
   };

   struct CflatAPI MethodUsage
   {
      Method* mMethod;
      size_t mOffset;

      MethodUsage();
   };

   enum class InstanceFlags : uint16_t
   {
      EnumValue = 1 << 0
   };

   struct CflatAPI Instance
   {
      TypeUsage mTypeUsage;
      Identifier mIdentifier;
      Value mValue;
      uint32_t mScopeLevel;
      uint16_t mFlags;

      Instance();
      Instance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
   };


   class CflatAPI TypesHolder
   {
   private:
      typedef CflatSTLMap(Hash, Type*) TypesRegistry;
      TypesRegistry mTypes;

      typedef CflatSTLMap(Hash, TypeAlias) TypeAliasesRegistry;
      TypeAliasesRegistry mTypeAliases;

   public:
      ~TypesHolder();

      template<typename T>
      T* registerType(const Identifier& pIdentifier, Namespace* pNamespace, Type* pParent)
      {
         T* type = (T*)CflatMalloc(sizeof(T));
         CflatInvokeCtor(T, type)(pNamespace, pIdentifier);

         const Hash hash = type->getHash();
         CflatAssert(mTypes.find(hash) == mTypes.end());

         type->mParent = pParent;
         mTypes[hash] = type;

         return type;
      }

      template<typename T>
      T* registerTemplate(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes,
         Namespace* pNamespace, Type* pParent)
      {
         T* type = (T*)CflatMalloc(sizeof(T));
         CflatInvokeCtor(T, type)(pNamespace, pIdentifier);

         type->mTemplateTypes.resize(pTemplateTypes.size());
         memcpy(&type->mTemplateTypes[0], &pTemplateTypes[0], pTemplateTypes.size() * sizeof(TypeUsage));

         const Hash hash = type->getHash();
         CflatAssert(mTypes.find(hash) == mTypes.end());

         type->mParent = pParent;
         mTypes[hash] = type;

         return type;
      }

      Type* getType(const Identifier& pIdentifier) const;
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes) const;

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier) const;

      bool deregisterType(Type* pType);

      void getAllTypes(CflatSTLVector(Type*)* pOutTypes) const;
   };

   class CflatAPI FunctionsHolder
   {
   private:
      typedef CflatSTLMap(Hash, CflatSTLVector(Function*)) FunctionsRegistry;
      FunctionsRegistry mFunctions;

   public:
      ~FunctionsHolder();

      Function* registerFunction(const Identifier& pIdentifier);

      Function* getFunction(const Identifier& pIdentifier) const;
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* getFunctionPerfectMatch(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;

      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier) const;
      void getAllFunctions(CflatSTLVector(Function*)* pOutFunctions) const;
      size_t getFunctionsCount() const;

      bool deregisterFunctions(const Identifier& pIdentifier);

   private:
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes,
         bool pRequirePerfectMatch) const;
   };

   class CflatAPI InstancesHolder
   {
   private:
      CflatSTLDeque(Instance) mInstances;

   public:
      InstancesHolder();
      ~InstancesHolder();

      Instance* setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier) const;

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier) const;
      void releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors);

      void getAllInstances(CflatSTLVector(Instance*)* pOutInstances) const;
   };


   struct CflatAPI BuiltInType : Type
   {
      BuiltInType(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct CflatAPI Enum : Type
   {
      InstancesHolder mInstancesHolder;

      Enum(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct CflatAPI EnumClass : Type
   {
      InstancesHolder mInstancesHolder;

      EnumClass(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct BaseType
   {
      Type* mType;
      uint16_t mOffset;
   };

   struct CflatAPI Struct : Type
   {
      static const int8_t kInvalidCachedMethodIndex = -1;

      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(BaseType) mBaseTypes;
      CflatSTLVector(Member) mMembers;
      CflatSTLVector(Method) mMethods;

      TypesHolder mTypesHolder;
      FunctionsHolder mFunctionsHolder;
      InstancesHolder mInstancesHolder;

      uint8_t mAlignment;

      int8_t mCachedMethodIndexDefaultConstructor;
      int8_t mCachedMethodIndexCopyConstructor;
      int8_t mCachedMethodIndexDestructor;

      Struct(Namespace* pNamespace, const Identifier& pIdentifier);

      virtual Hash getHash() const override;

      bool derivedFrom(Type* pBaseType) const;
      uint16_t getOffset(Type* pBaseType) const;

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         return mTypesHolder.registerType<T>(pIdentifier, mNamespace, this);
      }
      template<typename T>
      T* registerTemplate(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
      {
         return mTypesHolder.registerTemplate<T>(pIdentifier, pTemplateTypes, mNamespace, this);
      }
      Type* getType(const Identifier& pIdentifier) const;
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes) const;

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier) const;

      Function* registerStaticMethod(const Identifier& pIdentifier);
      Function* getStaticMethod(const Identifier& pIdentifier) const;
      Function* getStaticMethod(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* getStaticMethod(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      CflatSTLVector(Function*)* getStaticMethods(const Identifier& pIdentifier) const;

      void setStaticMember(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
         const Value& pValue);
      Value* getStaticMember(const Identifier& pIdentifier) const;
      Instance* getStaticMemberInstance(const Identifier& pIdentifier) const;

      Member* findMember(const Identifier& pIdentifier) const;

      Method* getDefaultConstructor() const;
      Method* getCopyConstructor() const;
      Method* getDestructor() const;
      Method* findConstructor(const CflatArgsVector(TypeUsage)& pParameterTypes) const;
      Method* findConstructor(const CflatArgsVector(Value)& pArguments) const;
      Method* findMethod(const Identifier& pIdentifier) const;
      Method* findMethod(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Method* findMethod(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* findStaticMethod(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      MethodUsage findMethodUsage(const Identifier& pIdentifier, size_t pOffset,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
   };

   struct CflatAPI Class : Struct
   {
      Class(Namespace* pNamespace, const Identifier& pIdentifier);
   };


   class CflatAPI TypeHelper
   {
   public:
      enum class Compatibility
      {
         PerfectMatch,
         ImplicitCastableInteger,
         ImplicitCastableIntegerFloat,
         ImplicitCastableFloat,
         ImplicitCastableInheritance,
         ImplicitConstructable,
         Incompatible
      };

      static void registerCustomPerfectMatch(Type* pTypeA, Type* pTypeB);
      static void releaseCustomPerfectMatchesRegistry();

      static Compatibility getCompatibility(const TypeUsage& pParameter,
         const TypeUsage& pArgument, uint32_t pRecursionDepth = 0u);
      static size_t calculateAlignment(const TypeUsage& pTypeUsage);

   private:
      static bool isCustomPerfectMatch(Type* pTypeA, Type* pTypeB);

      typedef CflatSTLMap(Hash, CflatSTLSet(Hash)) CustomPerfectMatchesRegistry;
      static CustomPerfectMatchesRegistry smCustomPerfectMatchesRegistry;
   };
   

   enum class TokenType
   {
      Punctuation,
      Number,
      Character,
      WideCharacter,
      String,
      WideString,
      Keyword,
      Identifier,
      Operator
   };

   struct Token
   {
      TokenType mType;
      char* mStart;
      size_t mLength;
      uint16_t mLine;
   };


   class CflatAPI Tokenizer
   {
   public:
      static void tokenize(const char* pCode, CflatSTLVector(Token)& pTokens);

      static bool isValidIdentifierCharacter(char pCharacter);
      static bool isValidIdentifierBeginningCharacter(char pCharacter);
   };


   struct Expression;

   struct Statement;
   struct StatementBlock;
   struct StatementUsingDirective;
   struct StatementTypeDefinition;
   struct StatementNamespaceDeclaration;
   struct StatementVariableDeclaration;
   struct StatementFunctionDeclaration;
   struct StatementStructDeclaration;
   struct StatementIf;
   struct StatementSwitch;
   struct StatementWhile;
   struct StatementDoWhile;
   struct StatementFor;
   struct StatementForRangeBased;
   struct StatementBreak;
   struct StatementContinue;
   struct StatementReturn;

   class Environment;

   struct CflatAPI Program
   {
      Identifier mIdentifier;
      CflatSTLString mCode;
      CflatSTLVector(Statement*) mStatements;

      ~Program();
   };


   class CflatAPI Namespace
   {
   private:
      Identifier mIdentifier;
      Identifier mFullIdentifier;

      Namespace* mParent;
      Environment* mEnvironment;

      typedef CflatSTLMap(Hash, Namespace*) NamespacesRegistry;
      NamespacesRegistry mNamespaces;

      TypesHolder mTypesHolder;
      FunctionsHolder mFunctionsHolder;
      InstancesHolder mInstancesHolder;

      Namespace* getChild(Hash pNameHash) const;

   public:
      Namespace(const Identifier& pName, Namespace* pParent, Environment* pEnvironment);
      ~Namespace();

      const Identifier& getIdentifier() const;
      const Identifier& getFullIdentifier() const;
      Namespace* getParent() const;

      Namespace* getNamespace(const Identifier& pName) const;
      Namespace* requestNamespace(const Identifier& pName);

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         const char* lastSeparator = pIdentifier.findLastSeparator();

         if(lastSeparator)
         {
            char buffer[kDefaultLocalStringBufferSize];
            const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
            strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
            buffer[nsIdentifierLength] = '\0';
            const Identifier nsIdentifier(buffer);
            const Identifier typeIdentifier(lastSeparator + 2);
            return requestNamespace(nsIdentifier)->registerType<T>(typeIdentifier);
         }

         return mTypesHolder.registerType<T>(pIdentifier, this, nullptr);
      }
      template<typename T>
      T* registerTemplate(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
      {
         const char* lastSeparator = pIdentifier.findLastSeparator();

         if(lastSeparator)
         {
            char buffer[kDefaultLocalStringBufferSize];
            const size_t nsIdentifierLength = lastSeparator - pIdentifier.mName;
            strncpy(buffer, pIdentifier.mName, nsIdentifierLength);
            buffer[nsIdentifierLength] = '\0';
            const Identifier nsIdentifier(buffer);
            const Identifier typeIdentifier(lastSeparator + 2);
            return requestNamespace(nsIdentifier)->registerTemplate<T>(typeIdentifier, pTemplateTypes);
         }

         return mTypesHolder.registerTemplate<T>(pIdentifier, pTemplateTypes, this, nullptr);
      }
      Type* getType(const Identifier& pIdentifier, bool pExtendSearchToParent = false) const;
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes,
         bool pExtendSearchToParent = false) const;

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier) const;

      bool deregisterType(Type* pType);

      TypeUsage getTypeUsage(const char* pTypeName) const;

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier, bool pExtendSearchToParent = false) const;
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList(),
         bool pExtendSearchToParent = false) const;
      Function* getFunctionPerfectMatch(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList(),
         bool pExtendSearchToParent = false) const;
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList(),
         bool pExtendSearchToParent = false) const;
      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier,
         bool pExtendSearchToParent = false) const;
      bool deregisterFunctions(const Identifier& pIdentifier);

      Instance* setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier, bool pExtendSearchToParent = false) const;

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier, bool pExtendSearchToParent = false) const;
      void releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors);

      void getAllNamespaces(CflatSTLVector(Namespace*)* pOutNamespaces, bool pRecursively = false) const;
      void getAllTypes(CflatSTLVector(Type*)* pOutTypes, bool pRecursively = false) const;
      void getAllInstances(CflatSTLVector(Instance*)* pOutInstances, bool pRecursively = false) const;
      void getAllFunctions(CflatSTLVector(Function*)* pOutFunctions, bool pRecursively = false) const;
   };


   enum class MacroArgumentType : uint8_t
   {
      Default,
      Stringize,
      TokenPaste
   };

   struct Macro
   {
      uint8_t mParametersCount;
      CflatSTLString mName;
      CflatSTLVector(CflatSTLString) mBody;
   };

   enum class ContextType
   {
      Parsing,
      Execution
   };

   struct CflatAPI Context
   {
   public:
      ContextType mType;

      Program* mProgram;
      uint32_t mBlockLevel;
      uint32_t mScopeLevel;
      CflatSTLVector(Namespace*) mNamespaceStack;
      CflatSTLVector(UsingDirective) mUsingDirectives;
      CflatSTLVector(TypeAlias) mTypeAliases;
      CflatSTLString mStringBuffer;
      InstancesHolder mLocalInstancesHolder;
      EnvironmentStack mStack;

   protected:
      Context(ContextType pType, Namespace* pGlobalNamespace);
   };

   struct CflatAPI ParsingContext : Context
   {
      CflatSTLString mPreprocessedCode;
      CflatSTLVector(Token) mTokens;
      size_t mTokenIndex;

      struct RegisteredInstance
      {
         Identifier mIdentifier;
         Namespace* mNamespace;
         uint32_t mScopeLevel;
      };
      CflatSTLVector(RegisteredInstance) mRegisteredInstances;

      Function* mCurrentFunction;

      struct LocalNamespace
      {
         Namespace* mNamespace;
         uint32_t mScopeLevel;
      };
      CflatSTLVector(LocalNamespace) mLocalNamespaceStack;
      uint32_t mLocalNamespaceGlobalIndex;

      ParsingContext(Namespace* pGlobalNamespace);
   };

   enum class CastType
   {
      CStyle,
      Static,
      Dynamic,
      Reinterpret
   };

   struct CflatAPI CallStackEntry
   {
      const Program* mProgram;
      const Function* mFunction;
      uint16_t mLine;

      CallStackEntry(const Program* pProgram, const Function* pFunction = nullptr);
   };

   typedef CflatSTLVector(CallStackEntry) CallStack;

   enum class JumpStatement : uint16_t
   {
      None,
      Break,
      Continue,
      Return
   };

   struct CflatAPI ExecutionContext : Context
   {
      JumpStatement mJumpStatement;
      Memory::StackVector<Value, kMaxNestedFunctionCalls> mReturnValues;
      CallStack mCallStack;

      ExecutionContext(Namespace* pGlobalNamespace);
   };


   class CflatAPI Environment
   {
   public:
      enum class Settings : uint32_t
      {
         DisallowStaticPointers = 1 << 0,
         DisallowDynamicCast = 1 << 1
      };

   private:
      enum class PreprocessorError : uint8_t
      {
         InvalidPreprocessorDirective,
         InvalidMacroArgumentCount,

         Count
      };
      enum class CompileError : uint8_t
      {
         UnexpectedSymbol,
         Expected,
         UndefinedType,
         UndefinedVariable,
         UndefinedFunction,
         VariableRedefinition,
         ParameterRedefinition,
         UninitializedReference,
         ArrayInitializationExpected,
         NonHomogeneousTypeList,
         TooManyArgumentsInAggregate,
         MismatchingTypeInAggregate,
         NoDefaultConstructor,
         NoCopyConstructor,
         InvalidNumericValue,
         InvalidType,
         InvalidAssignment,
         InvalidMemberAccessOperatorPtr,
         InvalidMemberAccessOperatorNonPtr,
         InvalidOperator,
         InvalidConditionalExpression,
         InvalidCast,
         InvalidEscapeSequence,
         MissingMember,
         MissingStaticMember,
         MissingConstructor,
         MissingMethod,
         MissingStaticMethod,
         NonIntegerValue,
         UnknownNamespace,
         CannotModifyConstExpression,
         CannotCallNonConstMethod,
         MissingReturnStatement,
         MissingReturnExpression,
         IncompatibleReturnExpressionType,
         VoidFunctionReturningValue,
         StaticPointersNotAllowed,
         DynamicCastNotAllowed,

         Count
      };
      enum class RuntimeError : uint8_t
      {
         NullPointerAccess,
         InvalidArrayIndex,
         DivisionByZero,
         MissingFunctionImplementation,

         Count
      };

      uint32_t mSettings;

      CflatSTLVector(Macro) mMacros;

      typedef CflatSTLMap(Hash, Program*) ProgramsRegistry;
      ProgramsRegistry mPrograms;

      typedef Memory::StringsRegistry<kLiteralStringsPoolSize> LiteralStringsPool;
      typedef Memory::WideStringsRegistry<kLiteralStringsPoolSize> LiteralWideStringsPool;
      LiteralStringsPool mLiteralStringsPool;
      LiteralWideStringsPool mLiteralWideStringsPool;

      typedef CflatSTLMap(uint64_t, Value) StaticValuesRegistry;
      StaticValuesRegistry mLocalStaticValues;

      ExecutionContext mExecutionContext;
      CflatSTLString mErrorMessage;

      Namespace mGlobalNamespace;

      Type* mTypeAuto;
      Type* mTypeVoid;
      Type* mTypeInt32;
      Type* mTypeUInt32;
      Type* mTypeFloat;
      Type* mTypeDouble;

      TypeUsage mTypeUsageVoid;
      TypeUsage mTypeUsageSizeT;
      TypeUsage mTypeUsageBool;
      TypeUsage mTypeUsageCString;
      TypeUsage mTypeUsageWideString;
      TypeUsage mTypeUsageCharacter;
      TypeUsage mTypeUsageWideCharacter;
      TypeUsage mTypeUsageVoidPtr;

      typedef void (*ExecutionHook)(Environment* pEnvironment, const CallStack& pCallStack);
      ExecutionHook mExecutionHook;

      void registerBuiltInTypes();

      TypeUsage parseTypeUsage(ParsingContext& pContext, size_t pTokenLastIndex) const;

      void throwPreprocessorError(ParsingContext& pContext, PreprocessorError pError,
         size_t pCursor, const char* pArg = "");
      void throwCompileError(ParsingContext& pContext, CompileError pError,
         const char* pArg1 = "", const char* pArg2 = "");
      void throwCompileErrorUnexpectedSymbol(ParsingContext& pContext);

      void preprocess(ParsingContext& pContext, const char* pCode);
      void tokenize(ParsingContext& pContext) const;
      void parse(ParsingContext& pContext);

      Expression* parseExpression(ParsingContext& pContext, size_t pTokenLastIndex,
         bool pNullAllowed = false);
      Expression* parseExpressionSingleToken(ParsingContext& pContext);
      Expression* parseExpressionMultipleTokens(ParsingContext& pContext,
         size_t pTokenFirstIndex, size_t pTokenLastIndex);

      Expression* parseExpressionLiteralString(ParsingContext& pContext, TokenType pTokenType);
      Expression* parseExpressionLiteralCharacter(ParsingContext& pContext, TokenType pTokenType);
      Expression* parseExpressionUnaryOperator(ParsingContext& pContext, Expression* pOperand,
         const char* pOperator, bool pPostOperator);
      Expression* parseExpressionCast(ParsingContext& pContext, CastType pCastType,
         size_t pTokenLastIndex);
      Expression* parseExpressionFunctionCall(ParsingContext& pContext,
         const Identifier& pFunctionIdentifier);
      Expression* parseExpressionMethodCall(ParsingContext& pContext, Expression* pMemberAccess);
      Expression* parseExpressionObjectConstruction(ParsingContext& pContext, Type* pType);
      Expression* parseExpressionAggregateInitialization(ParsingContext& pContext, Type* pType,
         size_t pTokenLastIndex);

      size_t findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pTokenIndexLimit = 0u) const;
      size_t findOpeningTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pClosureIndex) const;
      size_t findSeparationTokenIndex(ParsingContext& pContext, char pSeparationChar,
         size_t pClosureIndex) const;

      uint8_t getBinaryOperatorPrecedence(ParsingContext& pContext, size_t pTokenIndex) const;
      bool isTemplate(ParsingContext& pContext, size_t pOpeningTokenIndex, size_t pClosureTokenIndex) const;
      bool isTemplate(ParsingContext& pContext, size_t pTokenLastIndex) const;
      
      bool isCastAllowed(CastType pCastType, const TypeUsage& pFrom, const TypeUsage& pTo) const;
      bool isMethodCallAllowed(Method* pMethod, const TypeUsage& pOwnerTypeUsage) const;

      Statement* parseStatement(ParsingContext& pContext);
      StatementBlock* parseStatementBlock(ParsingContext& pContext,
         bool pAlterScope, bool pAllowInGlobalScope);
      StatementUsingDirective* parseStatementUsingDirective(ParsingContext& pContext);
      StatementTypeDefinition* parseStatementTypeDefinition(ParsingContext& pContext);
      StatementNamespaceDeclaration* parseStatementNamespaceDeclaration(ParsingContext& pContext);
      StatementVariableDeclaration* parseStatementVariableDeclaration(ParsingContext& pContext,
         TypeUsage& pTypeUsage, const Identifier& pIdentifier, bool pStatic);
      StatementFunctionDeclaration* parseStatementFunctionDeclaration(ParsingContext& pContext,
         const TypeUsage& pReturnType, bool pStatic);
      StatementStructDeclaration* parseStatementStructDeclaration(ParsingContext& pContext);
      StatementIf* parseStatementIf(ParsingContext& pContext);
      StatementSwitch* parseStatementSwitch(ParsingContext& pContext);
      StatementWhile* parseStatementWhile(ParsingContext& pContext);
      StatementDoWhile* parseStatementDoWhile(ParsingContext& pContext);
      Statement* parseStatementFor(ParsingContext& pContext);
      StatementFor* parseStatementForRegular(ParsingContext& pContext,
         size_t pInitializationClosureTokenIndex);
      StatementForRangeBased* parseStatementForRangeBased(ParsingContext& pContext,
         size_t pVariableClosureTokenIndex);
      StatementBreak* parseStatementBreak(ParsingContext& pContext);
      StatementContinue* parseStatementContinue(ParsingContext& pContext);
      StatementReturn* parseStatementReturn(ParsingContext& pContext);

      bool parseFunctionCallArguments(ParsingContext& pContext, CflatSTLVector(Expression*)* pArguments,
         CflatSTLVector(TypeUsage)* pTemplateTypes = nullptr);

      const TypeUsage& getTypeUsage(Expression* pExpression) const;

      Type* findType(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* findFunction(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;
      Function* findFunction(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList()) const;

      void registerTypeAlias(
        Context& pContext, const Identifier& pIdentifier, const TypeUsage& pTypeUsage);

      Instance* registerInstance(Context& pContext, const TypeUsage& pTypeUsage,
         const Identifier& pIdentifier);
      Instance* retrieveInstance(Context& pContext, const Identifier& pIdentifier) const;

      void incrementBlockLevel(Context& pContext);
      void decrementBlockLevel(Context& pContext);
      void incrementScopeLevel(Context& pContext);
      void decrementScopeLevel(Context& pContext);

      void throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg = "");

      void evaluateExpression(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getAddressOfValue(ExecutionContext& pContext, const Value& pInstanceDataValue, Value* pOutValue);
      void getArgumentValues(ExecutionContext& pContext, const CflatSTLVector(TypeUsage)& pParameters,
         const CflatSTLVector(Expression*)& pExpressions, CflatArgsVector(Value)& pValues);
      void prepareArgumentsForFunctionCall(ExecutionContext& pContext,
         const CflatSTLVector(TypeUsage)& pParameters, const CflatArgsVector(Value)& pOriginalValues,
         CflatArgsVector(Value)& pPreparedValues);
      void applyUnaryOperator(ExecutionContext& pContext, const Value& pOperand, const char* pOperator,
         Value* pOutValue);
      void applyBinaryOperator(ExecutionContext& pContext, const Value& pLeft, const Value& pRight,
         const char* pOperator, Value* pOutValue);
      void performAssignment(ExecutionContext& pContext, const Value& pValue,
         const char* pOperator, Value* pInstanceDataValue);
      void performStaticCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);
      void performIntegerCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);
      void performIntegerFloatCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);
      void performFloatCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);
      void performInheritanceCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);
      void performImplicitConstruction(ExecutionContext& pContext, Type* pCtorType,
         const Value& pCtorArg, Value* pObjectValue);
      void assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
         bool pDeclaration);
      void assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
         bool pDeclaration, TypeHelper::Compatibility pCompatibility);

      static void assertValueInitialization(ExecutionContext& pContext, const TypeUsage& pTypeUsage,
         Value* pOutValue);

      static int64_t getValueAsInteger(const Value& pValue);
      static double getValueAsDecimal(const Value& pValue);
      static void setValueAsInteger(int64_t pInteger, Value* pOutValue);
      static void setValueAsDecimal(double pDecimal, Value* pOutValue);

      static void getTypeFullName(Type* pType, CflatSTLString* pOutString);

      static bool containsReturnStatement(Statement* pStatement);

      void initArgumentsForFunctionCall(Function* pFunction, CflatArgsVector(Value)& pArgs);
      bool tryCallDefaultConstructor(ExecutionContext& pContext, Instance* pInstance, Type* pType, size_t pOffset = 0);

      void execute(ExecutionContext& pContext, const Program& pProgram);
      void execute(ExecutionContext& pContext, Statement* pStatement);

   public:
      static void assignReturnValueFromFunctionCall(const TypeUsage& pReturnTypeUsage,
         const void* pReturnValue, Value* pOutValue);

   public:
      Environment();
      ~Environment();

      void addSetting(Settings pSetting);
      void removeSetting(Settings pSetting);

      void defineMacro(const char* pDefinition, const char* pBody);

      Namespace* getGlobalNamespace();
      Namespace* getNamespace(const Identifier& pIdentifier);
      Namespace* requestNamespace(const Identifier& pIdentifier);

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         return mGlobalNamespace.registerType<T>(pIdentifier);
      }
      template<typename T>
      T* registerTemplate(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes)
      {
         return mGlobalNamespace.registerTemplate<T>(pIdentifier, pTemplateTypes);
      }
      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      Type* getType(const Identifier& pIdentifier) const;
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes) const;

      TypeUsage getTypeUsage(const char* pTypeName, Namespace* pNamespace = nullptr) const;

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier) const;
      Function* getFunction(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pParameterTypes) const;
      Function* getFunction(const Identifier& pIdentifier, const CflatArgsVector(Value)& pArguments) const;
      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier) const;

      Instance* setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier) const;

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier) const;

      void voidFunctionCall(Function* pFunction);
      template<typename ...Args>
      void voidFunctionCall(Function* pFunction, Args... pArgs)
      {
         CflatAssert(pFunction);

         constexpr size_t argsCount = sizeof...(Args);
         CflatAssert(argsCount == pFunction->mParameters.size());

         mErrorMessage.clear();

         Cflat::Value returnValue;

         CflatArgsVector(Value) args;
         initArgumentsForFunctionCall(pFunction, args);

         const void* argData[argsCount] = { pArgs... };

         for(size_t i = 0u; i < argsCount; i++)
         {
            args[i].set(argData[i]);
         }

         pFunction->execute(args, &returnValue);

         while(!args.empty())
         {
            args.pop_back();
         }
      }
      template<typename ReturnType>
      ReturnType returnFunctionCall(Function* pFunction)
      {
         CflatAssert(pFunction);

         mErrorMessage.clear();

         Cflat::Value returnValue;
         returnValue.initOnStack(pFunction->mReturnTypeUsage, &mExecutionContext.mStack);

         CflatArgsVector(Value) args;

         pFunction->execute(args, &returnValue);

         return *(reinterpret_cast<ReturnType*>(returnValue.mValueBuffer));
      }
      template<typename ReturnType, typename ...Args>
      ReturnType returnFunctionCall(Function* pFunction, Args... pArgs)
      {
         CflatAssert(pFunction);

         constexpr size_t argsCount = sizeof...(Args);
         CflatAssert(argsCount == pFunction->mParameters.size());

         mErrorMessage.clear();

         Cflat::Value returnValue;
         returnValue.initOnStack(pFunction->mReturnTypeUsage, &mExecutionContext.mStack);

         CflatArgsVector(Value) args;
         initArgumentsForFunctionCall(pFunction, args);

         const void* argData[argsCount] = { pArgs... };

         for(size_t i = 0u; i < argsCount; i++)
         {
            args[i].set(argData[i]);
         }

         pFunction->execute(args, &returnValue);

         while(!args.empty())
         {
            args.pop_back();
         }

         return *(reinterpret_cast<ReturnType*>(returnValue.mValueBuffer));
      }

      bool load(const char* pProgramName, const char* pCode);
      bool load(const char* pFilePath);

      const char* getErrorMessage();

      void setExecutionHook(ExecutionHook pExecutionHook);
      bool evaluateExpression(const char* pExpression, Value* pOutValue);

      void throwCustomRuntimeError(const char* pErrorMessage);

      void resetStatics();
   };
}

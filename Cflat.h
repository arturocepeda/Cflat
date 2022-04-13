
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

#include <cstdint>
#include <functional>
#include <vector>
#include <map>
#include <string>

#include "CflatConfig.h"

#define CflatMalloc  Cflat::Memory::malloc
#define CflatFree  Cflat::Memory::free

#define CflatHasFlag(pBitMask, pFlag)  ((pBitMask & (int)pFlag) > 0)
#define CflatSetFlag(pBitMask, pFlag)  (pBitMask |= (int)pFlag)
#define CflatResetFlag(pBitMask, pFlag)  (pBitMask &= ~((int)pFlag))

#define CflatInvokeCtor(pClassName, pPtr)  new (pPtr) pClassName
#define CflatInvokeDtor(pClassName, pPtr)  (pPtr)->~pClassName()

#define CflatSTLVector(T)  std::vector<T, Cflat::Memory::STLAllocator<T>>
#define CflatSTLMap(T, U)  std::map<T, U, std::less<T>, Cflat::Memory::STLAllocator<std::pair<const T, U>>>
#define CflatSTLString  std::basic_string<char, std::char_traits<char>, Cflat::Memory::STLAllocator<char>>

#define CflatArgsVector(T)  Cflat::Memory::StackVector<T, Cflat::kArgsVectorSize>

namespace Cflat
{
   typedef uint32_t Hash;

   class Memory
   {
   public:
      static void* (*malloc)(size_t pSize);
      static void (*free)(void* pPtr);

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
      Const      = 1 << 0,
      Reference  = 1 << 1,
      Array      = 1 << 2
   };
   

   Hash hash(const char* pString);


   struct Program;
   class Namespace;


   struct Identifier
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

   struct Type
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

      bool isDecimal() const;
      bool isInteger() const;

      bool compatibleWith(const Type& pOther) const;
   };

   struct TypeUsage
   {
      static const CflatArgsVector(TypeUsage) kEmptyList;

      Type* mType;
      uint16_t mArraySize;
      uint8_t mPointerLevel;
      uint8_t mFlags;

      TypeUsage();

      size_t getSize() const;

      bool isPointer() const;
      bool isConst() const;
      bool isReference() const;
      bool isArray() const;

      bool compatibleWith(const TypeUsage& pOther) const;

      bool operator==(const TypeUsage& pOther) const;
      bool operator!=(const TypeUsage& pOther) const;
   };

   struct TypeAlias
   {
     Identifier mIdentifier;
     TypeUsage mTypeUsage;
     uint32_t mScopeLevel;

     TypeAlias();
     TypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
   };

   struct Member
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

   struct Value
   {
      static const CflatArgsVector(Value) kEmptyList;

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

   struct UsingDirective
   {
      Namespace* mNamespace;
      uint32_t mBlockLevel;

      UsingDirective(Namespace* pNamespace);
   };

   struct Function
   {
      Namespace* mNamespace;
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      const Program* mProgram;
      uint16_t mLine;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(TypeUsage) mParameters;
      CflatSTLVector(Identifier) mParameterIdentifiers;
      CflatSTLVector(UsingDirective) mUsingDirectives;

      std::function<void(const CflatArgsVector(Value)& pArgs, Value* pOutReturnValue)> execute;

      Function(const Identifier& pIdentifier);
      ~Function();
   };

   struct Method
   {
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(TypeUsage) mParameters;

      std::function<void(const Value& pThis, const CflatArgsVector(Value)& pArgs, Value* pOutReturnValue)> execute;

      Method(const Identifier& pIdentifier);
      ~Method();
   };

   struct Instance
   {
      TypeUsage mTypeUsage;
      Identifier mIdentifier;
      uint32_t mScopeLevel;
      Value mValue;

      Instance();
      Instance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
   };


   class TypesHolder
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
         TypesRegistry::const_iterator it = mTypes.find(hash);

         if(it != mTypes.end())
         {
            CflatInvokeDtor(Type, it->second);
            CflatFree(it->second);
         }

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
         TypesRegistry::const_iterator it = mTypes.find(hash);

         if(it != mTypes.end())
         {
            CflatInvokeDtor(Type, it->second);
            CflatFree(it->second);
         }

         type->mParent = pParent;
         mTypes[hash] = type;

         return type;
      }

      Type* getType(const Identifier& pIdentifier);
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes);

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier);
   };

   class FunctionsHolder
   {
   private:
      typedef CflatSTLMap(Hash, CflatSTLVector(Function*)) FunctionsRegistry;
      FunctionsRegistry mFunctions;

   public:
      ~FunctionsHolder();

      Function* registerFunction(const Identifier& pIdentifier);

      Function* getFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);

      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier);
      void getAllFunctions(CflatSTLVector(Function*)* pOutFunctions);
   };

   class InstancesHolder
   {
   private:
      CflatSTLVector(Instance) mInstances;
      size_t mMaxInstances;

   public:
      InstancesHolder(size_t pMaxInstances);
      ~InstancesHolder();

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier);

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier);
      void releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors);

      void getAllInstances(CflatSTLVector(Instance*)* pOutInstances);
   };


   struct BuiltInType : Type
   {
      BuiltInType(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct Enum : Type
   {
      Enum(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct EnumClass : Type
   {
      EnumClass(Namespace* pNamespace, const Identifier& pIdentifier);
   };

   struct BaseType
   {
      Type* mType;
      uint16_t mOffset;
   };

   struct Struct : Type
   {
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(BaseType) mBaseTypes;
      CflatSTLVector(Member) mMembers;
      CflatSTLVector(Method) mMethods;

      TypesHolder mTypesHolder;
      FunctionsHolder mFunctionsHolder;
      InstancesHolder mInstancesHolder;

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
      Type* getType(const Identifier& pIdentifier);
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes);

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier);

      Function* registerStaticMethod(const Identifier& pIdentifier);
      Function* getStaticMethod(const Identifier& pIdentifier);
      Function* getStaticMethod(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* getStaticMethod(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      CflatSTLVector(Function*)* getStaticMethods(const Identifier& pIdentifier);

      void setStaticMember(const TypeUsage& pTypeUsage, const Identifier& pIdentifier,
         const Value& pValue);
      Value* getStaticMember(const Identifier& pIdentifier);
      Instance* getStaticMemberInstance(const Identifier& pIdentifier);
   };

   struct Class : Struct
   {
      Class(Namespace* pNamespace, const Identifier& pIdentifier);
   };


   class TypeHelper
   {
   public:
      enum class Compatibility
      {
         PerfectMatch,
         ImplicitCastableInteger,
         ImplicitCastableIntegerFloat,
         ImplicitCastableFloat,
         ImplicitCastableInheritance,
         Incompatible
      };

      static Compatibility getCompatibility(const TypeUsage& pParameter, const TypeUsage& pArgument);
   };
   

   enum class TokenType
   {
      Punctuation,
      Number,
      String,
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


   class Tokenizer
   {
   public:
      static void tokenize(const char* pCode, CflatSTLVector(Token)& pTokens);
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

   struct Program
   {
      Identifier mIdentifier;
      CflatSTLString mCode;
      CflatSTLVector(Statement*) mStatements;

      ~Program();
   };


   class Namespace
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

      Namespace* getChild(Hash pNameHash);

   public:
      Namespace(const Identifier& pName, Namespace* pParent, Environment* pEnvironment);
      ~Namespace();

      const Identifier& getIdentifier() const;
      const Identifier& getFullIdentifier() const;
      Namespace* getParent();

      Namespace* getNamespace(const Identifier& pName);
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
      Type* getType(const Identifier& pIdentifier, bool pExtendSearchToParent = false);
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes,
         bool pExtendSearchToParent = false);

      void registerTypeAlias(const Identifier& pIdentifier, const TypeUsage& pTypeUsage);
      const TypeAlias* getTypeAlias(const Identifier& pIdentifier);

      TypeUsage getTypeUsage(const char* pTypeName);

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier, bool pExtendSearchToParent = false);
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList,
         bool pExtendSearchToParent = false);
      Function* getFunction(const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList,
         bool pExtendSearchToParent = false);
      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier,
         bool pExtendSearchToParent = false);

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier, bool pExtendSearchToParent = false);

      Instance* registerInstance(const TypeUsage& pTypeUsage, const Identifier& pIdentifier);
      Instance* retrieveInstance(const Identifier& pIdentifier, bool pExtendSearchToParent = false);
      void releaseInstances(uint32_t pScopeLevel, bool pExecuteDestructors);

      void getAllNamespaces(CflatSTLVector(Namespace*)* pOutNamespaces);
      void getAllInstances(CflatSTLVector(Instance*)* pOutInstances);
      void getAllFunctions(CflatSTLVector(Function*)* pOutFunctions);
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

   struct Context
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

   struct ParsingContext : Context
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

      ParsingContext(Namespace* pGlobalNamespace);
   };

   enum class CastType
   {
      CStyle,
      Static,
      Dynamic,
      Reinterpret
   };

   struct CallStackEntry
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

   struct ExecutionContext : Context
   {
      JumpStatement mJumpStatement;
      Memory::StackVector<Value, kMaxNestedFunctionCalls> mReturnValues;
      CallStack mCallStack;

      ExecutionContext(Namespace* pGlobalNamespace);
   };


   class Environment
   {
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
         UninitializedReference,
         ArrayInitializationExpected,
         NoDefaultConstructor,
         InvalidLiteral,
         InvalidType,
         InvalidAssignment,
         InvalidMemberAccessOperatorPtr,
         InvalidMemberAccessOperatorNonPtr,
         InvalidOperator,
         InvalidConditionalExpression,
         InvalidCast,
         MissingMember,
         MissingStaticMember,
         MissingConstructor,
         MissingMethod,
         MissingStaticMethod,
         NonIntegerValue,
         UnknownNamespace,
         CannotModifyConstExpression,

         Count
      };
      enum class RuntimeError : uint8_t
      {
         NullPointerAccess,
         InvalidArrayIndex,
         DivisionByZero,

         Count
      };

      CflatSTLVector(Macro) mMacros;

      typedef CflatSTLMap(Hash, Program*) ProgramsRegistry;
      ProgramsRegistry mPrograms;

      typedef Memory::StringsRegistry<kLiteralStringsPoolSize> LiteralStringsPool;
      LiteralStringsPool mLiteralStringsPool;

      typedef CflatSTLMap(uint64_t, Value) StaticValuesRegistry;
      StaticValuesRegistry mStaticValues;

      ParsingContext mTypesParsingContext;
      ExecutionContext mExecutionContext;
      CflatSTLString mErrorMessage;

      Namespace mGlobalNamespace;

      Type* mTypeAuto;
      Type* mTypeVoid;
      Type* mTypeInt32;
      Type* mTypeUInt32;
      Type* mTypeFloat;
      Type* mTypeDouble;

      TypeUsage mTypeUsageSizeT;
      TypeUsage mTypeUsageBool;
      TypeUsage mTypeUsageCString;
      TypeUsage mTypeUsageVoidPtr;

      typedef void (*ExecutionHook)(Environment* pEnvironment, const CallStack& pCallStack);
      ExecutionHook mExecutionHook;

      void registerBuiltInTypes();

      TypeUsage parseTypeUsage(ParsingContext& pContext, size_t pTokenLastIndex);

      void throwPreprocessorError(ParsingContext& pContext, PreprocessorError pError,
         size_t pCursor, const char* pArg = "");
      void throwCompileError(ParsingContext& pContext, CompileError pError,
         const char* pArg1 = "", const char* pArg2 = "");
      void throwCompileErrorUnexpectedSymbol(ParsingContext& pContext);

      void preprocess(ParsingContext& pContext, const char* pCode);
      void tokenize(ParsingContext& pContext);
      void parse(ParsingContext& pContext);

      Expression* parseExpression(ParsingContext& pContext, size_t pTokenLastIndex,
         bool pNullAllowed = false);
      Expression* parseExpressionSingleToken(ParsingContext& pContext);
      Expression* parseExpressionMultipleTokens(ParsingContext& pContext, size_t pTokenLastIndex);

      Expression* parseExpressionLiteralString(ParsingContext& pContext);
      Expression* parseExpressionUnaryOperator(ParsingContext& pContext, Expression* pOperand,
         const char* pOperator, bool pPostOperator);
      Expression* parseExpressionCast(ParsingContext& pContext, CastType pCastType,
         size_t pTokenLastIndex);
      Expression* parseExpressionFunctionCall(ParsingContext& pContext,
         const Identifier& pFunctionIdentifier);
      Expression* parseExpressionMethodCall(ParsingContext& pContext, Expression* pMemberAccess);
      Expression* parseExpressionObjectConstruction(ParsingContext& pContext, Type* pType);

      size_t findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pTokenIndexLimit = 0u);
      size_t findOpeningTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pClosureIndex);
      size_t findSeparationTokenIndex(ParsingContext& pContext, char pSeparationChar,
         size_t pClosureIndex);

      uint8_t getBinaryOperatorPrecedence(ParsingContext& pContext, size_t pTokenIndex);
      bool isTemplate(ParsingContext& pContext, size_t pOpeningTokenIndex, size_t pClosureTokenIndex);
      bool isTemplate(ParsingContext& pContext, size_t pTokenLastIndex);
      
      static bool isCastAllowed(CastType pCastType, const TypeUsage& pFrom, const TypeUsage& pTo);

      Statement* parseStatement(ParsingContext& pContext);
      StatementBlock* parseStatementBlock(ParsingContext& pContext,
         bool pAlterScope, bool pAllowInGlobalScope);
      StatementUsingDirective* parseStatementUsingDirective(ParsingContext& pContext);
      StatementTypeDefinition* parseStatementTypeDefinition(ParsingContext& pContext);
      StatementNamespaceDeclaration* parseStatementNamespaceDeclaration(ParsingContext& pContext);
      StatementVariableDeclaration* parseStatementVariableDeclaration(ParsingContext& pContext,
         TypeUsage& pTypeUsage, const Identifier& pIdentifier, bool pStatic);
      StatementFunctionDeclaration* parseStatementFunctionDeclaration(ParsingContext& pContext,
         const TypeUsage& pReturnType);
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

      TypeUsage getTypeUsage(Context& pContext, Expression* pExpression);

      Type* findType(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* findFunction(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* findFunction(const Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);

      void registerTypeAlias(
        Context& pContext, const Identifier& pIdentifier, const TypeUsage& pTypeUsage);

      Instance* registerInstance(Context& pContext, const TypeUsage& pTypeUsage,
         const Identifier& pIdentifier);
      Instance* registerStaticInstance(Context& pContext, const TypeUsage& pTypeUsage,
         const Identifier& pIdentifier, void* pUniquePtr);
      Instance* retrieveInstance(Context& pContext, const Identifier& pIdentifier);

      void incrementBlockLevel(Context& pContext);
      void decrementBlockLevel(Context& pContext);
      void incrementScopeLevel(Context& pContext);
      void decrementScopeLevel(Context& pContext);

      void throwRuntimeError(ExecutionContext& pContext, RuntimeError pError, const char* pArg = "");

      void evaluateExpression(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getInstanceDataValue(ExecutionContext& pContext, Expression* pExpression, Value* pOutValue);
      void getAddressOfValue(ExecutionContext& pContext, const Value& pInstanceDataValue, Value* pOutValue);
      void getArgumentValues(ExecutionContext& pContext,
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
      void assignValue(ExecutionContext& pContext, const Value& pSource, Value* pTarget,
         bool pDeclaration);

      static void assertValueInitialization(ExecutionContext& pContext, const TypeUsage& pTypeUsage,
         Value* pOutValue);

      static int64_t getValueAsInteger(const Value& pValue);
      static double getValueAsDecimal(const Value& pValue);
      static void setValueAsInteger(int64_t pInteger, Value* pOutValue);
      static void setValueAsDecimal(double pDecimal, Value* pOutValue);

      static Method* getDefaultConstructor(Type* pType);
      static Method* getDestructor(Type* pType);
      static Method* findConstructor(Type* pType, const CflatArgsVector(TypeUsage)& pParameterTypes);
      static Method* findConstructor(Type* pType, const CflatArgsVector(Value)& pArguments);
      static Method* findMethod(Type* pType, const Identifier& pIdentifier);
      static Method* findMethod(Type* pType, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      static Method* findMethod(Type* pType, const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);

      void initArgumentsForFunctionCall(Function* pFunction, CflatArgsVector(Value)& pArgs);

      void execute(ExecutionContext& pContext, const Program& pProgram);
      void execute(ExecutionContext& pContext, Statement* pStatement);

   public:
      Environment();
      ~Environment();

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
      Type* getType(const Identifier& pIdentifier);
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes);

      TypeUsage getTypeUsage(const char* pTypeName, Namespace* pNamespace = nullptr);

      Function* registerFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier);
      Function* getFunction(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pParameterTypes);
      Function* getFunction(const Identifier& pIdentifier, const CflatArgsVector(Value)& pArguments);
      CflatSTLVector(Function*)* getFunctions(const Identifier& pIdentifier);

      void setVariable(const TypeUsage& pTypeUsage, const Identifier& pIdentifier, const Value& pValue);
      Value* getVariable(const Identifier& pIdentifier);

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
   };
}



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
//  Type validation
//
#define CflatValidateType(pType)  CflatAssert(pType)
#define CflatValidateTypeUsage(pTypeUsage)  CflatAssert(pTypeUsage.mType)


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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
      const pType enumValueInstance = pValueName; \
      Cflat::Value enumValue; \
      enumValue.mTypeUsage.mType = (pOwnerPtr)->getType(#pType); \
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      enumValue.initOnHeap(enumValue.mTypeUsage); \
      enumValue.set(&enumValueInstance); \
      const Cflat::Identifier identifier(#pValueName); \
      (pOwnerPtr)->setVariable(enumValue.mTypeUsage, identifier, enumValue); \
   }
#define CflatNestedEnumAddValue(pOwnerPtr, pParentType, pType, pValueName) \
   { \
      const pParentType::pType enumValueInstance = pParentType::pValueName; \
      Cflat::TypeUsage enumTypeUsage; \
      enumTypeUsage.mType = type; \
      CflatSetFlag(enumTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Struct* parentType = static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentType)); \
      Cflat::Instance* instance = parentType->mInstancesHolder.registerInstance(enumTypeUsage, identifier); \
      instance->mValue.initOnHeap(enumTypeUsage); \
      instance->mValue.set(&enumValueInstance); \
   }


//
//  Type definition: EnumClasses
//
#define CflatRegisterEnumClass(pOwnerPtr, pType) \
   Cflat::EnumClass* type = (pOwnerPtr)->registerType<Cflat::EnumClass>(#pType); \
   type->mSize = sizeof(pType);

#define CflatEnumClassAddValue(pOwnerPtr, pType, pValueName) \
   { \
      const pType enumValueInstance = pType::pValueName; \
      Cflat::Value enumValue; \
      enumValue.mTypeUsage.mType = (pOwnerPtr)->getType(#pType); \
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      enumValue.initOnHeap(enumValue.mTypeUsage); \
      enumValue.set(&enumValueInstance); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Namespace* ns = (pOwnerPtr)->requestNamespace(#pType); \
      ns->setVariable(enumValue.mTypeUsage, identifier, enumValue); \
   }


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pOwnerPtr, pType) \
   Cflat::Struct* type = (pOwnerPtr)->registerType<Cflat::Struct>(#pType); \
   type->mSize = sizeof(pType);
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
      Cflat::Struct* castedBaseType = static_cast<Cflat::Struct*>(baseType.mType); \
      for(size_t i = 0u; i < castedBaseType->mMembers.size(); i++) \
      { \
         type->mMembers.push_back(castedBaseType->mMembers[i]); \
         type->mMembers.back().mOffset += baseType.mOffset; \
      } \
      for(size_t i = 0u; i < castedBaseType->mMethods.size(); i++) \
      { \
         type->mMethods.push_back(castedBaseType->mMethods[i]); \
      } \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
      }; \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pOwnerPtr, pType) \
   Cflat::Class* type = (pOwnerPtr)->registerType<Cflat::Class>(#pType); \
   type->mSize = sizeof(pType);
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
#define _CflatStructDestructorDefine(pEnvironmentPtr, pStructType) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatValueAs(&pThis, pStructType*)->~pStructType(); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
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
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructType, pMethodName, pTemplateType) \
   { \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateType)); CflatValidateTypeUsage(method->mTemplateTypes.back()); \
   }

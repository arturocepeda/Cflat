
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

#include <cstdint>
#include <functional>
#include <vector>
#include <map>
#include <string>

#if !defined (CflatAssert)
# include <cassert>
# define CflatAssert  assert
#endif

#define CflatMalloc  Cflat::Memory::malloc
#define CflatFree  Cflat::Memory::free

#define CflatHasFlag(pBitMask, pFlag)  ((pBitMask & (int)pFlag) > 0)
#define CflatSetFlag(pBitMask, pFlag)  (pBitMask |= (int)pFlag)
#define CflatResetFlag(pBitMask, pFlag)  (pBitMask &= ~((int)pFlag))

#define CflatInvokeCtor(pClassName, pPtr)  new (pPtr) pClassName
#define CflatInvokeDtor(pClassName, pPtr)  pPtr->~pClassName()

#define CflatSTLVector(T)  std::vector<T, Cflat::Memory::STLAllocator<T>>
#define CflatSTLMap(T, U)  std::map<T, U, std::less<T>, Cflat::Memory::STLAllocator<std::pair<const T, U>>>
#define CflatSTLString  std::basic_string<char, std::char_traits<char>, Cflat::Memory::STLAllocator<char>>

#define CflatArgsVector(T)  Cflat::Memory::StackVector<T, 8u>

namespace Cflat
{
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
            mSize--;
            mEnd--;
         }
         void resize(size_t pSize)
         {
            CflatAssert(pSize <= Capacity);
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

         typedef CflatSTLMap(uint32_t, const char*) Registry;
         Registry mRegistry;

         StringsRegistry()
            : mPointer(mMemory + 1)
         {
            mMemory[0] = '\0';
            mRegistry[0u] = mMemory;
         }

         const char* registerString(uint32_t pHash, const char* pString)
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
         const char* retrieveString(uint32_t pHash)
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
   

   uint32_t hash(const char* pString);


   struct Identifier
   {
      typedef Memory::StringsRegistry<8192u> NamesRegistry;
      static NamesRegistry* smNames;

      static NamesRegistry* getNamesRegistry();
      static void releaseNamesRegistry();

      uint32_t mHash;
      const char* mName;

      Identifier();
      Identifier(const char* pName);

      const char* findFirstSeparator() const;
      const char* findLastSeparator() const;

      bool operator==(const Identifier& pOther) const;
      bool operator!=(const Identifier& pOther) const;
   };


   class Namespace;


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
      virtual uint32_t getHash() const;

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

   struct Member
   {
      Identifier mIdentifier;
      TypeUsage mTypeUsage;
      uint16_t mOffset;

      Member(const char* pName);
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

   typedef Memory::StackPool<8192u> EnvironmentStack;

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

      Value& operator=(const Value& pOther);
   };

   struct Function
   {
      Identifier mIdentifier;
      TypeUsage mReturnTypeUsage;
      CflatSTLVector(TypeUsage) mTemplateTypes;
      CflatSTLVector(TypeUsage) mParameters;

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
      typedef CflatSTLMap(uint32_t, Type*) TypesRegistry;
      TypesRegistry mTypes;

   public:
      ~TypesHolder();

      template<typename T>
      T* registerType(const Identifier& pIdentifier, Namespace* pNamespace, Type* pParent)
      {
         T* type = (T*)CflatMalloc(sizeof(T));
         CflatInvokeCtor(T, type)(pNamespace, pIdentifier);
         const uint32_t hash = type->getHash();
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
         const uint32_t hash = type->getHash();
         CflatAssert(mTypes.find(hash) == mTypes.end());
         type->mParent = pParent;
         mTypes[hash] = type;
         return type;
      }

      Type* getType(const Identifier& pIdentifier);
      Type* getType(const Identifier& pIdentifier, const CflatArgsVector(TypeUsage)& pTemplateTypes);
   };

   class FunctionsHolder
   {
   private:
      typedef CflatSTLMap(uint32_t, CflatSTLVector(Function*)) FunctionsRegistry;
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

      virtual uint32_t getHash() const override;

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
   struct StatementNamespaceDeclaration;
   struct StatementVariableDeclaration;
   struct StatementFunctionDeclaration;
   struct StatementIf;
   struct StatementSwitch;
   struct StatementWhile;
   struct StatementDoWhile;
   struct StatementFor;
   struct StatementBreak;
   struct StatementContinue;
   struct StatementReturn;

   struct Program
   {
      Identifier mName;
      CflatSTLString mCode;
      CflatSTLVector(Statement*) mStatements;

      ~Program();
   };


   class Namespace
   {
   private:
      static const size_t kMaxInstances = 32u;

      Identifier mName;
      Identifier mFullName;

      Namespace* mParent;

      typedef CflatSTLMap(uint32_t, Namespace*) NamespacesRegistry;
      NamespacesRegistry mNamespaces;

      TypesHolder mTypesHolder;
      FunctionsHolder mFunctionsHolder;
      InstancesHolder mInstancesHolder;

      Namespace* getChild(uint32_t pNameHash);

   public:
      Namespace(const Identifier& pName, Namespace* pParent);
      ~Namespace();

      const Identifier& getName() const;
      const Identifier& getFullName() const;
      Namespace* getParent();

      Namespace* getNamespace(const Identifier& pName);
      Namespace* requestNamespace(const Identifier& pName);

      template<typename T>
      T* registerType(const Identifier& pIdentifier)
      {
         const char* lastSeparator = pIdentifier.findLastSeparator();

         if(lastSeparator)
         {
            char buffer[256];
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
            char buffer[256];
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

   struct UsingDirective
   {
      Namespace* mNamespace;
      uint32_t mScopeLevel;

      UsingDirective(Namespace* pNamespace);
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
      uint32_t mScopeLevel;
      CflatSTLVector(Namespace*) mNamespaceStack;
      CflatSTLVector(UsingDirective) mUsingDirectives;
      CflatSTLString mStringBuffer;

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
      Static,
      Dynamic,
      Reinterpret
   };

   enum class JumpStatement : uint16_t
   {
      None,
      Break,
      Continue,
      Return
   };

   struct ExecutionContext : Context
   {
      static const size_t kMaxNestedFunctionCalls = 16u;

      uint16_t mCurrentLine;
      EnvironmentStack mStack;
      JumpStatement mJumpStatement;
      CflatSTLVector(Value) mReturnValues;

      ExecutionContext(Namespace* pGlobalNamespace);
   };


   class Environment
   {
   private:
      enum class CompileError : uint8_t
      {
         UnexpectedSymbol,
         Expected,
         UndefinedType,
         UndefinedVariable,
         UndefinedFunction,
         VariableRedefinition,
         ArrayInitializationExpected,
         NoDefaultConstructor,
         InvalidType,
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

      typedef CflatSTLMap(uint32_t, Program) ProgramsRegistry;
      ProgramsRegistry mPrograms;

      typedef Memory::StringsRegistry<1024u> LiteralStringsPool;
      LiteralStringsPool mLiteralStringsPool;

      typedef CflatSTLMap(void*, Value) StaticValuesRegistry;
      StaticValuesRegistry mStaticValues;

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

      typedef std::function<void(Program* pProgram, uint16_t pLine)> ExecutionHook;
      ExecutionHook mExecutionHook;

      void registerBuiltInTypes();

      TypeUsage parseTypeUsage(ParsingContext& pContext);

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

      Expression* parseExpressionCast(ParsingContext& pContext, CastType pCastType,
         size_t pTokenLastIndex);
      Expression* parseExpressionFunctionCall(ParsingContext& pContext,
         const Identifier& pFunctionIdentifier);
      Expression* parseExpressionMethodCall(ParsingContext& pContext, Expression* pMemberAccess);
      Expression* parseExpressionObjectConstruction(ParsingContext& pContext, Type* pType);
      Expression* parseImmediateExpression(ParsingContext& pContext, size_t pTokenLastIndex);

      size_t findClosureTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pTokenIndexLimit = 0u);
      size_t findOpeningTokenIndex(ParsingContext& pContext, char pOpeningChar, char pClosureChar,
         size_t pClosureIndex);
      size_t findSeparationTokenIndex(ParsingContext& pContext, char pSeparationChar,
         size_t pClosureIndex);

      uint8_t getBinaryOperatorPrecedence(ParsingContext& pContext, size_t pTokenIndex);
      bool isTemplate(ParsingContext& pContext, size_t pOpeningTokenIndex, size_t pClosureTokenIndex);
      bool isTemplate(ParsingContext& pContext, size_t pTokenLastIndex);

      Statement* parseStatement(ParsingContext& pContext);
      StatementBlock* parseStatementBlock(ParsingContext& pContext, bool pAlterScope);
      StatementUsingDirective* parseStatementUsingDirective(ParsingContext& pContext);
      StatementNamespaceDeclaration* parseStatementNamespaceDeclaration(ParsingContext& pContext);
      StatementVariableDeclaration* parseStatementVariableDeclaration(ParsingContext& pContext,
         TypeUsage& pTypeUsage, const Identifier& pIdentifier, bool pStatic);
      StatementFunctionDeclaration* parseStatementFunctionDeclaration(ParsingContext& pContext,
         const TypeUsage& pReturnType);
      StatementIf* parseStatementIf(ParsingContext& pContext);
      StatementSwitch* parseStatementSwitch(ParsingContext& pContext);
      StatementWhile* parseStatementWhile(ParsingContext& pContext);
      StatementDoWhile* parseStatementDoWhile(ParsingContext& pContext);
      StatementFor* parseStatementFor(ParsingContext& pContext);
      StatementBreak* parseStatementBreak(ParsingContext& pContext);
      StatementContinue* parseStatementContinue(ParsingContext& pContext);
      StatementReturn* parseStatementReturn(ParsingContext& pContext);

      bool parseFunctionCallArguments(ParsingContext& pContext, CflatSTLVector(Expression*)* pArguments,
         CflatSTLVector(TypeUsage)* pTemplateTypes = nullptr);

      TypeUsage getTypeUsage(Context& pContext, Expression* pExpression);

      Type* findType(Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* findFunction(Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(TypeUsage)& pParameterTypes,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);
      Function* findFunction(Context& pContext, const Identifier& pIdentifier,
         const CflatArgsVector(Value)& pArguments,
         const CflatArgsVector(TypeUsage)& pTemplateTypes = TypeUsage::kEmptyList);

      Instance* registerInstance(Context& pContext, const TypeUsage& pTypeUsage,
         const Identifier& pIdentifier);
      Instance* registerStaticInstance(Context& pContext, const TypeUsage& pTypeUsage,
         const Identifier& pIdentifier, void* pUniquePtr);
      Instance* retrieveInstance(const Context& pContext, const Identifier& pIdentifier);

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
      void applyUnaryOperator(ExecutionContext& pContext, const char* pOperator, Value* pOutValue);
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
      void performInheritanceCast(ExecutionContext& pContext, const Value& pValueToCast,
         const TypeUsage& pTargetTypeUsage, Value* pOutValue);

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
      TypeUsage getTypeUsage(const char* pTypeName);

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
         CflatAssert(argsCount == 0u);

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
   };
}



//
//  Value retrieval
//
#define CflatValueAs(pValuePtr, pTypeName) \
   (*reinterpret_cast<pTypeName*>((pValuePtr)->mValueBuffer))
#define CflatValueArrayElementAs(pValuePtr, pArrayElementIndex, pTypeName) \
   (*reinterpret_cast<pTypeName*>((pValuePtr)->mValueBuffer + (pArrayElementIndex * sizeof(pTypeName))))


//
//  Function definition
//
#define CflatRegisterFunctionVoid(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName(); \
      }; \
   }
#define CflatRegisterFunctionVoidParams1(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams2(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams3(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
      }; \
   }
#define CflatRegisterFunctionVoidParams4(pEnvironmentPtr, pVoid,pVoidRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
      }; \
   }
#define CflatRegisterFunctionReturn(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams1(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams2(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams3(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatRegisterFunctionReturnParams4(pEnvironmentPtr, pReturnTypeName,pReturnRef, pFunctionName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      Cflat::Function* function = (pEnvironmentPtr)->registerFunction(#pFunctionName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pFunctionName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }



//
//  Type definition: Built-in types
//
#define CflatRegisterBuiltInType(pEnvironmentPtr, pTypeName) \
   { \
      Cflat::BuiltInType* type = (pEnvironmentPtr)->registerType<Cflat::BuiltInType>(#pTypeName); \
      type->mSize = sizeof(pTypeName); \
   }


//
//  Type definition: Enums
//
#define CflatRegisterEnum(pOwnerPtr, pTypeName) \
   Cflat::Enum* type = (pOwnerPtr)->registerType<Cflat::Enum>(#pTypeName); \
   type->mSize = sizeof(pTypeName);

#define CflatEnumAddValue(pEnvironmentPtr, pTypeName, pValueName) \
   { \
      const pTypeName enumValueInstance = pValueName; \
      Cflat::Value enumValue; \
      enumValue.mTypeUsage.mType = (pEnvironmentPtr)->getType(#pTypeName); \
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      enumValue.initOnHeap(enumValue.mTypeUsage); \
      enumValue.set(&enumValueInstance); \
      const Cflat::Identifier identifier(#pValueName); \
      (pEnvironmentPtr)->setVariable(enumValue.mTypeUsage, identifier, enumValue); \
   }


//
//  Type definition: EnumClasses
//
#define CflatRegisterEnumClass(pOwnerPtr, pTypeName) \
   Cflat::EnumClass* type = (pOwnerPtr)->registerType<Cflat::EnumClass>(#pTypeName); \
   type->mSize = sizeof(pTypeName);

#define CflatEnumClassAddValue(pEnvironmentPtr, pTypeName, pValueName) \
   { \
      const pTypeName enumValueInstance = pTypeName::pValueName; \
      Cflat::Value enumValue; \
      enumValue.mTypeUsage.mType = (pEnvironmentPtr)->getType(#pTypeName); \
      CflatSetFlag(enumValue.mTypeUsage.mFlags, Cflat::TypeUsageFlags::Const); \
      enumValue.initOnHeap(enumValue.mTypeUsage); \
      enumValue.set(&enumValueInstance); \
      const Cflat::Identifier identifier(#pValueName); \
      Cflat::Namespace* ns = (pEnvironmentPtr)->requestNamespace(#pTypeName); \
      ns->setVariable(enumValue.mTypeUsage, identifier, enumValue); \
   }


//
//  Type definition: Structs
//
#define CflatRegisterStruct(pOwnerPtr, pTypeName) \
   Cflat::Struct* type = (pOwnerPtr)->registerType<Cflat::Struct>(#pTypeName); \
   type->mSize = sizeof(pTypeName);
#define CflatRegisterNestedStruct(pOwnerPtr, pParentTypeName, pTypeName) \
   using pTypeName = pParentTypeName::pTypeName; \
   CflatRegisterStruct(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentTypeName)), pTypeName);

#define CflatStructAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   { \
      Cflat::BaseType baseType; \
      baseType.mType = (pEnvironmentPtr)->getType(#pBaseTypeName); \
      CflatAssert(baseType.mType); \
      pTypeName* derivedTypePtr = reinterpret_cast<pTypeName*>(0x1); \
      pBaseTypeName* baseTypePtr = static_cast<pBaseTypeName*>(derivedTypePtr); \
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
#define CflatStructAddMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::Member member(#pMemberName); \
      member.mTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberTypeName); \
      CflatAssert(member.mTypeUsage.mType); \
      member.mTypeUsage.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      member.mOffset = (uint16_t)offsetof(pStructTypeName, pMemberName); \
      type->mMembers.push_back(member); \
   }
#define CflatStructAddStaticMember(pEnvironmentPtr, pStructTypeName, pMemberTypeName, pMemberName) \
   { \
      Cflat::TypeUsage typeUsage = (pEnvironmentPtr)->getTypeUsage(#pMemberTypeName); \
      CflatAssert(typeUsage.mType); \
      typeUsage.mArraySize = (uint16_t)(sizeof(pStructTypeName::pMemberName) / sizeof(pMemberTypeName)); \
      Cflat::Value value; \
      value.initExternal(typeUsage); \
      value.set(&pStructTypeName::pMemberName); \
      static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->setStaticMember(typeUsage, #pMemberName, value); \
   }
#define CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefine(pEnvironmentPtr, pStructTypeName); \
   }
#define CflatStructAddConstructorParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddConstructorParams2(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams2(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref); \
   }
#define CflatStructAddConstructorParams3(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams3(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref); \
   }
#define CflatStructAddConstructorParams4(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructConstructorDefineParams4(pEnvironmentPtr, pStructTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref); \
   }
#define CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName) \
   { \
      _CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName); \
      _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName); \
   }
#define CflatStructAddMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName); \
   }
#define CflatStructAddMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddMethodVoidParams2(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref); \
   }
#define CflatStructAddMethodVoidParams3(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref); \
   }
#define CflatStructAddMethodVoidParams4(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructTypeName, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref); \
   }
#define CflatStructAddMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName); \
   }
#define CflatStructAddMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddMethodReturnParams2(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref); \
   }
#define CflatStructAddMethodReturnParams3(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref); \
   }
#define CflatStructAddMethodReturnParams4(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref); \
   }

#define CflatStructAddTemplateMethodVoid(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName<pTemplateTypeName>); \
   }
#define CflatStructAddTemplateMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddTemplateMethodVoidParams2(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructTypeName, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref); \
   }
#define CflatStructAddTemplateMethodVoidParams3(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructTypeName, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref); \
   }
#define CflatStructAddTemplateMethodVoidParams4(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructTypeName, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref); \
   }
#define CflatStructAddTemplateMethodReturn(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName<pTemplateTypeName>); \
   }
#define CflatStructAddTemplateMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref); \
   }
#define CflatStructAddTemplateMethodReturnParams2(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref); \
   }
#define CflatStructAddTemplateMethodReturnParams3(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref); \
   }
#define CflatStructAddTemplateMethodReturnParams4(pEnvironmentPtr, pStructTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName); \
      _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName); \
      _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName<pTemplateTypeName>, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref); \
   }

#define CflatStructAddStaticMethodVoid(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructTypeName::pMethodName(); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams2(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams3(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodVoidParams4(pEnvironmentPtr, pStructTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
      }; \
   }
#define CflatStructAddStaticMethodReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pStructTypeName::pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams2(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams3(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define CflatStructAddStaticMethodReturnParams4(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      Cflat::Function* function = static_cast<Cflat::Struct*>((pEnvironmentPtr)->getType(#pStructTypeName))->registerStaticMethod(#pMethodName); \
      function->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      function->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      function->execute = [function](const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         CflatAssert(function->mParameters.size() == pArguments.size()); \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(function->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = pStructTypeName::pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }


//
//  Type definition: Classes
//
#define CflatRegisterClass(pOwnerPtr, pTypeName) \
   Cflat::Class* type = (pOwnerPtr)->registerType<Cflat::Class>(#pTypeName); \
   type->mSize = sizeof(pTypeName);
#define CflatRegisterNestedClass(pOwnerPtr, pParentTypeName, pTypeName) \
   using pTypeName = pParentTypeName::pTypeName; \
   CflatRegisterClass(static_cast<Cflat::Struct*>((pOwnerPtr)->getType(#pParentTypeName)), pTypeName);

#define CflatClassAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   { \
      CflatStructAddBaseType(pEnvironmentPtr, pTypeName, pBaseTypeName) \
   }
#define CflatClassAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   { \
      CflatStructAddMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   }
#define CflatClassAddStaticMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   { \
      CflatStructAddStaticMember(pEnvironmentPtr, pClassTypeName, pMemberTypeName, pMemberName) \
   }
#define CflatClassAddConstructor(pEnvironmentPtr, pClassTypeName) \
   { \
      CflatStructAddConstructor(pEnvironmentPtr, pClassTypeName) \
   }
#define CflatClassAddConstructorParams1(pEnvironmentPtr, pClassTypeName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddConstructorParams1(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddConstructorParams2(pEnvironmentPtr, pClassTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddConstructorParams2(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddConstructorParams3(pEnvironmentPtr, pClassTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddConstructorParams3(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddConstructorParams4(pEnvironmentPtr, pClassTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddConstructorParams4(pEnvironmentPtr, pClassTypeName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }
#define CflatClassAddDestructor(pEnvironmentPtr, pClassTypeName) \
   { \
      CflatStructAddDestructor(pEnvironmentPtr, pClassTypeName) \
   }
#define CflatClassAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatStructAddMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   }
#define CflatClassAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }
#define CflatClassAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatStructAddMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   }
#define CflatClassAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }

#define CflatClassAddTemplateMethodVoid(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatStructAddTemplateMethodVoid(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName) \
   }
#define CflatClassAddTemplateMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddTemplateMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddTemplateMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddTemplateMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddTemplateMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddTemplateMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddTemplateMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddTemplateMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }
#define CflatClassAddTemplateMethodReturn(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatStructAddTemplateMethodReturn(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   }
#define CflatClassAddTemplateMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddTemplateMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddTemplateMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddTemplateMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddTemplateMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddTemplateMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddTemplateMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddTemplateMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pTemplateTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }

#define CflatClassAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   { \
      CflatStructAddStaticMethodVoid(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName) \
   }
#define CflatClassAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddStaticMethodVoidParams1(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddStaticMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddStaticMethodVoidParams2(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddStaticMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddStaticMethodVoidParams3(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddStaticMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddStaticMethodVoidParams4(pEnvironmentPtr, pClassTypeName, pVoid,pVoidRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }
#define CflatClassAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      CflatStructAddStaticMethodReturn(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   }
#define CflatClassAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref) \
   { \
      CflatStructAddStaticMethodReturnParams1(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref) \
   }
#define CflatClassAddStaticMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      CflatStructAddStaticMethodReturnParams2(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref) \
   }
#define CflatClassAddStaticMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      CflatStructAddStaticMethodReturnParams3(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref) \
   }
#define CflatClassAddStaticMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      CflatStructAddStaticMethodReturnParams4(pEnvironmentPtr, pClassTypeName, pReturnTypeName,pReturnRef, pMethodName, \
         pParam0TypeName,pParam0Ref, \
         pParam1TypeName,pParam1Ref, \
         pParam2TypeName,pParam2Ref, \
         pParam3TypeName,pParam3Ref) \
   }


//
//  Type definition: Templates
//
#define CflatRegisterTemplateStructTypeNames1(pEnvironmentPtr, pTypeName, pTemplateTypeName) \
   CflatArgsVector(Cflat::TypeUsage) templateTypeNames; \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName)); \
   Cflat::Struct* type = (pEnvironmentPtr)->registerTemplate<Cflat::Struct>(#pTypeName, templateTypeNames); \
   type->mSize = sizeof(pTypeName<pTemplateTypeName>);
#define CflatRegisterTemplateStructTypeNames2(pEnvironmentPtr, pTypeName, pTemplateTypeName1, pTemplateTypeName2) \
   CflatArgsVector(Cflat::TypeUsage) templateTypeNames; \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName1)); \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName2)); \
   Cflat::Struct* type = (pEnvironmentPtr)->registerTemplate<Cflat::Struct>(#pTypeName, templateTypeNames); \
   type->mSize = sizeof(pTypeName<pTemplateTypeName1, pTemplateTypeName2>);

#define CflatRegisterTemplateClassTypeNames1(pEnvironmentPtr, pTypeName, pTemplateTypeName) \
   CflatArgsVector(Cflat::TypeUsage) templateTypeNames; \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName)); \
   Cflat::Class* type = (pEnvironmentPtr)->registerTemplate<Cflat::Class>(#pTypeName, templateTypeNames); \
   type->mSize = sizeof(pTypeName<pTemplateTypeName>);
#define CflatRegisterTemplateClassTypeNames2(pEnvironmentPtr, pTypeName, pTemplateTypeName1, pTemplateTypeName2) \
   CflatArgsVector(Cflat::TypeUsage) templateTypeNames; \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName1)); \
   templateTypeNames.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName2)); \
   Cflat::Class* type = (pEnvironmentPtr)->registerTemplate<Cflat::Class>(#pTypeName, templateTypeNames); \
   type->mSize = sizeof(pTypeName<pTemplateTypeName1, pTemplateTypeName2>);



//
//  Internal macros - helpers for the user macros
//
#define _CflatStructAddConstructor(pEnvironmentPtr, pStructTypeName) \
   { \
      Cflat::Method method(""); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddDestructor(pEnvironmentPtr, pStructTypeName) \
   { \
      Cflat::Method method("~"); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructAddMethod(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      Cflat::Method method(#pMethodName); \
      type->mMethods.push_back(method); \
   }
#define _CflatStructConstructorDefine(pEnvironmentPtr, pStructTypeName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         new (CflatValueAs(&pThis, pStructTypeName*)) pStructTypeName(); \
      }; \
   }
#define _CflatStructConstructorDefineParams1(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructTypeName*)) pStructTypeName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams2(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructTypeName*)) pStructTypeName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams3(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructTypeName*)) pStructTypeName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
      }; \
   }
#define _CflatStructConstructorDefineParams4(pEnvironmentPtr, pStructTypeName, \
   pParam0TypeName,pParam0Ref, \
   pParam1TypeName,pParam1Ref, \
   pParam2TypeName,pParam2Ref, \
   pParam3TypeName,pParam3Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         new (CflatValueAs(&pThis, pStructTypeName*)) pStructTypeName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
      }; \
   }
#define _CflatStructDestructorDefine(pEnvironmentPtr, pStructTypeName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatValueAs(&pThis, pStructTypeName*)->~pStructTypeName(); \
      }; \
   }
#define _CflatStructMethodDefineVoid(pEnvironmentPtr, pStructTypeName, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatValueAs(&pThis, pStructTypeName*)->pMethodName(); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams1(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams2(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams3(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref, \
      pParam2TypeName,pParam2Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineVoidParams4(pEnvironmentPtr, pStructTypeName, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref, \
      pParam2TypeName,pParam2Ref, \
      pParam3TypeName,pParam3Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
      }; \
   }
#define _CflatStructMethodDefineReturn(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         pReturnTypeName pReturnRef result = CflatValueAs(&pThis, pStructTypeName*)->pMethodName(); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams1(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
      pParam0TypeName,pParam0Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams2(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams3(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref, \
      pParam2TypeName,pParam2Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineReturnParams4(pEnvironmentPtr, pStructTypeName, pReturnTypeName,pReturnRef, pMethodName, \
      pParam0TypeName,pParam0Ref, \
      pParam1TypeName,pParam1Ref, \
      pParam2TypeName,pParam2Ref, \
      pParam3TypeName,pParam3Ref) \
   { \
      const size_t methodIndex = type->mMethods.size() - 1u; \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mReturnTypeUsage = (pEnvironmentPtr)->getTypeUsage(#pReturnTypeName #pReturnRef); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam0TypeName #pParam0Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam1TypeName #pParam1Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam2TypeName #pParam2Ref)); \
      method->mParameters.push_back((pEnvironmentPtr)->getTypeUsage(#pParam3TypeName #pParam3Ref)); \
      method->execute = [type, methodIndex] \
         (const Cflat::Value& pThis, const CflatArgsVector(Cflat::Value)& pArguments, Cflat::Value* pOutReturnValue) \
      { \
         Cflat::Method* method = &type->mMethods[methodIndex]; \
         CflatAssert(pOutReturnValue); \
         CflatAssert(pOutReturnValue->mTypeUsage.compatibleWith(method->mReturnTypeUsage)); \
         CflatAssert(method->mParameters.size() == pArguments.size()); \
         pReturnTypeName pReturnRef result = CflatValueAs(&pThis, pStructTypeName*)->pMethodName \
         ( \
            CflatValueAs(&pArguments[0], pParam0TypeName), \
            CflatValueAs(&pArguments[1], pParam1TypeName), \
            CflatValueAs(&pArguments[2], pParam2TypeName), \
            CflatValueAs(&pArguments[3], pParam3TypeName) \
         ); \
         pOutReturnValue->set(&result); \
      }; \
   }
#define _CflatStructMethodDefineTemplateType(pEnvironmentPtr, pStructTypeName, pMethodName, pTemplateTypeName) \
   { \
      Cflat::Method* method = &type->mMethods.back(); \
      method->mTemplateTypes.push_back((pEnvironmentPtr)->getTypeUsage(#pTemplateTypeName)); \
   }

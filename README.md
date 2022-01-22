# Cflat
### Embeddable lightweight scripting language with C++ syntax

Embeddable scripting languages provide the possibility of implementing and tweaking features during the software development process without the need of recompiling or restarting after each code change. This is particularly useful when working with large codebases which take long to compile.

Unfortunately, such an advantage usually implies some additional runtime costs, like a slow execution time (at least in comparison with what would be an equivalent compiled version of the same code written in C++) and a big amount of heap allocations, which might be considerable in performance-critical software like videogames.

Cflat is an embeddable scripting language whose syntax is 100% compatible with C++, what means that all the scripts written for Cflat can be actually compiled in release builds along with the rest of the code.

**In this case, compile means compile** - the scripts are not compiled into some kind of bytecode, though: they are compiled into machine code for the specific platform you are targeting, just like the rest of the C++ code from the project.

Both software engineers and other members of the team can benefit from Cflat. Regarding the second group, one might say that C++ is not the best choice for developers who are not software engineers or programmers, and C++ is indeed not as friendly as other scripting languages, but the truth is that high-level C++ code, which is the kind of code you usually write in scripts, does not look that different from other widely used languages like C# or Java(Script).

Cflat does not intend to be a fully featured C++ interpreter, but rather a lightweight scripting language whose syntax is 100% compatible with C++. This means that:
- Only a (small) subset of features from C++ are available in Cflat
- Everything from Cflat can be compiled with any C++11 compliant compiler

In case you are looking for a proper C++ interpreter, you might want to take a look at the following alternatives:
- [cling](https://github.com/root-project/cling) (extremely heavy, depends on clang and llvm)
- [Ch](https://www.softintegration.com/) (commercial)
- [CINT](http://www.hanno.jp/gotom/Cint.html)

Or, if what you need is a way of getting your C++ code hot-reloaded and recompiled, there are also some good options out there:
- [Runtime Compiled C++](https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus)
- [Live++](https://liveplusplus.tech/) (commercial)


## FAQ

Is any C++ code compatible with Cflat?
- *No. Cflat supports only a rather small subset of features from the C++11 standard.*


Is any Cflat code compatible with C++?
- *Yes. Cflat code can be compiled with any existing C++11 compiler.*


Is it in Cflat's roadmap to eventually support all the features from C++?
- *No. The idea is to keep it simple and lightweight. Although it is indeed planned to progressively add features to it and make it more powerful, Cflat will never provide all the features any C++ standard provides.*


Is Cflat going to provide any extra features outside from C++ in the future?
- *No. Cflat code shall always be 100% compatible with C++.*


Does Cflat require any external dependencies?
- *No. Only standard C++11 features and STL containers are required.*


Is Cflat cross-platform?
- *Yes, as cross-platform as any C++ code can be. If there is a C++11 compiler for the platform you are targeting, Cflat can be used on it.*


## Documentation
### Getting started

To integrate Cflat in a project, you just have to add the `Cflat.cpp` file to it and make sure that the header files from the Cflat directory are accessible. In order to take advantage of Cflat, you will need a Cflat environment:

```cpp
#include "Cflat/Cflat.h"

// ...

Cflat::Environment env;
```

You can then register the functions and types you would like to have exposed for scripting. Note that Cflat environments are empty by default, without anything registered apart from the built-in types, so you can decide exactly what you need to expose. The `Cflat.h` header provides you with a bunch of convenience macros to do that:

```cpp
{
   CflatRegisterFunctionReturnParams1(&env, float, floor, float);
   CflatRegisterFunctionReturnParams1(&env, float, sqrtf, float);
   CflatRegisterFunctionReturnParams2(&env, float, powf, float, float);
}
```

```cpp
struct TestStruct
{
   int var1;
   int var2;
};

{
   CflatRegisterStruct(&env, TestStruct);
   CflatStructAddMember(&env, TestStruct, int, var1);
   CflatStructAddMember(&env, TestStruct, int, var2);
}
```

```cpp
enum TestEnum
{
   kFirstValue,
   kSecondValue
};

{
   CflatRegisterEnum(&env, TestEnum);
   CflatEnumAddValue(&env, TestEnum, kFirstValue);
   CflatEnumAddValue(&env, TestEnum, kSecondValue);
}
```

```cpp
enum class TestEnum
{
   kFirstValue,
   kSecondValue
};

{
   CflatRegisterEnumClass(&env, TestEnum);
   CflatEnumClassAddValue(&env, TestEnum, kFirstValue);
   CflatEnumClassAddValue(&env, TestEnum, kSecondValue);
}
``` 

```cpp
struct Base
{
   int baseMember;
};
struct Derived : Base
{
   int derivedMember;
};

{
   CflatRegisterStruct(&env, Base);
   CflatStructAddMember(&env, Base, int, baseMember);
}
{
   CflatRegisterStruct(&env, Derived);
   CflatStructAddBaseType(&env, Derived, Base);
   CflatStructAddMember(&env, Derived, int, derivedMember);
}
```

```cpp
struct BaseA
{
   int baseAMember;
};
struct BaseB
{
   int baseBMember;
};
struct Derived : BaseA, BaseB
{
   int derivedMember;
};

{
   CflatRegisterStruct(&env, BaseA);
   CflatStructAddMember(&env, BaseA, int, baseAMember);
}
{
   CflatRegisterStruct(&env, BaseB);
   CflatStructAddMember(&env, BaseB, int, baseBMember);
}
{
   CflatRegisterStruct(&env, Derived);
   CflatStructAddBaseType(&env, Derived, BaseA);
   CflatStructAddBaseType(&env, Derived, BaseB);
   CflatStructAddMember(&env, Derived, int, derivedMember);
}
```

```cpp
struct OuterType
{
   struct InnerType
   {
      int value;
   };
   enum InnerEnum
   {
      kInnerEnumValue
   };
};

{
   CflatRegisterStruct(&env, OuterType);
}
{
   CflatRegisterNestedStruct(&env, OuterType, InnerType);
   CflatStructAddMember(&env, InnerType, int, value);
}
{
   CflatRegisterNestedEnum(&env, OuterType, InnerEnum);
   CflatNestedEnumAddValue(&env, OuterType, InnerEnum, kInnerEnumValue);
}
```

```cpp
struct TestStruct
{
   TestStruct();
   ~TestStruct();

   void method(int pValue);

   static void staticMethod(int pValue);
};

{
   CflatRegisterStruct(&env, TestStruct);
   CflatStructAddConstructor(&env, TestStruct);
   CflatStructAddDestructor(&env, TestStruct);
   CflatStructAddMethodVoidParams1(&env, TestStruct, void, method, int);
   CflatStructAddStaticMethodVoidParams1(&env, TestStruct, void, staticMethod, int);
}
```

The first argument for the macros can be both a pointer to the environment, or a pointer to the namespace where the type to register is defined:

```cpp
namespace Math
{
   struct Vector3
   {
      float x;
      float y;
      float z;
   };
}

{
   using namespace Math;
   Cflat::Namespace* ns = env.requestNamespace("Math");

   {
      CflatRegisterStruct(ns, Vector3);
      CflatStructAddMember(ns, Vector3, float, x);
      CflatStructAddMember(ns, Vector3, float, y);
      CflatStructAddMember(ns, Vector3, float, z);
   }
}
```

**Overloaded methods or functions** must be registered once per overload:

```cpp
class TestClass
{
public:
   void method(int pValue);
   void method(float pValue);
};

{
   CflatRegisterClass(&env, TestClass);
   CflatClassAddMethodVoidParams1(&env, TestClass, void, method, int);
   CflatClassAddMethodVoidParams1(&env, TestClass, void, method, float);
}
```

Methods and functions with **default arguments** must be registered once per number of parameters:

```cpp
class TestClass
{
public:
   void method(int pValue1 = 0, int pValue2 = 0);
};

{
   CflatRegisterClass(&env, TestClass);
   CflatClassAddMethodVoid(&env, TestClass, void, method);
   CflatClassAddMethodVoidParams1(&env, TestClass, void, method, int);
   CflatClassAddMethodVoidParams2(&env, TestClass, void, method, int, int);
}
```

**Operators** can be registered as regular methods or functions, with the name being `operator` concatenated with the operator itself, without any spaces in between:

```cpp
struct TestStruct
{
   const TestStruct operator+(int pValue) const;
};

{
   CflatRegisterStruct(&env, TestStruct);
   CflatStructAddMethodReturnParams1(&env, TestStruct, const TestStruct, operator+, int);
}
```

For more complex standard types and global values, you can take advantage of the helpers included in `CflatHelper.h`:

```cpp
#include "Cflat/CflatHelper.h"

// ...

Cflat::Helper::registerStdString(&env);  // std::string
Cflat::Helper::registerStdOut(&env);     // std::cout

CflatRegisterSTLVector(&env, int);    // std::vector<int>
CflatRegisterSTLVector(&env, float);  // std::vector<float>

CflatRegisterSTLMap(&env, int, float);  // std::map<int, float>
```

In case you need to register **template structs or classes**, you can take the way the helper registers STL types as a reference.


### Loading scripts into the environment

It is possible to load scripts into the environment both passing the code as a string and passing the path of the file:

```cpp
env.load("test", "const char* str = \"Hello world!\";");
env.load("./scripts/test.cpp");
```


### Accessing script values and executing script functions

```cpp
//
//  Cflat script
//
namespace CfTest
{
   static const float kTestConst = 42.0f;

   static void voidFunc(int pA, int pB)
   {
      // ...
   }
   static int returnFunc(int pA, int pB)
   {
      // ...
   }
}
```

```cpp
//
//  cpp file
//
Cflat::Value* testConstValue = env.getVariable("CfTest::kTestConst");
const float testConst = CflatValueAs(testConstValue, float);

const int a = 42;
const int b = 10;

Cflat::Function* voidFunc = env.getFunction("CfTest::voidFunc");
env.voidFunctionCall(voidFunc, &a, &b);

Cflat::Function* returnFunc = env.getFunction("CfTest::returnFunc");
const int returnValue = env.returnFunctionCall<int>(returnFunc, &a, &b);
```


### Switching between interpreter and compiler

Cflat comes with the `CflatGlobal.h` header file, which provides a convenient way to write code that takes advantage of the Cflat scripting system in development configurations and gets the scripts compiled into machine code in final configurations. The first thing to do is to open the `CflatGlobalConfig.h` header and set both the path where the Cflat includes are, and the path where the scripts are:

```cpp
#if defined CFLAT_ENABLED

// Relative directory where the Cflat headers are located
# define CflatHeadersPath  ./Cflat

#else

// Relative directory where the scripts are located
# define CflatScriptsPath  ./scripts

#endif
```

Note that the base directory used as the reference for those relative paths must be defined in the list of additional include directories for the compiler. Otherwise, the `#include CflatScript(...)` directives (see below) will not compile, since the macro is resolved using brackets (<...>) instead of quotes ("...").

Making use of `CflatGlobal.h` requires that you implement the following functions (it can be in any cpp of the project):

```cpp
#if defined CFLAT_ENABLED
namespace CflatGlobal
{
   Cflat::Environment gEnv;
   std::mutex gMutex;

   Cflat::Environment* getEnvironment()
   {
      return &gEnv;
   }
   void lockEnvironment()
   {
      gMutex.lock();
   }
   void unlockEnvironment()
   {
      gMutex.unlock();
   }
   void onError(const char* pErrorMessage)
   {
      std::cerr << "[Cflat] " << pErrorMessage << std::endl;
   }
}
#endif
```

Then you have to make sure that the source files that need to access scripts include `CflatGlobal.h`, whether directly in the source file itself, or through some kind of common header which all source files of the project include:

```cpp
#include "Cflat/CflatGlobal.h"
```

In order to define in what configurations the scripts should be interpreted through the Cflat environment, you need to make sure that `CFLAT_ENABLED` is one of the preprocessor definitions in those configurations. In all the configurations where `CFLAT_ENABLED` is not defined, the scripts will be compiled into machine code.
The next step is to include the script or scripts you want to access from the source file:

```cpp
#include CflatScript(test.cpp)
```

Once that's done, you can already retrieve values and call functions from the included script.

```cpp
//
//  Cflat script
//
namespace CfTest
{
   static const float kTestConst = 42.0f;

   static int add(int pA, int pB)
   {
      return pA + pB;
   }
}
```

```cpp
//
//  cpp file
//
#include CflatScript(test.cpp)

// ...

const float testConst = CflatGet(float, CfTest::kTestConst);
std::cout << “kTestConst: “ << testConst << std::endl;

const int a = 42;
const int b = 10;
int addTest;
CflatReturnCall(addTest, int, CflatArg(a), CflatArg(b));

std::cout << “addTest: “ << addTest << std::endl;
```

Regarding function calls, note that there are two different macros defined in `CflatGlobal.h`, depending on whether the function to call returns something or not (`CflatReturnCall` and `CflatVoidCall`, respectively), and that you have to use the `CflatArg` macro for each argument.


### Using a custom allocator

You can define custom functions both for allocating and for releasing dynamic memory as follows:

```cpp
Cflat::Memory::malloc = [](size_t p_Size) -> void*
{
   return myCustomAllocatorMalloc(p_Size);
};
Cflat::Memory::free = [](void* p_Ptr)
{
   myCustomAllocatorFree(p_Ptr);
};
```

NOTE - if you use a custom allocator, remember to release the identifier names registry before shutting down the application (this is not required otherwise):

```cpp
Cflat::Identifier::releaseNamesRegistry();
```


### Execution hook

There is the possibility of registering an execution hook, for example to implement script debugging features in your application:

```cpp
void executionHook(Cflat::Environment* pEnv, const Cflat::CallStack& pCallStack)
{
   // ...
}

env.setExecutionHook(executionHook);
```

The function is then called right before each statement is executed. The `evaluateExpression` method, provided by the environment, allows you to inspect and modify values.


## Support the project

I work on this project in my spare time. If you would like to support it, you can [buy me a coffee!](https://ko-fi.com/arturocepeda)


## License

Cflat is distributed with a *zlib* license, and is free to use for both non-commercial and commercial projects:

```
Copyright (c) 2019-2022 Arturo Cepeda Pérez

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
```


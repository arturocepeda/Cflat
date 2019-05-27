
#include "gtest/gtest.h"

#include "../Cflat.h"

TEST(Namespaces, DirectChild)
{
   Cflat::Environment env;

   Cflat::Namespace* testNS = env.getNamespace("Test");
   EXPECT_FALSE(testNS);

   testNS = env.requestNamespace("Test");
   EXPECT_TRUE(testNS);

   EXPECT_EQ(testNS->getParent(), env.getGlobalNamespace());
   EXPECT_EQ(strcmp(testNS->getName().mName.c_str(), "Test"), 0);
   EXPECT_EQ(strcmp(testNS->getFullName().mName.c_str(), "Test"), 0);
}

TEST(Namespaces, Tree)
{
   Cflat::Environment env;

   Cflat::Namespace* test3NS = env.requestNamespace("Test1::Test2::Test3");
   EXPECT_TRUE(test3NS);

   Cflat::Namespace* test2NSAsParent = test3NS->getParent();
   EXPECT_TRUE(test2NSAsParent);
   Cflat::Namespace* test2NSFromRoot = env.getNamespace("Test1::Test2");
   EXPECT_TRUE(test2NSFromRoot);
   EXPECT_EQ(test2NSAsParent, test2NSFromRoot);

   Cflat::Namespace* test3NSAsChild = test2NSFromRoot->getNamespace("Test3");
   EXPECT_TRUE(test3NSAsChild);
   EXPECT_EQ(test3NS, test3NSAsChild);

   Cflat::Namespace* test1NSAsParent = test2NSFromRoot->getParent();
   EXPECT_TRUE(test1NSAsParent);
   Cflat::Namespace* test1NSFromRoot = env.getNamespace("Test1");
   EXPECT_TRUE(test1NSFromRoot);
   EXPECT_EQ(test1NSAsParent, test1NSFromRoot);
   EXPECT_EQ(test1NSFromRoot->getParent(), env.getGlobalNamespace());

   Cflat::Namespace* test3NSAsGrandChild = test1NSFromRoot->getNamespace("Test2::Test3");
   EXPECT_TRUE(test3NSAsGrandChild);
   EXPECT_EQ(test3NSAsGrandChild, test3NS);
}

TEST(Namespaces, RequestDoesNotRecreate)
{
   Cflat::Environment env;

   Cflat::Namespace* test3NS = env.getNamespace("Test1::Test2::Test3");
   EXPECT_FALSE(test3NS);

   test3NS = env.requestNamespace("Test1::Test2::Test3");
   EXPECT_TRUE(test3NS);

   Cflat::Namespace* test3NSRetrieved = env.requestNamespace("Test1::Test2::Test3");
   EXPECT_TRUE(test3NSRetrieved);
   EXPECT_EQ(test3NSRetrieved, test3NS);
}

TEST(Cflat, VariableDeclaration)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, VariableDeclarationInNamespace)
{
   Cflat::Environment env;

   const char* code =
      "namespace Test\n"
      "{\n"
      "  int var = 42;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Value* variable = env.getVariable("var");
   EXPECT_FALSE(variable);

   Cflat::Namespace* ns = env.getNamespace("Test");
   EXPECT_TRUE(ns);

   variable = ns->getVariable("var");
   EXPECT_TRUE(variable);
   EXPECT_EQ(CflatValueAs(variable, int), 42);
}

TEST(Cflat, VariableAssignment)
{
   Cflat::Environment env;

   const char* code =
      "int var;\n"
      "var = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, VariableIncrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "var++;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 43);
}

TEST(Cflat, VariableDecrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "var--;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 41);
}

TEST(Cflat, VariableIncrementPrePost)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42;\n"
      "int var2 = 42;\n"
      "int incVar1 = var1++;\n"
      "int incVar2 = ++var2;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 43);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 43);
   EXPECT_EQ(CflatValueAs(env.getVariable("incVar1"), int), 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("incVar2"), int), 43);
}

TEST(Cflat, Enum)
{
   Cflat::Environment env;

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

   const char* code =
      "const int var1 = kFirstValue;\n"
      "const int var2 = kSecondValue;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), kFirstValue);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), kSecondValue);
}

TEST(Cflat, EnumClass)
{
   Cflat::Environment env;

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

   const char* code =
      "const TestEnum var1 = TestEnum::kFirstValue;\n"
      "const TestEnum var2 = TestEnum::kSecondValue;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), TestEnum), TestEnum::kFirstValue);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), TestEnum), TestEnum::kSecondValue);
}

TEST(Cflat, ComparisonOperators)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "bool op1 = var == 42;\n"
      "bool op2 = var != 42;\n"
      "bool op3 = var > 42;\n"
      "bool op4 = var < 42;\n"
      "bool op5 = var >= 42;\n"
      "bool op6 = var <= 42;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(CflatValueAs(env.getVariable("op1"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("op2"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("op3"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("op4"), bool));
   EXPECT_TRUE(CflatValueAs(env.getVariable("op5"), bool));
   EXPECT_TRUE(CflatValueAs(env.getVariable("op6"), bool));
}

TEST(Cflat, LogicalOperators)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "bool op1 = (var == 42) && (var > 0);\n"
      "bool op2 = (var == 42) && (var < 0);\n"
      "bool op3 = (var == 42) || (var < 0);\n"
      "bool op4 = (var == 0) || (var < 0);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(CflatValueAs(env.getVariable("op1"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("op2"), bool));
   EXPECT_TRUE(CflatValueAs(env.getVariable("op3"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("op4"), bool));
}

TEST(Cflat, ArithmeticOperators)
{
   Cflat::Environment env;

   const char* code =
      "int iop1 = 10 + 5;\n"
      "int iop2 = 10 - 5;\n"
      "int iop3 = 10 * 5;\n"
      "int iop4 = 10 / 5;\n"
      "float fop1 = 10.0f + 5.0f;\n"
      "float fop2 = 10.0f - 5.0f;\n"
      "float fop3 = 10.0f * 5.0f;\n"
      "float fop4 = 10.0f / 5.0f;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("iop1"), int), 15);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop2"), int), 5);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop3"), int), 50);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop4"), int), 2);

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop1"), float), 15.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop2"), float), 5.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop3"), float), 50.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop4"), float), 2.0f);
}

TEST(Cflat, ConditionalExpression)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42;\n"
      "int var2 = var1 == 42 ? 1 : 0;\n"
      "int var3 = var1 != 42 ? 1 : 0;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("var3"), int), 0);
}

TEST(Cflat, IfStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "\n"
      "if(var == 42)\n"
      "{\n"
      "  var++;\n"
      "}\n"
      "else\n"
      "{\n"
      "  var--;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 43);
}

TEST(Cflat, WhileStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "\n"
      "while(var < 100)\n"
      "{\n"
      "  var++;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 100);
}

TEST(Cflat, ForStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "\n"
      "for(int i = 0; i < 10; i++)\n"
      "{\n"
      "  var++;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 52);
}

TEST(Cflat, StdStringUsageV1)
{
   Cflat::Environment env;

   {
      CflatRegisterClass(&env, std::string);
      CflatClassAddConstructor(&env, std::string);
      CflatClassAddMethodReturnParams1(&env, std::string, std::string,&, assign, const char*,);
   }

   const char* code =
      "std::string str;\n"
      "str.assign(\"Hello world!\");\n";

   EXPECT_TRUE(env.load("test", code));

   std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, StdStringUsageV2)
{
   Cflat::Environment env;
   Cflat::Namespace* ns = env.requestNamespace("std");

   {
      using namespace std;

      {
         CflatRegisterClass(ns, string);
         CflatClassAddConstructor(ns, string);
         CflatClassAddMethodReturnParams1(ns, string, string,&, assign, const char*,);
      }
   }

   const char* code =
      "std::string str;\n"
      "str.assign(\"Hello world!\");\n";

   EXPECT_TRUE(env.load("test", code));

   std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, UsingNamespace)
{
   Cflat::Environment env;

   {
      CflatRegisterClass(&env, std::string);
      CflatClassAddConstructor(&env, std::string);
      CflatClassAddMethodReturnParams1(&env, std::string, std::string,&, assign, const char*,);
   }

   const char* code =
      "using namespace std;\n"
      "string str;\n"
      "str.assign(\"Hello world!\");\n";

   EXPECT_TRUE(env.load("test", code));

   std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, MemberAssignment)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var1;
      int var2;
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var1);
      CflatStructAddMember(&env, TestStruct, int, var2);
      CflatStructAddConstructor(&env, TestStruct);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.var1 = 42;\n"
      "testStruct.var2 = 100;\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var1, 42);
   EXPECT_EQ(testStruct.var2, 100);
}

TEST(Cflat, MemberAssignmentPointer)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var1;
      int var2;
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var1);
      CflatStructAddMember(&env, TestStruct, int, var2);
      CflatStructAddConstructor(&env, TestStruct);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "TestStruct* testStructPtr = &testStruct;\n"
      "testStructPtr->var1 = 42;\n"
      "testStructPtr->var2 = 100;\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var1, 42);
   EXPECT_EQ(testStruct.var2, 100);
}

TEST(Cflat, VoidMethodCallNoParams)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;

      TestStruct() : var(0) {}
      void method() { var = 42; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddConstructor(&env, TestStruct);
      CflatStructAddMethodVoid(&env, TestStruct, void,, method);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.method();\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, VoidMethodCallWithParam)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;

      TestStruct() : var(0) {}
      void method(int val) { var = val; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddConstructor(&env, TestStruct);
      CflatStructAddMethodVoidParams1(&env, TestStruct, void,, method, int,);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.method(42);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, VoidMethodCallWithParamAndPointerOperator)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;

      TestStruct() : var(0) {}
      void method(int val) { var = val; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddConstructor(&env, TestStruct);
      CflatStructAddMethodVoidParams1(&env, TestStruct, void,, method, int,);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "TestStruct* testStructPtr = &testStruct;\n"
      "testStructPtr->method(42);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, StaticMethodCall)
{
   Cflat::Environment env;

   static int staticVar = 0;

   struct TestStruct
   {
      static void incrementStaticVar() { staticVar++; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddStaticMethodVoid(&env, TestStruct, void,, incrementStaticVar);
   }

   const char* code =
      "TestStruct::incrementStaticVar();\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(staticVar, 1);
}

TEST(Cflat, FunctionDeclarationNoParams)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "\n"
      "void func()\n"
      "{\n"
      "  var = 42;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   int& var = CflatValueAs(env.getVariable("var"), int);

   CflatSTLVector<Cflat::Value> args;

   Cflat::Function* func = env.getFunction("func");
   EXPECT_TRUE(func);
   func->execute(args, nullptr);

   EXPECT_EQ(var, 42);
}

TEST(Cflat, FunctionDeclarationWithParam)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "\n"
      "void func(int param)\n"
      "{\n"
      "  var = param;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   int& var = CflatValueAs(env.getVariable("var"), int);

   int argValue = 42;
   Cflat::TypeUsage argTypeUsage = env.getTypeUsage("int");
   Cflat::Value arg;
   arg.initOnHeap(argTypeUsage);
   arg.set(&argValue);

   CflatSTLVector<Cflat::Value> args;
   args.push_back(arg);

   Cflat::Function* func = env.getFunction("func");
   EXPECT_TRUE(func);
   func->execute(args, nullptr);

   EXPECT_EQ(var, 42);
}

TEST(Cflat, FunctionDeclarationWithReturnValue)
{
   Cflat::Environment env;

   const char* code =
      "int func()\n"
      "{\n"
      "  return 42;\n"
      "}\n"
      "\n"
      "int var = func();\n";

   EXPECT_TRUE(env.load("test", code));

   int& var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, FunctionDeclarationWithPointerParameterV1)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;
      TestStruct() : var(0) {}
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var);
      CflatStructAddConstructor(&env, TestStruct);
   }

   const char* code =
      "void func(TestStruct* pTestStruct)\n"
      "{\n"
         "pTestStruct->var = 42;\n"
      "}\n"
      "\n"
      "TestStruct testStruct;\n"
      "func(&testStruct);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, FunctionDeclarationWithPointerParameterV2)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;
      TestStruct() : var(0) {}
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var);
      CflatStructAddConstructor(&env, TestStruct);
   }

   const char* code =
      "void func(TestStruct* pTestStruct)\n"
      "{\n"
         "pTestStruct->var = 42;\n"
      "}\n"
      "\n"
      "TestStruct testStruct;\n"
      "TestStruct* testStructPtr = &testStruct;\n"
      "func(testStructPtr);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, FunctionDeclarationWithReferenceParameter)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var;
      TestStruct() : var(0) {}
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var);
      CflatStructAddConstructor(&env, TestStruct);
   }

   const char* code =
      "void func(TestStruct& pTestStruct)\n"
      "{\n"
         "pTestStruct.var = 42;\n"
      "}\n"
      "\n"
      "TestStruct testStruct;\n"
      "func(testStruct);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, OperatorOverload)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var1;
      int var2;

      const TestStruct operator+(int pValue) const
      {
         TestStruct other = *this;
         other.var1 = var1 + pValue;
         other.var2 = var2 + pValue;
         return other;
      }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var1);
      CflatStructAddMember(&env, TestStruct, int, var2);
      CflatStructAddConstructor(&env, TestStruct);
      CflatStructAddMethodReturnParams1(&env, TestStruct, const TestStruct,, operator+, int,);
   }

   const char* code =
      "TestStruct testStruct1;\n"
      "testStruct1.var1 = 42;\n"
      "testStruct1.var2 = 100;\n"
      "TestStruct testStruct2 = testStruct1 + 10;\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct2 = CflatValueAs(env.getVariable("testStruct2"), TestStruct);
   EXPECT_EQ(testStruct2.var1, 52);
   EXPECT_EQ(testStruct2.var2, 110);
}

TEST(Cflat, RegisteringDerivedClass)
{
   Cflat::Environment env;

   {
      CflatRegisterClass(&env, std::string);
      CflatClassAddConstructor(&env, std::string);
      CflatClassAddMethodReturnParams1(&env, std::string, std::string,&, assign, const char*,);
   }

   class TestClass : public std::string
   {
   public:
      int mInternalValue;

      TestClass() : mInternalValue(0) {}
      void setInternalValue(int pValue) { mInternalValue = pValue; }
   };

   {
      CflatRegisterClass(&env, TestClass);
      CflatClassAddBaseType(&env, TestClass, std::string);
      CflatClassAddConstructor(&env, TestClass);
      CflatClassAddMethodVoidParams1(&env, TestClass, void,, setInternalValue, int,);
   }

   const char* code =
      "TestClass testClass;\n"
      "testClass.assign(\"Hello world!\");\n"
      "testClass.setInternalValue(42);\n";

   EXPECT_TRUE(env.load("test", code));

   TestClass& testClass = CflatValueAs(env.getVariable("testClass"), TestClass);
   EXPECT_EQ(strcmp(testClass.c_str(), "Hello world!"), 0);
   EXPECT_EQ(testClass.mInternalValue, 42);
}

TEST(RuntimeErrors, NullPointerAccess)
{
   Cflat::Environment env;

   {
      CflatRegisterClass(&env, std::string);
      CflatClassAddConstructor(&env, std::string);
      CflatClassAddMethodReturnParams1(&env, std::string, std::string,&, assign, const char*,);
   }

   const char* code =
      "std::string* strPtr = nullptr;\n"
      "strPtr->assign(\"Hello world!\");\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(), "[Runtime Error] Line 2: null pointer access ('strPtr')"), 0);
}

TEST(RuntimeErrors, DivisionByZero)
{
   Cflat::Environment env;

   const char* code =
      "int val = 10 / 0;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(), "[Runtime Error] Line 1: division by zero"), 0);
}


#include "gtest/gtest.h"

#include "../Cflat.h"

TEST(Cflat, VariableDeclaration)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatRetrieveValue(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, VariableAssignment)
{
   Cflat::Environment env;

   const char* code =
      "int var;"
      "var = 42;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatRetrieveValue(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, VariableIncrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;"
      "var++;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatRetrieveValue(env.getVariable("var"), int);
   EXPECT_EQ(var, 43);
}

TEST(Cflat, VariableDecrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;"
      "var--;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatRetrieveValue(env.getVariable("var"), int);
   EXPECT_EQ(var, 41);
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

   EXPECT_TRUE(CflatRetrieveValue(env.getVariable("op1"), bool));
   EXPECT_FALSE(CflatRetrieveValue(env.getVariable("op2"), bool));
   EXPECT_FALSE(CflatRetrieveValue(env.getVariable("op3"), bool));
   EXPECT_FALSE(CflatRetrieveValue(env.getVariable("op4"), bool));
   EXPECT_TRUE(CflatRetrieveValue(env.getVariable("op5"), bool));
   EXPECT_TRUE(CflatRetrieveValue(env.getVariable("op6"), bool));
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

   EXPECT_TRUE(CflatRetrieveValue(env.getVariable("op1"), bool));
   EXPECT_FALSE(CflatRetrieveValue(env.getVariable("op2"), bool));
   EXPECT_TRUE(CflatRetrieveValue(env.getVariable("op3"), bool));
   EXPECT_FALSE(CflatRetrieveValue(env.getVariable("op4"), bool));
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

   EXPECT_EQ(CflatRetrieveValue(env.getVariable("iop1"), int), 15);
   EXPECT_EQ(CflatRetrieveValue(env.getVariable("iop2"), int), 5);
   EXPECT_EQ(CflatRetrieveValue(env.getVariable("iop3"), int), 50);
   EXPECT_EQ(CflatRetrieveValue(env.getVariable("iop4"), int), 2);

   EXPECT_FLOAT_EQ(CflatRetrieveValue(env.getVariable("fop1"), float), 15.0f);
   EXPECT_FLOAT_EQ(CflatRetrieveValue(env.getVariable("fop2"), float), 5.0f);
   EXPECT_FLOAT_EQ(CflatRetrieveValue(env.getVariable("fop3"), float), 50.0f);
   EXPECT_FLOAT_EQ(CflatRetrieveValue(env.getVariable("fop4"), float), 2.0f);
}

TEST(Cflat, StdStringUsage)
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

   std::string& str = CflatRetrieveValue(env.getVariable("str"), std::string);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   int& var = CflatRetrieveValue(env.getVariable("var"), int);

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

   int& var = CflatRetrieveValue(env.getVariable("var"), int);

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

   int& var = CflatRetrieveValue(env.getVariable("var"), int);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct);
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

   TestStruct& testStruct2 = CflatRetrieveValue(env.getVariable("testStruct2"), TestStruct);
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

   TestClass& testClass = CflatRetrieveValue(env.getVariable("testClass"), TestClass);
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

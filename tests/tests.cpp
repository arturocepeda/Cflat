
#include "gtest/gtest.h"

#include "../Cflat.h"

TEST(Cflat, VariableDeclaration)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;";

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   int var = CflatRetrieveValue(env.getVariable("var"), int,,);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, VariableAssignment)
{
   Cflat::Environment env;

   const char* code =
      "int var;"
      "var = 42;";

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   int var = CflatRetrieveValue(env.getVariable("var"), int,,);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, StdStringUsage)
{
   Cflat::Environment env;

   {
      CflatRegisterClass(&env, std::string);
      CflatClassAddConstructor(&env, std::string);
      CflatClassAddMethodReturnParams1(&env, std::string, std::string,&,*, assign, const char*,,);
   }

   const char* code =
      "std::string str;\n"
      "str.assign(\"Hello world!\");\n";

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   std::string& str = CflatRetrieveValue(env.getVariable("str"), std::string,,);
   EXPECT_TRUE(strcmp(str.c_str(), "Hello world!") == 0);
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

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct,,);
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

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct,,);
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
      CflatStructAddMethodVoid(&env, TestStruct, void,,, method);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.method();\n";

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct,,);
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
      CflatStructAddMethodVoidParams1(&env, TestStruct, void,,, method, int,,);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.method(42);\n";

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   TestStruct& testStruct = CflatRetrieveValue(env.getVariable("testStruct"), TestStruct,,);
   EXPECT_EQ(testStruct.var, 42);
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

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   int& var = CflatRetrieveValue(env.getVariable("var"), int,,);

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

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   int& var = CflatRetrieveValue(env.getVariable("var"), int,,);

   int argValue = 42;
   Cflat::TypeUsage argTypeUsage = env.getTypeUsage("int");
   Cflat::Value arg(argTypeUsage, &argValue);

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

   Cflat::Program program;
   EXPECT_TRUE(env.load(code, program));
   EXPECT_TRUE(env.execute(program));

   int& var = CflatRetrieveValue(env.getVariable("var"), int,,);
   EXPECT_EQ(var, 42);
}

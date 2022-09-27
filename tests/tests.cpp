
#include "gtest/gtest.h"

#include "../CflatHelper.h"

TEST(Namespaces, DirectChild)
{
   Cflat::Environment env;

   Cflat::Namespace* testNS = env.getNamespace("Test");
   EXPECT_FALSE(testNS);

   testNS = env.requestNamespace("Test");
   EXPECT_TRUE(testNS);

   EXPECT_EQ(testNS->getParent(), env.getGlobalNamespace());
   EXPECT_EQ(strcmp(testNS->getIdentifier().mName, "Test"), 0);
   EXPECT_EQ(strcmp(testNS->getFullIdentifier().mName, "Test"), 0);
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

TEST(Preprocessor, MacroReplacement)
{
   Cflat::Environment env;
   env.defineMacro("INT_TYPE", "int");

   const char* code =
      "INT_TYPE var = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Preprocessor, MacroReplacementWithArgument)
{
   Cflat::Environment env;
   env.defineMacro("INT_TYPE(name)", "int name");

   const char* code =
      "INT_TYPE(var) = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Preprocessor, DefinedMacroEmpty)
{
   Cflat::Environment env;

   const char* code =
      "#define EMPTY\n"
      "EMPTY int var = EMPTY 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Preprocessor, DefinedMacroReplacement)
{
   Cflat::Environment env;

   const char* code =
      "#define INT_TYPE  int\n"
      "INT_TYPE var = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Preprocessor, DefinedMacroReplacementWithArgument)
{
   Cflat::Environment env;

   const char* code =
      "#define INT_TYPE(name)  int name\n"
      "INT_TYPE(var) = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
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

TEST(Cflat, VariableDeclarationWithAuto)
{
   Cflat::Environment env;

   const char* code =
      "auto var1 = 42;\n"
      "auto var2 = 42.0f;\n"
      "auto var3 = 42.0;\n"
      "auto var4 = true;\n";

   EXPECT_TRUE(env.load("test", code));

   const char* var1TypeName = 
      env.getVariable("var1")->mTypeUsage.mType->mIdentifier.mName;
   EXPECT_EQ(strcmp(var1TypeName, "int"), 0);

   const char* var2TypeName = 
      env.getVariable("var2")->mTypeUsage.mType->mIdentifier.mName;
   EXPECT_EQ(strcmp(var2TypeName, "float"), 0);

   const char* var3TypeName = 
      env.getVariable("var3")->mTypeUsage.mType->mIdentifier.mName;
   EXPECT_EQ(strcmp(var3TypeName, "double"), 0);

   const char* var4TypeName = 
      env.getVariable("var4")->mTypeUsage.mType->mIdentifier.mName;
   EXPECT_EQ(strcmp(var4TypeName, "bool"), 0);
}

TEST(Cflat, Reference)
{
   Cflat::Environment env;

   const char* code =
      "float var = 42.0f;\n"
      "float& ref = var;\n"
      "ref += 100.0f;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var"), float), 142.0f);
}

TEST(Cflat, FloatingPointFormat)
{
   Cflat::Environment env;

   const char* code =
      "const float var1a = 0.5f;\n"
      "const float var1b = .5f;\n"
      "const float var2a = 1.0f;\n"
      "const float var2b = 1.f;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var1a"), float), 0.5f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var1b"), float), 0.5f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2a"), float), 1.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2b"), float), 1.0f);
}

TEST(Cflat, HexadecimalValues)
{
  Cflat::Environment env;

  const char* code =
    "const uint32_t value1 = 0x123456;\n"
    "const uint32_t value2 = 0xabcdef;\n"
    "const uint32_t value3 = 0x1a2b3c;\n";

  EXPECT_TRUE(env.load("test", code));

  EXPECT_EQ(CflatValueAs(env.getVariable("value1"), uint32_t), 0x123456);
  EXPECT_EQ(CflatValueAs(env.getVariable("value2"), uint32_t), 0xabcdef);
  EXPECT_EQ(CflatValueAs(env.getVariable("value3"), uint32_t), 0x1a2b3c);
}

TEST(Cflat, MultilineStringLiteral)
{
  Cflat::Environment env;

  const char* code =
    "const char* str = \"Hello\"\n"
    "  \" \"\n"
    "  \"world\"\n"
    "  \"!\";\n";

  EXPECT_TRUE(env.load("test", code));

  const char* str = CflatValueAs(env.getVariable("str"), const char*);
  EXPECT_EQ(strcmp(str, "Hello world!"), 0);
}

TEST(Cflat, ObjectDeclaration)
{
   Cflat::Environment env;

   class TestClass
   {
   private:
      int mValue;
   public:
      TestClass(int pValue) : mValue(pValue) {}
      int getValue() const { return mValue; }
   };

   {
      CflatRegisterClass(&env, TestClass);
      CflatClassAddConstructorParams1(&env, TestClass, int);
      CflatClassAddMethodReturn(&env, TestClass, int, getValue);
   }

   const char* code =
      "TestClass testClass(42);\n"
      "int value = testClass.getValue();\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("value"), int), 42);
}

TEST(Cflat, ObjectDeclarationWithAssignment)
{
   Cflat::Environment env;

   class TestClass
   {
   private:
      int mValue;
   public:
      TestClass(int pValue) : mValue(pValue) {}
      int getValue() const { return mValue; }
   };

   {
      CflatRegisterClass(&env, TestClass);
      CflatClassAddConstructorParams1(&env, TestClass, int);
      CflatClassAddMethodReturn(&env, TestClass, int, getValue);
   }

   const char* code =
      "TestClass testClass = TestClass(42);\n"
      "int value = testClass.getValue();\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("value"), int), 42);
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

TEST(Cflat, VariableAssignmentsWithArithmeticOperations)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42;\n"
      "int var2 = 42;\n"
      "int var3 = 42;\n"
      "int var4 = 42;\n"
      "\n"
      "var1 += 2;\n"
      "var2 -= 2;\n"
      "var3 *= 2;\n"
      "var4 /= 2;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 44);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 40);
   EXPECT_EQ(CflatValueAs(env.getVariable("var3"), int), 84);
   EXPECT_EQ(CflatValueAs(env.getVariable("var4"), int), 21);
}

TEST(Cflat, VariableInitializationWithImplicitCast)
{
   Cflat::Environment env;

   const char* code =
      "size_t var1 = 42;\n"
      "float var2 = 42;\n"
      "float var3 = 42.0;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), size_t), 42u);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2"), float), 42.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var3"), float), 42.0f);
}

TEST(Cflat, StaticConstInitializationWithImplicitCast)
{
   Cflat::Environment env;

   const char* code =
      "static const size_t var1 = 42;\n"
      "static const float var2 = 42;\n"
      "static const float var3 = 42.0;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), size_t), 42u);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2"), float), 42.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var3"), float), 42.0f);
}

TEST(Cflat, VariableAssignmentWithImplicitCast)
{
   Cflat::Environment env;

   const char* code =
      "size_t var1 = 0u;\n"
      "float var2 = 0.0f;\n"
      "float var3 = 0.0f;\n"
      "var1 = 42;\n"
      "var2 = 42;\n"
      "var3 = 42.0;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), size_t), 42u);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2"), float), 42.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var3"), float), 42.0f);
}

TEST(Cflat, AssignmentsInvolvingMembers)
{
   struct TestStruct
   {
      float member;
   };

   Cflat::Environment env;

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, float, member);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "testStruct.member = 42.0f;\n"
      "float var = 0.0f;\n"
      "var = testStruct.member;\n";

   EXPECT_TRUE(env.load("test", code));

   float var = CflatValueAs(env.getVariable("var"), float);
   EXPECT_FLOAT_EQ(var, 42.0f);
}

TEST(Cflat, ArrayDeclaration)
{
   Cflat::Environment env;

   const char* code =
      "int array[3] = { 0, 1, 2 };\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 3u);

   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[0], 0);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[1], 1);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[2], 2);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 0);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 1);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 2, int), 2);
}


TEST(Cflat, ArrayDeclarationWithStaticConstSize)
{
   Cflat::Environment env;

   const char* code =
      "static const int kArraySize = 3;\n"
      "int array[kArraySize] = { 0, 1, 2 };\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 3u);

   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[0], 0);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[1], 1);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[2], 2);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 0);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 1);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 2, int), 2);
}

TEST(Cflat, ArrayDeclarationWithoutSpecifyingSize)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 0, 1, 2 };\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 3u);

   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[0], 0);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[1], 1);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[2], 2);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 0);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 1);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 2, int), 2);
}

TEST(Cflat, ArrayElementAssignment)
{
   Cflat::Environment env;

   const char* code =
      "int array[3];\n"
      "void func()\n"
      "{\n"
      "  array[0] = 0;\n"
      "  array[1] = 1;\n"
      "  array[2] = 2;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 3u);

   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[0], 0);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[1], 1);
   EXPECT_EQ(CflatValueAsArray(arrayValue, int)[2], 2);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 0);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 1);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 2, int), 2);
}

TEST(Cflat, ArrayElementAccess)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 42, 420, 4200 };\n"
      "int var1 = array[0];\n"
      "int var2 = array[1];\n"
      "int var3 = array[2];\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 420);
   EXPECT_EQ(CflatValueAs(env.getVariable("var3"), int), 4200);
}

TEST(Cflat, ArrayElementAccessThroughPointer)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 42, 420, 4200 };\n"
      "int* ptr = &array[0];\n"
      "int var1 = ptr[0];\n"
      "int var2 = ptr[1];\n"
      "int var3 = ptr[2];\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 420);
   EXPECT_EQ(CflatValueAs(env.getVariable("var3"), int), 4200);
}

TEST(Cflat, VariableIncrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  var++;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 43);
}

TEST(Cflat, VariableDecrement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  var--;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

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

TEST(Cflat, UnaryOperatorMinus)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "int opVar = -var;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("opVar"), int), -42);
}

TEST(Cflat, UnaryOperatorMinusInBinaryExpression)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42 * -1;\n"
      "float var2 = 42.0f * -1.0f;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), -42);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var2"), float), -42.0f);
}

TEST(Cflat, PointerArithmetic)
{
   Cflat::Environment env;

   const char* code =
      "int var;\n"
      "int* ptr = &var;\n"
      "int* incPtr1 = ptr;\n"
      "int* incPtr2 = ptr;\n"
      "int* incPtr3 = ptr;\n"
      "int* incPtr4 = ptr;\n"
      "void func()\n"
      "{\n"
      "  incPtr1++;\n"
      "  incPtr2--;\n"
      "  incPtr3 += 42;\n"
      "  incPtr4 -= 42;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int* ptr = CflatValueAs(env.getVariable("ptr"), int*);

   EXPECT_EQ(CflatValueAs(env.getVariable("incPtr1"), int*), ptr + 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("incPtr2"), int*), ptr - 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("incPtr3"), int*), ptr + 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("incPtr4"), int*), ptr - 42);
}

TEST(Cflat, PointerArithmeticWithArrayV1)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 0, 1, 2, 3 };\n"
      "int* ptr = array;\n"
      "int value0 = *ptr++;\n"
      "int value1 = *ptr++;\n"
      "int value2 = *ptr++;\n"
      "int value3 = *ptr++;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("value0"), int), 0);
   EXPECT_EQ(CflatValueAs(env.getVariable("value1"), int), 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("value2"), int), 2);
   EXPECT_EQ(CflatValueAs(env.getVariable("value3"), int), 3);
}

TEST(Cflat, PointerArithmeticWithArrayV2)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 0, 1, 2, 3 };\n"
      "int* ptr = &array[0];\n"
      "int value0 = *ptr++;\n"
      "int value1 = *ptr++;\n"
      "int value2 = *ptr++;\n"
      "int value3 = *ptr++;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("value0"), int), 0);
   EXPECT_EQ(CflatValueAs(env.getVariable("value1"), int), 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("value2"), int), 2);
   EXPECT_EQ(CflatValueAs(env.getVariable("value3"), int), 3);
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

TEST(Cflat, LogicalOperatorsNegation)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "bool op1 = (var == 42) && (var > 0);\n"
      "bool op2 = (var == 42) && (var < 0);\n"
      "bool op3 = (var == 42) || (var < 0);\n"
      "bool op4 = (var == 0) || (var < 0);\n"
      "bool nop1 = !op1;\n"
      "bool nop2 = !op2;\n"
      "bool nop3 = !op3;\n"
      "bool nop4 = !op4;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("op1"), bool), !CflatValueAs(env.getVariable("nop1"), bool));
   EXPECT_EQ(CflatValueAs(env.getVariable("op2"), bool), !CflatValueAs(env.getVariable("nop2"), bool));
   EXPECT_EQ(CflatValueAs(env.getVariable("op3"), bool), !CflatValueAs(env.getVariable("nop3"), bool));
   EXPECT_EQ(CflatValueAs(env.getVariable("op4"), bool), !CflatValueAs(env.getVariable("nop4"), bool));
}

TEST(Cflat, DifferentiateBetweenLogicOperatorsAndTemplates)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "bool opb = var < 100 && var > 0;\n"
      "int opi = static_cast<int>(opb);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(CflatValueAs(env.getVariable("opb"), bool));
   EXPECT_EQ(CflatValueAs(env.getVariable("opi"), int), 1);
}

TEST(Cflat, ArithmeticOperators)
{
   Cflat::Environment env;

   const char* code =
      "int iop1 = 10 + 5;\n"
      "int iop2 = 10 - 5;\n"
      "int iop3 = 10 * 5;\n"
      "int iop4 = 10 / 5;\n"
      "int iop5 = 10 % 4;\n"
      "float fop1 = 10.0f + 5.0f;\n"
      "float fop2 = 10.0f - 5.0f;\n"
      "float fop3 = 10.0f * 5.0f;\n"
      "float fop4 = 10.0f / 5.0f;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("iop1"), int), 15);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop2"), int), 5);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop3"), int), 50);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop4"), int), 2);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop5"), int), 2);

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop1"), float), 15.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop2"), float), 5.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop3"), float), 50.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fop4"), float), 2.0f);
}

TEST(Cflat, BitwiseOperators)
{
   Cflat::Environment env;

   const char* code =
      "int iop1 = 0x01 & 0x02;\n"
      "int iop2 = 0x01 | 0x02;\n"
      "int iop3 = 0x01 ^ 0x01;\n"
      "int iop4 = ~0x01;\n"
      "int iop5 = 0x01 << 3;\n"
      "int iop6 = 0x08 >> 3;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("iop1"), int), 0x01 & 0x02);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop2"), int), 0x01 | 0x02);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop3"), int), 0x01 ^ 0x01);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop4"), int), ~0x01);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop5"), int), 0x01 << 3);
   EXPECT_EQ(CflatValueAs(env.getVariable("iop6"), int), 0x08 >> 3);
}

TEST(Cflat, BinaryOperatorPrecedence)
{
   Cflat::Environment env;

   const char* code =
      "const int var = 42;\n"
      "const bool condition1 = var > 0 && var < 50;\n"
      "const bool condition2 = var > 50 && var < 100;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(CflatValueAs(env.getVariable("condition1"), bool));
   EXPECT_FALSE(CflatValueAs(env.getVariable("condition2"), bool));
}

TEST(Cflat, SignedUnsignedArithmetic)
{
   Cflat::Environment env;

   const char* code =
      "int signedVar = 0;\n"
      "uint8_t unsignedVar = 255u;\n"
      "void func()\n"
      "{\n"
      "  signedVar += unsignedVar;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("signedVar"), int), 255);
}

TEST(Cflat, ShortCircuitEvaluation)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 0;\n"
      "int var2 = 0;\n"
      "const bool condition1 = var1++ && var2++;\n"
      "const bool condition2 = var1++ || var2++;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_FALSE(CflatValueAs(env.getVariable("condition1"), bool));
   EXPECT_TRUE(CflatValueAs(env.getVariable("condition2"), bool));
   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 2);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 0);
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

TEST(Cflat, ConditionalExpressionAsFunctionCallArgument)
{
   Cflat::Environment env;

   const char* code =
      "int func(int arg) { return arg * 2; }\n"
      "int var = 42;\n"
      "int result = func(var == 42 ? 1 : 0);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("result"), int), 2);
}

TEST(Cflat, ImplicitCastBetweenIntegerAndFloat)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42;\n"
      "float var2 = 10.0f;\n"
      "float var3 = var1 + var2;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var3"), float), 52.0f);
}

TEST(Cflat, IfStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  if(var == 42)\n"
      "  {\n"
      "    var++;\n"
      "  }\n"
      "  else\n"
      "  {\n"
      "    var--;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 43);
}

TEST(Cflat, SwitchStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  switch(var)\n"
      "  {\n"
      "  case 0:\n"
      "    var += 10;\n"
      "    break;\n"
      "  case 42:\n"
      "    var += 100;\n"
      "    break;\n"
      "  case 100:\n"
      "    var += 1000;\n"
      "    break;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 142);
}

TEST(Cflat, SwitchStatementNoBreak)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  switch(var)\n"
      "  {\n"
      "  case 0:\n"
      "    var += 10;\n"
      "  case 42:\n"
      "    var += 100;\n"
      "  case 100:\n"
      "    var += 1000;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 1142);
}

TEST(Cflat, SwitchStatementDefault)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  switch(var)\n"
      "  {\n"
      "  case 0:\n"
      "    var += 10;\n"
      "    break;\n"
      "  case 10:\n"
      "    var += 100;\n"
      "    break;\n"
      "  default:\n"
      "    var += 1000;\n"
      "    break;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 1042);
}

TEST(Cflat, WhileStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  while(var < 100)\n"
      "  {\n"
      "    var++;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 100);
}

TEST(Cflat, DoWhileStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  do\n"
      "  {\n"
      "    var++;\n"
      "  }\n"
      "  while(var < 100);\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 100);
}

TEST(Cflat, ForStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  for(int i = 0; i < 10; i++)\n"
      "  {\n"
      "    var++;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 52);
}

TEST(Cflat, ContinueStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  for(int i = 0; i < 10; i++)\n"
      "  {\n"
      "    if((i % 2) == 0)\n"
      "    {\n"
      "      continue;\n"
      "    }\n"
      "    var++;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 47);
}

TEST(Cflat, BreakStatement)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  for(int i = 0; i < 10; i++)\n"
      "  {\n"
      "    if(i == 5)\n"
      "    {\n"
      "      break;\n"
      "    }\n"
      "    var++;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 47);
}

TEST(Cflat, Scope)
{
   Cflat::Environment env;

   const char* code =
      "int outerVar = 42;\n"
      "void func()\n"
      "{\n"
      "  int innerVar = 42;\n"
      "  outerVar += innerVar;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   EXPECT_EQ(CflatValueAs(env.getVariable("outerVar"), int), 84);
   EXPECT_EQ(env.getVariable("innerVar"), nullptr);
}

TEST(Cflat, SameVariableNameInDifferentScope)
{
   Cflat::Environment env;

   const char* code =
      "int var = 42;\n"
      "void func()\n"
      "{\n"
      "  int var = 0;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var"), int), 42);
}

TEST(Cflat, StdStringUsage)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "std::string str;\n"
      "void func()\n"
      "{\n"
      "  str.assign(\"Hello world!\");\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, StdVectorUsage)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec;\n"
      "void func()\n"
      "{\n"
      "  vec.push_back(42);\n"
      "  vec.push_back(42);\n"
      "  vec[1] = 0;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::vector<int>& vec = CflatValueAs(env.getVariable("vec"), std::vector<int>);
   EXPECT_EQ(vec.size(), 2u);
   EXPECT_EQ(vec[0], 42);
   EXPECT_EQ(vec[1], 0);
}

TEST(Cflat, StdVectorIteration)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec1;\n"
      "std::vector<int> vec2;\n"
      "void func()\n"
      "{\n"
      "  vec1.push_back(0);\n"
      "  vec1.push_back(1);\n"
      "  vec1.push_back(2);\n"
      "  for(auto it = vec1.begin(); it != vec1.end(); it++)\n"
      "  {\n"
      "    vec2.push_back(*it + 42);\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::vector<int>& vec2 = CflatValueAs(env.getVariable("vec2"), std::vector<int>);
   EXPECT_EQ(vec2.size(), 3u);
   EXPECT_EQ(vec2[0], 42);
   EXPECT_EQ(vec2[1], 43);
   EXPECT_EQ(vec2[2], 44);
}

TEST(Cflat, StdVectorErase)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec;\n"
      "void func()\n"
      "{\n"
      "  vec.push_back(42);\n"
      "  vec.push_back(43);\n"
      "  vec.push_back(44);\n"
      "  vec.push_back(45);\n"
      "  vec.erase(vec.begin() + 2);\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::vector<int>& vec = CflatValueAs(env.getVariable("vec"), std::vector<int>);
   EXPECT_EQ(vec.size(), 3u);
   EXPECT_EQ(vec[0], 42);
   EXPECT_EQ(vec[1], 43);
   EXPECT_EQ(vec[2], 45);
}

TEST(Cflat, StdMapUsage)
{
   Cflat::Environment env;

   CflatRegisterSTLMap(&env, int, float);

   const char* code =
      "std::map<int, float> map;\n"
      "void func()\n"
      "{\n"
      "  map[42] = 100.0f;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   typedef std::map<int, float> MapType;
   MapType& map = CflatValueAs(env.getVariable("map"), MapType);
   EXPECT_EQ(map.size(), 1u);
   EXPECT_FLOAT_EQ(map[42], 100.0f);
}

TEST(Cflat, StdMapLookUp)
{
   Cflat::Environment env;

   CflatRegisterSTLMap(&env, int, float);

   const char* code =
      "std::map<int, float> map;\n"
      "float value = 0.0f;"
      "void func()\n"
      "{\n"
      "  map[42] = 100.0f;\n"
      "  auto it = map.find(42);\n"
      "  if(it != map.end())\n"
      "  {\n"
      "    value = (*it).second;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   const float value = CflatValueAs(env.getVariable("value"), float);
   EXPECT_FLOAT_EQ(value, 100.0f);
}

TEST(Cflat, RangeBasedForWithArray)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "int array[] = { 42, 42 };\n"
      "void func()\n"
      "{\n"
      "  for(int& val : array)\n"
      "  {\n"
      "    val += 1;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 2u);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 43);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 43);
}

TEST(Cflat, RangeBasedForWithArrayAndAuto)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "int array[] = { 42, 42 };\n"
      "void func()\n"
      "{\n"
      "  for(auto& val : array)\n"
      "  {\n"
      "    val += 1;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   Cflat::Value* arrayValue = env.getVariable("array");
   EXPECT_EQ(arrayValue->mTypeUsage.mArraySize, 2u);

   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 0, int), 43);
   EXPECT_EQ(CflatValueAsArrayElement(arrayValue, 1, int), 43);
}

TEST(Cflat, RangeBasedForWithStdVector)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec;\n"
      "void func()\n"
      "{\n"
      "  vec.push_back(42);\n"
      "  vec.push_back(42);\n"
      "  \n"
      "  for(int& val : vec)\n"
      "  {\n"
      "    val += 10;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::vector<int>& vec = CflatValueAs(env.getVariable("vec"), std::vector<int>);
   EXPECT_EQ(vec.size(), 2u);
   EXPECT_EQ(vec[0], 52);
   EXPECT_EQ(vec[1], 52);
}

TEST(Cflat, RangeBasedForWithStdVectorAndAuto)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec;\n"
      "void func()\n"
      "{\n"
      "  vec.push_back(42);\n"
      "  vec.push_back(42);\n"
      "  \n"
      "  for(auto& val : vec)\n"
      "  {\n"
      "    val += 10;\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::vector<int>& vec = CflatValueAs(env.getVariable("vec"), std::vector<int>);
   EXPECT_EQ(vec.size(), 2u);
   EXPECT_EQ(vec[0], 52);
   EXPECT_EQ(vec[1], 52);
}

TEST(Cflat, TypeUsagesWithTemplateArguments)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);
   CflatRegisterSTLMap(&env, int, float);

   Cflat::TypeUsage typeUsage = env.getTypeUsage("std::vector<int>");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("std::vector<int>*");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("std::vector<int>&");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("const std::vector<int>&");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("std::map<int, float>");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("std::map<int, float>*");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("std::map<int, float>&");
   EXPECT_TRUE(typeUsage.mType);
   typeUsage = env.getTypeUsage("const std::map<int, float>&");
   EXPECT_TRUE(typeUsage.mType);
}

TEST(Cflat, BinaryOperatorDefinedAsFunction)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "std::string str1(\"Hello \");\n"
      "std::string str2(\"world!\");\n"
      "std::string str3 = str1 + str2;\n";

   EXPECT_TRUE(env.load("test", code));

   std::string& str3 = CflatValueAs(env.getVariable("str3"), std::string);
   EXPECT_EQ(strcmp(str3.c_str(), "Hello world!"), 0);
}

TEST(Cflat, UsingNamespace)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "using namespace std;\n"
      "string str;\n"
      "void func()\n"
      "{\n"
      "  str.assign(\"Hello world!\");\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, UsingNamespaceInsideNamespace)
{
   Cflat::Environment env;

   const char* code =
      "namespace Consts\n"
      "{\n"
      "  static const float kConst = 42.0f;\n"
      "}\n"
      "namespace Vars\n"
      "{\n"
      "  using namespace Consts;\n"
      "  float var = kConst;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("Vars::var"), float), 42.0f);
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
   }

   const char* code =
      "TestStruct testStruct;\n"
      "void func()\n"
      "{\n"
      "  testStruct.var1 = 42;\n"
      "  testStruct.var2 = 100;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

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
   }

   const char* code =
      "TestStruct testStruct;\n"
      "TestStruct* testStructPtr = &testStruct;\n"
      "void func()\n"
      "{\n"
      "  testStructPtr->var1 = 42;\n"
      "  testStructPtr->var2 = 100;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var1, 42);
   EXPECT_EQ(testStruct.var2, 100);
}

TEST(Cflat, IndirectionOperator)
{
   Cflat::Environment env;

   const char* code =
      "int var1 = 42;\n"
      "int* ptr = &var1;\n"
      "int var2 = *ptr;\n"
      "*ptr *= 2;\n";

   EXPECT_TRUE(env.load("test", code));
   
   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 84);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 42);
}

TEST(Cflat, VoidPtr)
{
   Cflat::Environment env;

   const char* code =
      "void* ptr = nullptr;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("ptr"), void*), nullptr);
}

TEST(Cflat, NullptrComparison)
{
   Cflat::Environment env;

   struct TestStruct {};

   {
      CflatRegisterStruct(&env, TestStruct);
   }

   const char* code =
      "TestStruct* test = nullptr;\n"
      "const bool testIsValid = test != nullptr;\n"
      "const bool testIsInvalid = test == nullptr;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("testIsValid"), bool), false);
   EXPECT_EQ(CflatValueAs(env.getVariable("testIsInvalid"), bool), true);
}

TEST(Cflat, SizeOf)
{
   Cflat::Environment env;

   struct TestStruct
   {
      float var1;
      int var2;
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, float, var1);
      CflatStructAddMember(&env, TestStruct, int, var2);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "const size_t size1 = sizeof(TestStruct);\n"
      "const size_t size2 = sizeof(testStruct);\n";

   EXPECT_TRUE(env.load("test", code));

   const size_t size = sizeof(TestStruct);
   EXPECT_EQ(CflatValueAs(env.getVariable("size1"), size_t), size);
   EXPECT_EQ(CflatValueAs(env.getVariable("size2"), size_t), size);
}

TEST(Cflat, SizeOfArray)
{
   Cflat::Environment env;

   const char* code =
      "const float array[] = { 1.0f, 2.0f, 3.0f, 4.0f };\n"
      "const size_t arraySize = sizeof(array) / sizeof(float);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("arraySize"), size_t), 4u);
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
      CflatStructAddMethodVoid(&env, TestStruct, void, method);
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
      CflatStructAddMethodVoidParams1(&env, TestStruct, void, method, int);
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
      CflatStructAddMethodVoidParams1(&env, TestStruct, void, method, int);
   }

   const char* code =
      "TestStruct testStruct;\n"
      "TestStruct* testStructPtr = &testStruct;\n"
      "testStructPtr->method(42);\n";

   EXPECT_TRUE(env.load("test", code));

   TestStruct& testStruct = CflatValueAs(env.getVariable("testStruct"), TestStruct);
   EXPECT_EQ(testStruct.var, 42);
}

TEST(Cflat, MethodOverload)
{
   Cflat::Environment env;

   class TestClass
   {
   private:
      bool mOverload1Called;
      bool mOverload2Called;
   public:
      TestClass() : mOverload1Called(false), mOverload2Called(false) {}
      void method(int pValue) { mOverload1Called = true; }
      void method(float pValue) { mOverload2Called = true; }
      bool getOverload1Called() const { return mOverload1Called; }
      bool getOverload2Called() const { return mOverload2Called; }
   };

   {
      CflatRegisterClass(&env, TestClass);
      CflatClassAddConstructor(&env, TestClass);
      CflatClassAddMethodVoidParams1(&env, TestClass, void, method, int);
      CflatClassAddMethodVoidParams1(&env, TestClass, void, method, float);
   }

   const char* code =
      "TestClass testClass;\n"
      "testClass.method(42);\n"
      "testClass.method(42.0f);\n";

   EXPECT_TRUE(env.load("test", code));

   TestClass& testClass = CflatValueAs(env.getVariable("testClass"), TestClass);
   EXPECT_TRUE(testClass.getOverload1Called());
   EXPECT_TRUE(testClass.getOverload2Called());
}

template<typename T>
static T add(T a, T b)
{
   return a + b;
}

TEST(Cflat, FunctionCallWithTemplateType)
{
   Cflat::Environment env;

   CflatRegisterFunctionReturnParams2(&env, int, add, int, int);
   CflatRegisterFunctionReturnParams2(&env, float, add, float, float);

   const char* code =
      "const int intValue = add(1, 2);\n"
      "const float floatValue = add(10.0f, 20.0f);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("intValue"), int), 3);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("floatValue"), float), 30.0f);
}

struct TestStructWithTemplateMethod
{
   int value;
   TestStructWithTemplateMethod() : value(42) {}
   template<typename T> T get() { return (T)value; }
   template<typename T> T get(T pOffset) { return pOffset + (T)value; }
   template<typename T> T get(T pOffset, T pScale) { return pOffset + ((T)value * pScale); }
};

TEST(Cflat, MethodCallWithTemplateType)
{
   Cflat::Environment env;

   {
      CflatRegisterStruct(&env, TestStructWithTemplateMethod);
      CflatStructAddConstructor(&env, TestStructWithTemplateMethod);
      CflatStructAddTemplateMethodReturn(&env, TestStructWithTemplateMethod, int, int, get);
      CflatStructAddTemplateMethodReturn(&env, TestStructWithTemplateMethod, float, float, get);
      CflatStructAddTemplateMethodReturnParams1(&env, TestStructWithTemplateMethod, int, int, get, int);
      CflatStructAddTemplateMethodReturnParams2(&env, TestStructWithTemplateMethod, int, int, get, int, int);
      CflatStructAddTemplateMethodReturnParams1(&env, TestStructWithTemplateMethod, float, float, get, float);
      CflatStructAddTemplateMethodReturnParams2(&env, TestStructWithTemplateMethod, float, float, get, float, float);
   }

   const char* code =
      "TestStructWithTemplateMethod test;\n"
      "const int intValue = test.get<int>();\n"
      "const int intValueWithOffset = test.get<int>(10);\n"
      "const int intValueWithOffsetAndScale = test.get<int>(10, 2);\n"
      "const float floatValue = test.get<float>();\n"
      "const float floatValueWithOffset = test.get<float>(10.0f);\n"
      "const float floatValueWithOffsetAndScale = test.get<float>(10.0f, 2.0f);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("intValue"), int), 42);
   EXPECT_EQ(CflatValueAs(env.getVariable("intValueWithOffset"), int), 52);
   EXPECT_EQ(CflatValueAs(env.getVariable("intValueWithOffsetAndScale"), int), 94);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("floatValue"), float), 42.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("floatValueWithOffset"), float), 52.0f);
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("floatValueWithOffsetAndScale"), float), 94.0f);
}

struct TestStruct
{
   static int staticVar;
};
int TestStruct::staticVar = 0;

TEST(Cflat, StaticMember)
{
   Cflat::Environment env;

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddStaticMember(&env, TestStruct, int, staticVar);
   }

   const char* code =
      "TestStruct::staticVar = 42;\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(TestStruct::staticVar, 42);
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
      CflatStructAddStaticMethodVoid(&env, TestStruct, void, incrementStaticVar);
   }

   const char* code =
      "TestStruct::incrementStaticVar();\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(staticVar, 1);
}

TEST(Cflat, StaticVariable)
{
   Cflat::Environment env;

   const char* code =
      "int func()\n"
      "{\n"
      "  static int var = 0;\n"
      "  return ++var;\n"
      "}\n"
      "int var1 = func();\n"
      "int var2 = func();\n"
      "int var3 = func();\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("var1"), int), 1);
   EXPECT_EQ(CflatValueAs(env.getVariable("var2"), int), 2);
   EXPECT_EQ(CflatValueAs(env.getVariable("var3"), int), 3);
}

TEST(Cflat, Destructor)
{
   Cflat::Environment env;

   static int staticVar = 0;

   struct TestStruct
   {
      ~TestStruct() { staticVar++; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddDestructor(&env, TestStruct);
   }

   const char* code =
      "void func()\n"
      "{\n"
      "  TestStruct testStruct;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

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

   CflatArgsVector(Cflat::Value) args;

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

   CflatArgsVector(Cflat::Value) args;
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

TEST(Cflat, FunctionDeclarationInNamespace)
{
   Cflat::Environment env;

   const char* code =
      "namespace Test\n"
      "{\n"
      "  void func()\n"
      "  {\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Function* func = env.getFunction("func");
   EXPECT_FALSE(func);

   func = env.getFunction("Test::func");
   EXPECT_TRUE(func);
}

TEST(Cflat, FunctionDeclarationsInNamespace)
{
   Cflat::Environment env;

   const char* code =
      "namespace Test\n"
      "{\n"
      "  void func1()\n"
      "  {\n"
      "  }\n"
      "  void func2()\n"
      "  {\n"
      "  }\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(env.getFunction("Test::func1"));
   EXPECT_TRUE(env.getFunction("Test::func2"));
}

TEST(Cflat, FunctionDeclarationsInNamespaceWithLocalVariables)
{
  Cflat::Environment env;

  const char* code =
    "namespace Test\n"
    "{\n"
    "  void func1()\n"
    "  {\n"
    "    int foo = 0;\n"
    "  }\n"
    "  void func2()\n"
    "  {\n"
    "    int foo = 0;\n"
    "  }\n"
    "}\n";

  EXPECT_TRUE(env.load("test", code));

  EXPECT_TRUE(env.getFunction("Test::func1"));
  EXPECT_TRUE(env.getFunction("Test::func2"));
}

TEST(Cflat, FunctionDeclarationWithReferenceUsages)
{
   Cflat::Environment env;

   const char* code =
      "void func(int& param1, int& param2)\n"
      "{\n"
      "  param1 += param2;\n"
      "}\n"
      "\n"
      "int var1 = 3;\n"
      "int var2 = 2;\n"
      "func(var1, var2);\n";

   EXPECT_TRUE(env.load("test", code));

   int& var1 = CflatValueAs(env.getVariable("var1"), int);
   EXPECT_EQ(var1, 5);
}

TEST(Cflat, StructDeclaration)
{
   Cflat::Environment env;

   const char* code =
      "struct MyStruct\n"
      "{\n"
      "  int mIntValue;\n"
      "  float mFloatValue;\n"
      "};\n";

   EXPECT_TRUE(env.load("test", code));

   Cflat::Type* type = env.getType("MyStruct"); 
   EXPECT_TRUE(type);
   EXPECT_EQ(type->mSize, sizeof(int) + sizeof(float));
}

TEST(Cflat, TypeDefinitionGlobal)
{
   Cflat::Environment env;

   const char* code =
      "typedef int NumericType;\n"
      "NumericType var = 42;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeDefinitionLocal)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "void func()\n"
      "{\n"
      "  typedef int NumericType;\n"
      "  NumericType tempValue = 42;\n"
      "  var = tempValue;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeUsageDefinitionGlobal)
{
   Cflat::Environment env;

   const char* code =
      "typedef int* NumericPtrType;\n"
      "int var = 42;\n"
      "NumericPtrType ptr = &var;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   int* ptr = CflatValueAs(env.getVariable("ptr"), int*);
   EXPECT_EQ(var, 42);
   EXPECT_EQ(*ptr, 42);
}

TEST(Cflat, TypeUsageDefinitionLocal)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "void func()\n"
      "{\n"
      "  typedef int* NumericPtrType;\n"
      "  NumericPtrType ptr = &var;\n"
      "  *ptr = 42;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeAliasGlobal)
{
   Cflat::Environment env;

   const char* code =
      "using NumericType = int;\n"
      "NumericType var = 42;";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeAliasLocal)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "void func()\n"
      "{\n"
      "  using NumericType = int;\n"
      "  NumericType tempValue = 42;\n"
      "  var = tempValue;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeUsageAliasGlobal)
{
   Cflat::Environment env;

   const char* code =
      "using NumericPtrType = int*;\n"
      "int var = 42;\n"
      "NumericPtrType ptr = &var;\n";

   EXPECT_TRUE(env.load("test", code));

   int var = CflatValueAs(env.getVariable("var"), int);
   int* ptr = CflatValueAs(env.getVariable("ptr"), int*);
   EXPECT_EQ(var, 42);
   EXPECT_EQ(*ptr, 42);
}

TEST(Cflat, TypeUsageAliasLocal)
{
   Cflat::Environment env;

   const char* code =
      "int var = 0;\n"
      "void func()\n"
      "{\n"
      "  using NumericPtrType = int*;\n"
      "  NumericPtrType ptr = &var;\n"
      "  *ptr = 42;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   int var = CflatValueAs(env.getVariable("var"), int);
   EXPECT_EQ(var, 42);
}

TEST(Cflat, TypeUsingGlobal)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "using std::string;\n"
      "string str(\"Hello world!\");";

   EXPECT_TRUE(env.load("test", code));

   const std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, TypeUsingLocal)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "std::string str;\n"
      "void func()\n"
      "{\n"
      "  using std::string;\n"
      "  string innerStr(\"Hello world!\");\n"
      "  str = innerStr;\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   const std::string& str = CflatValueAs(env.getVariable("str"), std::string);
   EXPECT_EQ(strcmp(str.c_str(), "Hello world!"), 0);
}

TEST(Cflat, FunctionCallAsArgument)
{
   Cflat::Environment env;

   const char* code =
      "int add(int pOp1, int pOp2)\n"
      "{\n"
         "return pOp1 + pOp2;\n"
      "}\n"
      "\n"
      "int result = add(add(1, 2), add(3, 4));\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("result"), int), 10);
}

TEST(Cflat, RecursiveFunctionCall)
{
   Cflat::Environment env;

   const char* code =
      "int factorial(int pNum)\n"
      "{\n"
         "return pNum == 1 ? 1 : pNum * factorial(pNum - 1);\n"
      "}\n"
      "\n"
      "int fac5 = factorial(5);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_EQ(CflatValueAs(env.getVariable("fac5"), int), 120);
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
      CflatStructAddMethodReturnParams1(&env, TestStruct, const TestStruct, operator+, int);
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

   Cflat::Helper::registerStdString(&env);

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
      CflatClassAddMethodVoidParams1(&env, TestClass, void, setInternalValue, int);
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

TEST(Cflat, RegisteringNestedTypes)
{
   Cflat::Environment env;

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
      CflatRegisterClass(&env, OuterType);
   }
   {
      CflatRegisterNestedClass(&env, OuterType, InnerType);
      CflatClassAddMember(&env, InnerType, int, value);
   }
   {
      CflatRegisterNestedEnum(&env, OuterType, InnerEnum);
      CflatNestedEnumAddValue(&env, OuterType, InnerEnum, kInnerEnumValue);
   }

   const char* code =
      "OuterType::InnerType innerType;\n"
      "innerType.value = 42;\n"
      "OuterType::InnerEnum innerEnum = OuterType::kInnerEnumValue;\n";
   EXPECT_TRUE(env.load("test", code));

   OuterType::InnerType& innerType = CflatValueAs(env.getVariable("innerType"), OuterType::InnerType);
   EXPECT_EQ(innerType.value, 42);

   OuterType::InnerEnum innerEnum = CflatValueAs(env.getVariable("innerEnum"), OuterType::InnerEnum);
   EXPECT_EQ(innerEnum, OuterType::kInnerEnumValue);
}

TEST(Cflat, CastCStyleBuiltInTypes)
{
   Cflat::Environment env;

   const char* code =
      "int ival = 42;\n"
      "float fval = (float)ival;\n";

   EXPECT_TRUE(env.load("test", code));
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fval"), float), 42.0f);
}

TEST(Cflat, CastCStyleWithMethodCall)
{
   Cflat::Environment env;

   CflatRegisterSTLVector(&env, int);

   const char* code =
      "std::vector<int> vec;\n"
      "uint32_t vecSize = 0u;\n"
      "void func()\n"
      "{\n"
      "  vec.push_back(42);\n"
      "  vec.push_back(43);\n"
      "  vec.push_back(44);\n"
      "  vec.push_back(45);\n"
      "  vecSize = (uint32_t)vec.size();\n"
      "}\n";

   EXPECT_TRUE(env.load("test", code));
   env.voidFunctionCall(env.getFunction("func"));

   const uint32_t vecSize = CflatValueAs(env.getVariable("vecSize"), uint32_t);
   EXPECT_EQ(vecSize, 4u);
}

TEST(Cflat, CastStaticBuiltInTypes)
{
   Cflat::Environment env;

   const char* code =
      "int ival = 42;\n"
      "float fval = static_cast<float>(ival);\n";

   EXPECT_TRUE(env.load("test", code));
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("fval"), float), 42.0f);
}

TEST(Cflat, CastStaticBuiltInTypesImplicit)
{
   Cflat::Environment env;

   const char* code =
      "float var = 0.0f;\n"
      "void setVar(float value)\n"
      "{\n"
      "  var = value;\n"
      "}\n"
      "setVar(42);\n";

   EXPECT_TRUE(env.load("test", code));
   EXPECT_FLOAT_EQ(CflatValueAs(env.getVariable("var"), float), 42.0f);
}

TEST(Cflat, CastStaticBaseType)
{
   Cflat::Environment env;

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

   const char* code =
      "Derived derived;\n"
      "derived.baseMember = 42;\n"
      "Base* base = static_cast<Base*>(&derived);\n";

   EXPECT_TRUE(env.load("test", code));

   Base* base = CflatValueAs(env.getVariable("base"), Base*);
   EXPECT_EQ(base->baseMember, 42);
}

TEST(Cflat, CastStaticBaseTypeImplicit)
{
   Cflat::Environment env;

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

   const char* code =
      "void funcA(BaseA* baseA)\n"
      "{\n"
      "  baseA->baseAMember = 42;\n"
      "}\n"
      "void funcB(BaseB* baseB)\n"
      "{\n"
      "  baseB->baseBMember = 43;\n"
      "}\n"
      "Derived derived;\n"
      "derived.baseAMember = 0;\n"
      "derived.baseBMember = 0;\n"
      "derived.derivedMember = 0;\n"
      "funcA(&derived);\n"
      "funcB(&derived);\n";

   EXPECT_TRUE(env.load("test", code));

   Derived& derived = CflatValueAs(env.getVariable("derived"), Derived);
   EXPECT_EQ(derived.baseAMember, 42);
   EXPECT_EQ(derived.baseBMember, 43);
}

TEST(Cflat, CastDynamic)
{
   Cflat::Environment env;

   struct Base
   {
      int baseMember;
   };
   struct Derived : Base
   {
      int derivedMember;
   };
   struct Other
   {
      int otherMember;
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
   {
      CflatRegisterStruct(&env, Other);
      CflatStructAddMember(&env, Other, int, otherMember);
   }

   const char* code =
      "Derived derived;\n"
      "Base* base = static_cast<Base*>(&derived);\n"
      "Derived* castDerived = dynamic_cast<Derived*>(base);\n"
      "Other* castOther = dynamic_cast<Other*>(base);\n";

   EXPECT_TRUE(env.load("test", code));

   EXPECT_TRUE(CflatValueAs(env.getVariable("castDerived"), Derived*));
   EXPECT_FALSE(CflatValueAs(env.getVariable("castOther"), Other*));
}

TEST(Cflat, CastReinterpret)
{
   Cflat::Environment env;

   struct Base
   {
      int baseMember;
   };
   struct Other
   {
      int otherMember;
   };

   {
      CflatRegisterStruct(&env, Base);
      CflatStructAddMember(&env, Base, int, baseMember);
   }
   {
      CflatRegisterStruct(&env, Other);
      CflatStructAddMember(&env, Other, int, otherMember);
   }

   const char* code =
      "Base base;\n"
      "base.baseMember = 42;\n"
      "Other* other = reinterpret_cast<Other*>(&base);\n";

   EXPECT_TRUE(env.load("test", code));

   Other* other = CflatValueAs(env.getVariable("other"), Other*);
   EXPECT_EQ(other->otherMember, 42);
}

TEST(Cflat, Logging)
{
   Cflat::Environment env;

   std::streambuf* coutBuf = std::cout.rdbuf();
   std::ostringstream output;
   std::cout.rdbuf(output.rdbuf());

   Cflat::Helper::registerStdOut(&env);

   const char* code =
      "int var = 42;\n"
      "std::cout << \"This is my value: \" << var << \"\\n\";\n";

   EXPECT_TRUE(env.load("test", code));

   const std::string outputContent = output.str();
   EXPECT_EQ(strcmp(outputContent.c_str(), "This is my value: 42\n"), 0);

   std::cout.rdbuf(coutBuf);
}

TEST(Debugging, ExpressionEvaluation)
{
   Cflat::Environment env;

   struct TestStruct
   {
      int var1;
      int var2;

      int getVar1(int pFactor = 1) { return var1 * pFactor; }
   };

   {
      CflatRegisterStruct(&env, TestStruct);
      CflatStructAddMember(&env, TestStruct, int, var1);
      CflatStructAddMember(&env, TestStruct, int, var2);
      CflatStructAddMethodReturn(&env, TestStruct, int, getVar1);
      CflatStructAddMethodReturnParams1(&env, TestStruct, int, getVar1, int);
   }

   const char* code =
      "float testValue = 42.0f;\n"
      "TestStruct testStruct;\n"
      "testStruct.var1 = 42;\n"
      "testStruct.var2 = 100;\n"
      "int unused = 0;\n";

   env.setExecutionHook([](Cflat::Environment* pEnvironment, const Cflat::CallStack& pCallStack)
   {
      if(!pCallStack.empty() && pCallStack.back().mLine == 6u)
      {
         Cflat::Value testValue;
         EXPECT_TRUE(pEnvironment->evaluateExpression("testValue", &testValue));
         EXPECT_FLOAT_EQ(CflatValueAs(&testValue, float), 42.0f);

         Cflat::Value var1;
         EXPECT_TRUE(pEnvironment->evaluateExpression("testStruct.var1", &var1));
         int expressionValue = CflatValueAs(&var1, int);
         EXPECT_EQ(expressionValue, 42);

         Cflat::Value var2;
         EXPECT_TRUE(pEnvironment->evaluateExpression("testStruct.var2", &var2));
         expressionValue = CflatValueAs(&var2, int);
         EXPECT_EQ(expressionValue, 100);

         Cflat::Value getVar1ReturnValue1;
         EXPECT_TRUE(pEnvironment->evaluateExpression("testStruct.getVar1()", &getVar1ReturnValue1));
         expressionValue = CflatValueAs(&getVar1ReturnValue1, int);
         EXPECT_EQ(expressionValue, 42);

         Cflat::Value getVar1ReturnValue2;
         EXPECT_TRUE(pEnvironment->evaluateExpression("testStruct.getVar1(2)", &getVar1ReturnValue2));
         expressionValue = CflatValueAs(&getVar1ReturnValue2, int);
         EXPECT_EQ(expressionValue, 84);
      }
   });

   EXPECT_TRUE(env.load("test", code));
}

TEST(PreprocessorErrors, InvalidMacroArgumentCount)
{
   Cflat::Environment env;
   env.defineMacro("ADD(a, b)", "(a + b)");

   const char* code =
      "int var = ADD(42);\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Preprocessor Error] 'test' -- Line 1: invalid number of arguments for the 'ADD' macro"), 0);
}

TEST(CompileErrors, VoidVariable)
{
   Cflat::Environment env;

   const char* code =
      "void var;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Compile Error] 'test' -- Line 1: invalid type ('void')"), 0);
}

TEST(CompileErrors, InvalidAssignment)
{
   Cflat::Environment env;

   const char* code =
      "void func() {}\n"
      "int var = func();\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Compile Error] 'test' -- Line 2: invalid assignment"), 0);
}

TEST(CompileErrors, InvalidFloatingPointFormat)
{
  Cflat::Environment env;

  const char* code =
    "const float var = 0f;\n";

  EXPECT_FALSE(env.load("test", code));
  EXPECT_EQ(strcmp(env.getErrorMessage(),
    "[Compile Error] 'test' -- Line 1: invalid literal ('0f')"), 0);
}

TEST(CompileErrors, ConstModificationAssignment)
{
   Cflat::Environment env;

   const char* code =
      "const int var = 42;\n"
      "var = 0;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Compile Error] 'test' -- Line 2: cannot modify constant expression"), 0);
}

TEST(CompileErrors, ConstModificationIncrement)
{
   Cflat::Environment env;

   const char* code =
      "const int var = 42;\n"
      "var++;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Compile Error] 'test' -- Line 2: cannot modify constant expression"), 0);
}

TEST(CompileErrors, ConstModificationDecrement)
{
   Cflat::Environment env;

   const char* code =
      "const int var = 42;\n"
      "var--;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Compile Error] 'test' -- Line 2: cannot modify constant expression"), 0);
}

TEST(RuntimeErrors, NullPointerAccess)
{
   Cflat::Environment env;

   Cflat::Helper::registerStdString(&env);

   const char* code =
      "std::string* strPtr = nullptr;\n"
      "strPtr->assign(\"Hello world!\");\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Runtime Error] 'test' -- Line 2: null pointer access ('assign')"), 0);
}

TEST(RuntimeErrors, InvalidArrayIndex)
{
   Cflat::Environment env;

   const char* code =
      "int array[] = { 0, 1, 2 };\n"
      "int arrayIndex = 42;\n"
      "int var = array[arrayIndex];\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Runtime Error] 'test' -- Line 3: invalid array index (size 3, index 42)"), 0);
}

TEST(RuntimeErrors, DivisionByZero)
{
   Cflat::Environment env;

   const char* code =
      "int val = 10 / 0;\n";

   EXPECT_FALSE(env.load("test", code));
   EXPECT_EQ(strcmp(env.getErrorMessage(),
      "[Runtime Error] 'test' -- Line 1: division by zero"), 0);
}

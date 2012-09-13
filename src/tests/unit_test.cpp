#include "unit_test.h"

/*----------------------------------------------------------------------------*\
Internal
\*----------------------------------------------------------------------------*/
namespace {

TEST(CanMakeTest)
{
}

IGNORE_TEST(CanFailATest)
{
    FAIL("Failure!");
}

TEST(CheckIntEqual)
{
    CHECK_EQUAL(33, 33);
    CHECK_NOT_EQUAL(-56, 56);
}
TEST(CheckIntLTGT)
{
    CHECK_LESS_THAN(458, 558);
    CHECK_GREATER_THAN(-564, -664);
    CHECK_LESS_THAN_EQUAL(458, 558);
    CHECK_LESS_THAN_EQUAL(458, 458);
    CHECK_GREATER_THAN_EQUAL(-564, -564);
    CHECK_GREATER_THAN_EQUAL(-564, -664);
}
TEST(CheckPointerEqual)
{
    int* a = (int*)0xDEADBEEF;
    int* b = (int*)0xDEADBEEF;
    int* c = (int*)0xBEEFDEAD;
    int* d = 0;
    CHECK_EQUAL_POINTER(a, b);
    CHECK_NOT_EQUAL_POINTER(a, c);
    CHECK_NULL(d);
    CHECK_NOT_NULL(a);
}
TEST(String)
{
    const char* a = "Hello World";
    const char* b = "Hello World";
    const char* c = "Goodbye world";
    c++;
    CHECK_EQUAL_STRING(a, b);
    CHECK_NOT_EQUAL_STRING(a, c);
}
struct TestFixture
{
    TestFixture()
        : test_int(42)
    {
    }
    ~TestFixture()
    {
    }

    int test_int;
};

TEST_FIXTURE(TestFixture, TestInt)
{
    CHECK_EQUAL(42, test_int);
}

}

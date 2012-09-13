/*  @file unit_test.h
 *  @brief Automated unit testing
 *  @author Kyle Weicht
 *  @date 6/25/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __unit_test_h_
#define __unit_test_h_

/*----------------------------------------------------------------------------*/
/* C headers */
#include <stdint.h>
#include <string.h>
/* C++ headers */
/* External headers */
/* Internal headers */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus

/**
 * Test macros and functions
 */
#define TEST(test_name)                                                     \
    static void TEST_##test_name(void);                                     \
    static int _##test_name##_register = register_test(&TEST_##test_name);  \
    static void TEST_##test_name(void)
#define IGNORE_TEST(test_name)                                              \
    static void TEST_##test_name(void);                                     \
    static int _##test_name##_register = ignore_test(&TEST_##test_name);    \
    static void TEST_##test_name(void)
    
typedef void (test_func_t)(void);
int register_test(test_func_t* test);
int ignore_test(test_func_t* test);

#define TEST_FIXTURE(fixture, test_name)                                                           \
    struct TEST_##test_name : public fixture                                                       \
    {                                                                                              \
        void test(void);                                                                           \
    };                                                                                             \
    static void TEST_##fixture##_##test_name(void)                                                 \
    {                                                                                              \
        TEST_##test_name test;                                                                     \
        test.test();                                                                               \
    }                                                                                              \
    static int _##fixture##_##test_name##_register = register_test(&TEST_##fixture##_##test_name); \
    void TEST_##test_name::test(void )

#define IGNORE_TEST_FIXTURE(fixture, test_name)                                                    \
    struct TEST_##test_name : public fixture                                                       \
    {                                                                                              \
        void test(void);                                                                           \
    };                                                                                             \
    static void TEST_##fixture##_##test_name(void)                                                 \
    {                                                                                              \
        TEST_##test_name test;                                                                     \
        test.test();                                                                               \
    }                                                                                              \
    static int _##fixture##_##test_name##_register = ignore_test(&TEST_##fixture##_##test_name);   \
    void TEST_##test_name::test(void )

extern "C" { // Use C linkage
#endif 

/**
 * Checking functions
 */
#define FAIL(message)   \
    fail(__FILE__, __LINE__, message)
void fail(const char* file, int line, const char* message);

/* bool */
#define CHECK_TRUE(value) \
    _check_true(__FILE__, __LINE__, (int)(value))
#define CHECK_FALSE(value) \
    _check_false(__FILE__, __LINE__, (int)(value))

void _check_true(const char* file, int line, int value);
void _check_false(const char* file, int line, int value);

/* integer */
#define CHECK_EQUAL(expected, actual) \
    _check_equal(__FILE__, __LINE__, (int64_t)expected, (int64_t)actual)
#define CHECK_NOT_EQUAL(expected, actual) \
    _check_not_equal(__FILE__, __LINE__, (int64_t)expected, (int64_t)actual)
#define CHECK_LESS_THAN(left, right) \
    _check_less_than(__FILE__, __LINE__, (int64_t)left, (int64_t)right)
#define CHECK_GREATER_THAN(left, right) \
    _check_greater_than(__FILE__, __LINE__, (int64_t)left, (int64_t)right)
#define CHECK_LESS_THAN_EQUAL(left, right) \
    _check_less_than_equal(__FILE__, __LINE__, (int64_t)left, (int64_t)right)
#define CHECK_GREATER_THAN_EQUAL(left, right) \
    _check_greater_than_equal(__FILE__, __LINE__, (int64_t)left, (int64_t)right)
    
void _check_equal(const char* file, int line, int64_t expected, int64_t actual);
void _check_not_equal(const char* file, int line, int64_t expected, int64_t actual);
void _check_less_than(const char* file, int line, int64_t left, int64_t right);
void _check_greater_than(const char* file, int line, int64_t left, int64_t right);
void _check_less_than_equal(const char* file, int line, int64_t left, int64_t right);
void _check_greater_than_equal(const char* file, int line, int64_t left, int64_t right);
    
/* pointer */
#define CHECK_EQUAL_POINTER(expected, actual) \
    _check_equal_pointer(__FILE__, __LINE__, (const void*)expected, (const void*)actual)
#define CHECK_NOT_EQUAL_POINTER(expected, actual) \
    _check_not_equal_pointer(__FILE__, __LINE__, (const void*)expected, (const void*)actual)
#define CHECK_NULL(ptr) \
    _check_null(__FILE__, __LINE__, (const void*)ptr)
#define CHECK_NOT_NULL(ptr) \
    _check_not_null(__FILE__, __LINE__, (const void*)ptr)
    
void _check_equal_pointer(const char* file, int line, const void* expected, const void* actual);
void _check_not_equal_pointer(const char* file, int line, const void* expected, const void* actual);
void _check_null(const char* file, int line, const void* pointer);
void _check_not_null(const char* file, int line, const void* pointer);

/* float */
#define CHECK_EQUAL_FLOAT(expected, actual) \
    _check_equal_float(__FILE__, __LINE__, (double)expected, (double)actual)
#define CHECK_NOT_EQUAL_FLOAT(expected, actual) \
    _check_not_equal_float(__FILE__, __LINE__, (double)expected, (double)actual)
#define CHECK_LESS_THAN_FLOAT(left, right) \
    _check_less_than_float(__FILE__, __LINE__, (double)left, (double)right)
#define CHECK_GREATER_THAN_FLOAT(left, right) \
    _check_greater_than_float(__FILE__, __LINE__, (double)left, (double)right)
#define CHECK_LESS_THAN_EQUAL_FLOAT(left, right) \
    _check_less_than_equal_float(__FILE__, __LINE__, (double)left, (double)right)
#define CHECK_GREATER_THAN_EQUAL_FLOAT(left, right) \
    _check_greater_than_equal_float(__FILE__, __LINE__, (double)left, (double)right)
    
void _check_equal_float(const char* file, int line, double expected, double actual);
void _check_not_equal_float(const char* file, int line, double expected, double actual);
void _check_less_than_float(const char* file, int line, double left, double right);
void _check_greater_than_float(const char* file, int line, double left, double right);
void _check_less_than_equal_float(const char* file, int line, double left, double right);
void _check_greater_than_equal_float(const char* file, int line, double left, double right);

/* string */
#define CHECK_EQUAL_STRING(expected, actual) \
    _check_equal_string(__FILE__, __LINE__, expected, actual)
#define CHECK_NOT_EQUAL_STRING(expected, actual) \
    _check_not_equal_string(__FILE__, __LINE__, expected, actual)
    
void _check_equal_string(const char* file, int line, const char* expected, const char* actual);
void _check_not_equal_string(const char* file, int line, const char* expected, const char* actual);

/** Runs all registered tests and returns the number of failures */
int run_all_tests(int argc, const char* argv[]);

#define RUN_ALL_TESTS(argc, argv, test_arg)         \
    do {                                            \
        int _ii;                                     \
        for(_ii=0;_ii<argc;++_ii)                      \
            if(strcmp(argv[_ii], test_arg) == 0)     \
                return run_all_tests(argc, argv);   \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* include guard */

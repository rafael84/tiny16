#ifndef TESTING_H
#define TESTING_H

/*
 * Minimal unit testing framework for C.
 *
 * Platform independent - uses only standard C library functions:
 *   - setjmp/longjmp for non-local jumps on assertion failure
 *   - stdio for output
 *   - string for memset
 *
 * Works on: Windows (MSVC, MinGW), Linux (GCC, Clang), macOS (Clang)
 */

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* Maximum number of tests and failures we can track */
#define TEST_MAX_TESTS    256
#define TEST_MAX_FAILURES 64

/* Test case structure */
typedef struct {
    const char* name;
    void (*func)(void);
} TestCase;

/* Failure record structure */
typedef struct {
    const char* test_name;
    const char* file;
    int line;
    const char* expr;
} TestFailure;

/* Global test context */
typedef struct {
    TestCase tests[TEST_MAX_TESTS];
    int test_count;
    TestFailure failures[TEST_MAX_FAILURES];
    int failure_count;
    int pass_count;
    const char* current_test;
    jmp_buf jump_buf;
    int in_test;
} TestContext;

// Global test context
extern TestContext g_test_ctx;

// Initialize the test context
void test_init(void);

// Register a test
void test_register(const char* name, void (*func)(void));

// Run all registered tests
int test_run_all(void);

// Internal: record a failure and longjmp
void test_fail(const char* file, int line, const char* expr);

// Custom assertion macro that uses our test framework
#define TEST_ASSERT(expr)                                                                          \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            test_fail(__FILE__, __LINE__, #expr);                                                  \
        }                                                                                          \
    } while (0)

// Macro to register and declare a test
#define TEST(fn)                                                                                   \
    void fn(void);                                                                                 \
    test_register(#fn, fn)

// Macro for test main function
#define TEST_MAIN()                                                                                \
    int main(void) {                                                                               \
        test_init();

#define TEST_RUN()                                                                                 \
    return test_run_all();                                                                         \
    }

#endif // TESTING_H

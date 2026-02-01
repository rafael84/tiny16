#include "testing.h"

/* Global test context */
TestContext g_test_ctx;

void test_init(void) { memset(&g_test_ctx, 0, sizeof(g_test_ctx)); }

void test_register(const char* name, void (*func)(void)) {
    if (g_test_ctx.test_count < TEST_MAX_TESTS) {
        g_test_ctx.tests[g_test_ctx.test_count].name = name;
        g_test_ctx.tests[g_test_ctx.test_count].func = func;
        g_test_ctx.test_count++;
    }
}

void test_fail(const char* file, int line, const char* expr) {
    if (g_test_ctx.failure_count < TEST_MAX_FAILURES) {
        TestFailure* f = &g_test_ctx.failures[g_test_ctx.failure_count];
        f->test_name = g_test_ctx.current_test;
        f->file = file;
        f->line = line;
        f->expr = expr;
        g_test_ctx.failure_count++;
    }
    if (g_test_ctx.in_test) {
        longjmp(g_test_ctx.jump_buf, 1);
    }
}

int test_run_all(void) {
    int i;

    /* Run all tests, printing . for pass, F for fail */
    for (i = 0; i < g_test_ctx.test_count; i++) {
        g_test_ctx.current_test = g_test_ctx.tests[i].name;
        g_test_ctx.in_test = 1;

        if (setjmp(g_test_ctx.jump_buf) == 0) {
            g_test_ctx.tests[i].func();
            /* Test passed */
            printf(".");
            g_test_ctx.pass_count++;
        } else {
            /* Test failed (longjmp called from test_fail) */
            printf("F");
        }
        fflush(stdout);

        g_test_ctx.in_test = 0;
    }

    printf("\n");

    /* Print failures */
    if (g_test_ctx.failure_count > 0) {
        printf("\nFailures:\n\n");
        for (i = 0; i < g_test_ctx.failure_count; i++) {
            TestFailure* f = &g_test_ctx.failures[i];
            printf("  %d) %s\n", i + 1, f->test_name);
            printf("     %s:%d\n", f->file, f->line);
            printf("     Assertion failed: %s\n\n", f->expr);
        }
    }

    printf("%d passed, %d failed\n", g_test_ctx.pass_count, g_test_ctx.failure_count);

    return g_test_ctx.failure_count > 0 ? 1 : 0;
}

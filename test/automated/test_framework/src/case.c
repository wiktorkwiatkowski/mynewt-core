#include "test_framework_priv.h"
#include "test_framework/test_framework.h"

static case_context case_ctx;
extern suite_context suite_ctx;

void
mtest_case_init(const char *name)
{
    case_ctx.failed = 0;
    case_ctx.name = name;
    printf("MTEST start=%s\n", case_ctx.name);
}

void
mtest_case_fail()
{
    case_ctx.failed = 1;
}

void
mtest_case_complete()
{
    printf("MTEST end=%s, status=%s\n", case_ctx.name,
           case_ctx.failed != 1 ? "pass" : "fail");

    suite_ctx.tests_run++;

    if (case_ctx.failed) {
        suite_ctx.tests_failed++;
    } else {
        suite_ctx.tests_passed++;
    }
}

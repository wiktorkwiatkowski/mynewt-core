#ifndef TEST_FRAMEWORK_PRIV_H
#define TEST_FRAMEWORK_PRIV_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *name;
    uint8_t failed;
} case_context;

typedef struct
{
    const char *name;
    int tests_run;
    int tests_failed;
    int tests_passed;
    uint8_t suite_aborted;
} suite_context;

#ifdef __cplusplus
}
#endif

#endif

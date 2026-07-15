/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "mtest/mtest.h"
#include "hal/hal_system.h"
#include "hal/hal_watchdog.h"
#include "mtest_watchdog/mtest_watchdog.h"
#include "os/os_cputime.h"

extern struct watchdog_test_ctx test_ctx;
extern volatile int reset_count;

MTEST_CASE(watchdog_test_case_1)
{
    int rc;
    test_ctx.counter_ms = 0;
    enum hal_reset_reason reason = hal_reset_cause();

    rc = os_cputime_timer_relative(&test_ctx.timer_ms, 1000);
    MTEST_CASE_ASSERT(rc == 0, "Timer start failed");
    MTEST_CASE_ASSERT(reason != HAL_RESET_WATCHDOG, "WDOG tickle failed");

    while (1) {
        if (test_ctx.counter_ms >= MYNEWT_VAL(WATCHDOG_INTERVAL) / 4) {
            hal_watchdog_tickle();
            test_ctx.counter_ms = 0;
        }
        if (test_ctx.total_ms >= 2 * MYNEWT_VAL(WATCHDOG_INTERVAL)) {
            rc = hal_timer_stop(&test_ctx.timer_ms);
            test_ctx.total_ms = 0;
            test_ctx.counter_ms = 0;
            MTEST_CASE_ASSERT(rc == 0, "Timer stop failed");
            break;
        }
    }
}

MTEST_CASE(watchdog_test_case_2)
{
    int rc;
    enum hal_reset_reason reason = hal_reset_cause();

    if (reset_count == 1) {
        MTEST_CASE_ASSERT(reason == HAL_RESET_WATCHDOG, "Expected WDOG reset");
        return;
    }

    reset_count++;
    /* Readback required by STM32H723ZG to ensure data reaches DTCM */
    (void)reset_count;

    rc = os_cputime_timer_relative(&test_ctx.timer_ms, 1000);
    MTEST_CASE_ASSERT(rc == 0, "Timer start failed");

    printf("Starving WDOG...\n");
    while (1) {
        if (test_ctx.total_ms > 2 * MYNEWT_VAL(WATCHDOG_INTERVAL)) {
            hal_timer_stop(&test_ctx.timer_ms);
            MTEST_CASE_ASSERT(0, "WDOG failed to fire");
        }
    }
}

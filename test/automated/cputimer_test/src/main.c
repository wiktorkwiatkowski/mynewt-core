#include <stdio.h>
#include <bsp/bsp.h>

#include "console/console.h"
#include "sysinit/sysinit.h"
#include "os/os.h"
#include "os/os_cputime.h"
#include "hal/hal_timer.h"
#include "syscfg/syscfg.h"

static struct hal_timer ms_timer;
static struct os_callout check_callout;

static volatile int ms_counter = 0;

static void
ms_cb(void *arg)
{
    ms_counter++;
    os_cputime_timer_relative(&ms_timer, 1000);
}

static void
check_cb(struct os_event *ev)
{
    static int last = 0;
    int actual = ms_counter - last;
    int expected = MYNEWT_VAL(CHECK_DELAY) * 1000;

    int diff = actual - expected;
    if (diff < 0) {
        diff = -diff;
    }

    int ph = diff * 10000 / expected;

    int ipart = ph / 100;
    int frac = ph % 100;

    int threshold = MYNEWT_VAL(TOLERANCE) * 100;

    const char *result = ph > threshold ? "failed" : "passed";

    console_printf("Check: expected=%d, actual=%d, error=%d.%02d%% %s\n",
           expected,
           actual,
           ipart,
           frac,
           result);

    last = ms_counter;

    os_callout_reset(&check_callout,
                     OS_TICKS_PER_SEC * MYNEWT_VAL(CHECK_DELAY));
}

int
mynewt_main(int argc, char **argv)
{
    sysinit();

    os_cputime_timer_init(&ms_timer, ms_cb, NULL);
    os_callout_init(&check_callout, os_eventq_dflt_get(), check_cb, NULL);

    os_cputime_timer_relative(&ms_timer, 1000);
    os_callout_reset(&check_callout,
                     OS_TICKS_PER_SEC * MYNEWT_VAL(CHECK_DELAY));

    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }

}

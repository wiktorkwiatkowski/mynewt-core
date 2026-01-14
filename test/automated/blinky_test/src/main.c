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

#include "sysinit/sysinit.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "console/console.h"

int
mynewt_main(int argc, char **argv)
{
    sysinit();
    int led_read_pin = MYNEWT_VAL(LED_READ_PIN);
    hal_gpio_init_out(LED_BLINK_PIN, 1);
    hal_gpio_init_in(led_read_pin, HAL_GPIO_PULL_DOWN);
    int prev_state = 1;

    while (1) {

        os_time_delay(OS_TICKS_PER_SEC);

        hal_gpio_toggle(LED_BLINK_PIN);
        int state = hal_gpio_read(led_read_pin);

        if (state != prev_state) {
            console_printf("LED status changed, test passed\n");
        }
        prev_state = state;

    }
}

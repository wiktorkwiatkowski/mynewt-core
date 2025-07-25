/*
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

#include "os/mynewt.h"

#if MYNEWT_VAL(LOG_CONSOLE)

#include <cbmem/cbmem.h>
#include <console/console.h>
#include "log/log.h"
#include "tinycbor/cbor.h"
#include "tinycbor/cbor_buf_reader.h"
#include "datetime/datetime.h"

static int
log_console_dump_cbor_entry(const void *dptr, uint16_t len)
{
    struct CborParser cbor_parser;
    struct CborValue cbor_value;
    struct cbor_buf_reader cbor_buf_reader;

    cbor_buf_reader_init(&cbor_buf_reader, dptr, len);
    cbor_parser_init(&cbor_buf_reader.r, 0, &cbor_parser, &cbor_value);
    cbor_value_to_pretty(stdout, &cbor_value);

    console_write("\n", 1);
    return 0;
}

static struct log log_console;

struct log *
log_console_get(void)
{
    return &log_console;
}

#if MYNEWT_VAL(LOG_CONSOLE_PRETTY)

#if MYNEWT_VAL(LOG_CONSOLE_PRETTY_WITH_COLORS)

#define ANSI_CSI               "\x1b["
#define ANSI_SEP               ";"
#define ANSI_CSE               "m"
#define ANSI_COLOR_RESET       ANSI_CSI "0" ANSI_CSE

#define COLOR_BLACK             30
#define COLOR_RED               31
#define COLOR_GREEN             32
#define COLOR_YELLOW            33
#define COLOR_BLUE              34
#define COLOR_MAGENTA           35
#define COLOR_CYAN              36
#define COLOR_WHITE             37

#define MOD_COLOR_MIN           COLOR_GREEN
#define MOD_COLOR_MAX           COLOR_CYAN


static void
log_module_color(uint8_t module, char *color_on, char *color_off)
{
    *color_on = 0;
    *color_off = 0;

    if (module) {
        sprintf(color_on, ANSI_CSI "%d" ANSI_SEP "%d" ANSI_CSE,
                module < LOG_MODULE_PERUSER ? MYNEWT_VAL(LOG_COLOR_HILIGHT_MODULE) : 0,
                MOD_COLOR_MIN + module % (MOD_COLOR_MAX - MOD_COLOR_MIN + 1));
        strcpy(color_off, ANSI_COLOR_RESET);
    }
}

#else
#define ANSI_CSI                ""
#define ANSI_SEP                ""
#define ANSI_CSE                ""
#define ANSI_COLOR_RESET        ""
#define log_module_color(hdr, on, off)
#endif

static const uint8_t log_level_color_code[] = {
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_DEBUG),
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_INFO),
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_WARNING),
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_ERROR),
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_CRITICAL),
    /* Add new custom log levels here */
    MYNEWT_VAL(LOG_LEVEL_COLOR_CODE_MAXIMUM),
};

static const char * const log_level_str[] = {
    MYNEWT_VAL(LOG_LEVEL_STRING_DEBUG),
    MYNEWT_VAL(LOG_LEVEL_STRING_INFO),
    MYNEWT_VAL(LOG_LEVEL_STRING_WARNING),
    MYNEWT_VAL(LOG_LEVEL_STRING_ERROR),
    MYNEWT_VAL(LOG_LEVEL_STRING_CRITICAL),
    /* Add new custom log levels here */
    MYNEWT_VAL(LOG_LEVEL_STRING_MAXIMUM),
};

static const int real_log_levels = ARRAY_SIZE(log_level_str) - 1;

void
log_console_print_hdr(const struct log_entry_hdr *hdr)
{
    char module_num[10];
    char image_hash_str[17];
    char level_str_buf[23];
    const char *level_str = "";
    const char *module_name = NULL;
    char color[11] = "";
    char color_off[6] = "";
    uint8_t mapped_log_level;
    uint8_t color_code;

    /* Find module defined in syscfg.logcfg sections */
    module_name = log_module_get_name(hdr->ue_module);

    if (MYNEWT_VAL(LOG_CONSOLE_PRETTY_COLOR_MODULES)) {
        log_module_color(hdr->ue_module, color, color_off);
    }

    if (module_name == NULL) {
        module_name = module_num;
        sprintf(module_num, "mod=%u", hdr->ue_module);
    }
    if (hdr->ue_flags & LOG_FLAGS_IMG_HASH) {
        sprintf(image_hash_str, "[ih=0x%02x%02x%02x%02x]", hdr->ue_imghash[0], hdr->ue_imghash[1],
                hdr->ue_imghash[2], hdr->ue_imghash[3]);
    } else {
        image_hash_str[0] = 0;
    }
    if (hdr->ue_level < real_log_levels || hdr->ue_level == LOG_LEVEL_MAX) {
        mapped_log_level =
            hdr->ue_level < real_log_levels ? hdr->ue_level : real_log_levels;
        color_code = log_level_color_code[mapped_log_level];
        if (MYNEWT_VAL(LOG_CONSOLE_PRETTY_WITH_COLORS)) {
            sprintf(level_str_buf, ANSI_CSI "%d" ANSI_SEP "%d" ANSI_CSE "%s" ANSI_COLOR_RESET,
                color_code >= 10 ? MYNEWT_VAL(LOG_COLOR_HILIGHT_LEVEL) : 0,
                color_code % 10 + 30,
                log_level_str[mapped_log_level]);
            level_str = level_str_buf;
        } else {
            level_str = log_level_str[mapped_log_level];
        }
    } else {
        sprintf(level_str_buf, "%-3u", hdr->ue_level);
        level_str = level_str_buf;
    }

    if (MYNEWT_VAL(LOG_CONSOLE_PRETTY_WITH_TIMESTAMP) &&
        MYNEWT_VAL(LOG_CONSOLE_PRETTY_TIMESTAMP_CLOCK)) {
        struct os_timeval tv = {
            .tv_usec = (unsigned int)hdr->ue_ts % 1000000,
            .tv_sec = (unsigned int)(hdr->ue_ts / 1000000),
        };
        struct clocktime ct;
        timeval_to_clocktime(&tv, NULL, &ct);
        console_printf("[%d-%02d-%02dT%02d:%02d:%02d.%06u][%s%7s%s][%s]%s ",
                       ct.year, ct.mon, ct.day,
                       ct.hour, ct.min, ct.sec, ct.usec,
                       color, module_name, color_off, level_str, image_hash_str);
    } else if (MYNEWT_VAL(LOG_CONSOLE_PRETTY_WITH_TIMESTAMP)) {
        unsigned int us = (unsigned int)hdr->ue_ts % 1000000;
        unsigned int s = (unsigned int)(hdr->ue_ts / 1000000);
        console_printf("[%u.%06u][%s%7s%s][%s]%s ", s, us, color, module_name, color_off, level_str, image_hash_str);
    } else {
        console_printf("[%s%7s%s][%s]%s ", color, module_name, color_off, level_str, image_hash_str);
    }
}

#else
void
log_console_print_hdr(const struct log_entry_hdr *hdr)
{
    console_printf("[ts=" "%" PRIu64 "us, mod=%u level=%u ",
                   hdr->ue_ts, hdr->ue_module, hdr->ue_level);

    if (hdr->ue_flags & LOG_FLAGS_IMG_HASH) {
        console_printf("ih=0x%02x%02x%02x%02x", hdr->ue_imghash[0], hdr->ue_imghash[1],
                       hdr->ue_imghash[2], hdr->ue_imghash[3]);
    }
    console_printf("]");
}
#endif

static int
log_console_append_body(struct log *log, const struct log_entry_hdr *hdr,
                        const void *body, int body_len)
{
    if (!console_is_init()) {
        return (0);
    }

    if (!console_is_midline) {
        log_console_print_hdr(hdr);
    }

    if (hdr->ue_etype != LOG_ETYPE_CBOR) {
        console_write(body, body_len);
        if (((const uint8_t *)body)[body_len - 1] != '\n') {
            console_out('\n');
        }
    } else {
        log_console_dump_cbor_entry(body, body_len);
    }
    return (0);
}

static int
log_console_append(struct log *log, void *buf, int len)
{
    int hdr_len;

    hdr_len = log_hdr_len(buf);

    return log_console_append_body(log, buf, (uint8_t *)buf + hdr_len,
                                   len - hdr_len);
}

static int
log_console_read(struct log *log, const void *dptr, void *buf, uint16_t offset,
        uint16_t len)
{
    /* You don't read console, console read you */
    return (OS_EINVAL);
}

static int
log_console_walk(struct log *log, log_walk_func_t walk_func,
        struct log_offset *log_offset)
{
    /* You don't walk console, console walk you. */
    return (OS_EINVAL);
}

static int
log_console_flush(struct log *log)
{
    /* You don't flush console, console flush you. */
    return (OS_EINVAL);
}

const struct log_handler log_console_handler = {
    .log_type = LOG_TYPE_STREAM,
    .log_read = log_console_read,
    .log_append = log_console_append,
    .log_append_body = log_console_append_body,
    .log_walk = log_console_walk,
    .log_flush = log_console_flush,
};

void
log_console_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = log_register("console", &log_console, &log_console_handler, NULL,
                      MYNEWT_VAL(LOG_LEVEL));
    SYSINIT_PANIC_ASSERT(rc == 0);
}

#endif

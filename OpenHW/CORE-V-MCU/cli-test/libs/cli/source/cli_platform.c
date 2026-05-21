/***************************************************************************/
/* Copyright 2020 QuickLogic Corporation
 * Copyright (C) 2026 Eclipse ThreadX contributors
 *
 * Derived from core-v-mcu-cli-test (https://github.com/openhwgroup/core-v-mcu-cli-test)
 * Original work licensed under the Apache License, Version 2.0.
 * Modifications licensed under MIT (https://opensource.org/licenses/MIT).
 *
 * AI Disclosure: Some portions generated or modified by Copilot (Sonnet 4.6).
 *
 * SPDX-License-Identifier: Apache-2.0 AND MIT
 ***************************************************************************/

#include "libs/cli/include/cli.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SDKConfig.h"
#include "uart_driver.h"
#include <string.h>

#define CLI_PRINTF_BASE_10      10U
#define CLI_PRINTF_BASE_16      16U
#define CLI_PRINTF_FLOAT_SCALE  1000000UL
#define CLI_PRINTF_TMP_LEN      48U
#define CLI_KEY_SEQUENCE_DELAY  50
#define CLI_TASK_STACK_MULTIPLIER 14U
#define CLI_BANNER_DELAY_MS     100U
#define CLI_RX_TIMEOUT_MS       10000

struct cli CLI_common;
static TaskHandle_t cli_task_handle;

/* Forward declaration for the command dispatch entry point. */
void CLI_dispatch(void);

/* Convert milliseconds to scheduler ticks with round-up semantics. */
static TickType_t cli_ms_to_ticks(int timeout_ms)
{
    TickType_t ticks;
    uint32_t scaled;

    if (timeout_ms <= 0)
    {
        ticks = 0U;
    }
    else
    {
        scaled = ((uint32_t)timeout_ms * (uint32_t)configTICK_RATE_HZ) + 999U;
        ticks = (TickType_t)(scaled / 1000U);
        if (ticks == 0U)
        {
            ticks = 1U;
        }
    }

    return ticks;
}

/* Return the current CLI timebase in milliseconds. */
uint32_t CLI_time_now(void)
{
    return ((uint32_t)xTaskGetTickCount() * 1000U) / (uint32_t)configTICK_RATE_HZ;
}

/* Start a lightweight timer token. */
intptr_t CLI_timeout_start(void)
{
    return (intptr_t)CLI_time_now();
}

/* Return non-zero when the supplied timeout has elapsed. */
int CLI_timeout_expired(intptr_t token, int timeout)
{
    uint32_t now;
    uint32_t start;
    uint32_t delta;

    if (timeout == 0)
    {
        return 1;
    }
    if (timeout < 0)
    {
        return 0;
    }

    now = CLI_time_now();
    start = (uint32_t)token;
    delta = now - start;

    return (delta > (uint32_t)timeout) ? 1 : 0;
}

/* Return the remaining time for a CLI timeout token. */
int CLI_timeout_remain(intptr_t token)
{
    (void)token;
    return 0;
}

/* Start the lightweight timer used by the copied standard wait command. */
intptr_t ql_lw_timer_start(void)
{
    return CLI_timeout_start();
}

/* Return the remaining timeout, in milliseconds, for the wait command. */
int ql_lw_timer_remain(intptr_t token, int timeout_ms)
{
    uint32_t now;
    uint32_t start;
    uint32_t elapsed;
    uint32_t timeout_u32;

    if (timeout_ms <= 0)
    {
        return 0;
    }

    now = CLI_time_now();
    start = (uint32_t)token;
    elapsed = now - start;
    timeout_u32 = (uint32_t)timeout_ms;

    return (elapsed >= timeout_u32) ? 0 : (int)(timeout_u32 - elapsed);
}

/* Sleep for the requested number of milliseconds. */
void CLI_sleep(int nMSecs)
{
    TickType_t ticks;

    ticks = cli_ms_to_ticks(nMSecs);
    if (ticks != 0U)
    {
        vTaskDelay(ticks);
    }
}

/* Sleep for a microsecond delay rounded up to the scheduler tick. */
void CLI_sleep_uSecs(int nUSecs)
{
    int timeout_ms;

    timeout_ms = (nUSecs <= 0) ? 0 : ((nUSecs + 999) / 1000);
    CLI_sleep(timeout_ms);
}

/* Ring the CLI bell. */
void CLI_beep(void)
{
    CLI_putc(0x07);
}

/* Write a single byte to the UART without any translation. */
void CLI_putc_raw(int c)
{
    uart_write_byte((uint8_t)UART_ID_CONSOLE, (uint8_t)c);
}

/* Poll the UART for a single byte. */
int CLI_getkey_raw(int timeout_ms)
{
    intptr_t tstart;
    int ch;

    tstart = CLI_timeout_start();
    for (;;)
    {
        ch = uart_read_byte((uint8_t)UART_ID_CONSOLE);
        if (ch >= 0)
        {
            return ch;
        }

        if (CLI_timeout_expired(tstart, timeout_ms) != 0)
        {
            break;
        }

        vTaskDelay(1U);
    }

    return EOF;
}

/* Return the string length while tolerating a null pointer. */
static size_t cli_strlen_safe(const char *s)
{
    size_t len;

    len = 0U;
    if (s != NULL)
    {
        while (s[len] != '\0')
        {
            ++len;
        }
    }

    return len;
}

/* Print padding characters. */
static void cli_print_padding(char pad, int count)
{
    int i;

    for (i = 0; i < count; ++i)
    {
        CLI_putc((int)pad);
    }
}

/* Convert an unsigned integer to text in the requested base. */
static size_t cli_format_unsigned(char *buf, size_t buf_len, unsigned long long value,
                                  unsigned int base, int uppercase)
{
    static const char digits_l[] = "0123456789abcdef";
    static const char digits_u[] = "0123456789ABCDEF";
    char temp[CLI_PRINTF_TMP_LEN];
    const char *digits;
    size_t count;
    size_t i;

    digits = (uppercase != 0) ? digits_u : digits_l;
    count = 0U;

    if ((buf == NULL) || (buf_len == 0U))
    {
        return 0U;
    }

    do
    {
        temp[count] = digits[value % base];
        value /= base;
        ++count;
    } while ((value != 0U) && (count < sizeof(temp)));

    if (count >= buf_len)
    {
        count = buf_len - 1U;
    }

    for (i = 0U; i < count; ++i)
    {
        buf[i] = temp[count - 1U - i];
    }
    buf[count] = '\0';

    return count;
}

/* Print a string field with alignment support. */
static void cli_print_string_field(const char *s, int width, int left_justify, char pad)
{
    size_t len;
    int pad_count;
    size_t i;
    const char *text;

    text = (s == NULL) ? "(null)" : s;
    len = cli_strlen_safe(text);
    pad_count = (width > (int)len) ? (width - (int)len) : 0;

    if (left_justify == 0)
    {
        cli_print_padding(pad, pad_count);
    }

    for (i = 0U; i < len; ++i)
    {
        CLI_putc((int)text[i]);
    }

    if (left_justify != 0)
    {
        cli_print_padding(' ', pad_count);
    }
}

/* Print an integer field with sign, width and padding support. */
static void cli_print_integer_field(unsigned long long magnitude, int negative,
                                    unsigned int base, int width, int left_justify,
                                    int zero_pad, int uppercase)
{
    char digits[CLI_PRINTF_TMP_LEN];
    size_t digit_len;
    int total_len;
    int pad_count;
    char pad_char;

    digit_len = cli_format_unsigned(digits, sizeof(digits), magnitude, base, uppercase);
    total_len = (int)digit_len + ((negative != 0) ? 1 : 0);
    pad_count = (width > total_len) ? (width - total_len) : 0;
    pad_char = ((zero_pad != 0) && (left_justify == 0)) ? '0' : ' ';

    if ((left_justify == 0) && (pad_char == ' '))
    {
        cli_print_padding(pad_char, pad_count);
    }

    if (negative != 0)
    {
        CLI_putc('-');
    }

    if ((left_justify == 0) && (pad_char == '0'))
    {
        cli_print_padding(pad_char, pad_count);
    }

    cli_print_string_field(digits, 0, 0, ' ');

    if (left_justify != 0)
    {
        cli_print_padding(' ', pad_count);
    }
}

/* Print a floating-point value with six decimal places. */
static void cli_print_float_field(double value, int width, int left_justify)
{
    char int_digits[CLI_PRINTF_TMP_LEN];
    char frac_digits[CLI_PRINTF_TMP_LEN];
    unsigned long long int_part;
    unsigned long frac_part;
    unsigned long long scaled_int_part;
    unsigned long long temp_int;
    int negative;
    int total_len;
    int pad_count;

    negative = 0;
    if (value < 0.0)
    {
        negative = 1;
        value = -value;
    }

    int_part = (unsigned long long)value;
    frac_part = (unsigned long)((value - (double)int_part) * (double)CLI_PRINTF_FLOAT_SCALE + 0.5);
    if (frac_part >= CLI_PRINTF_FLOAT_SCALE)
    {
        ++int_part;
        frac_part -= CLI_PRINTF_FLOAT_SCALE;
    }

    (void)cli_format_unsigned(int_digits, sizeof(int_digits), int_part, CLI_PRINTF_BASE_10, 0);
    scaled_int_part = (unsigned long long)frac_part;
    (void)cli_format_unsigned(frac_digits, sizeof(frac_digits), scaled_int_part, CLI_PRINTF_BASE_10, 0);

    total_len = (int)cli_strlen_safe(int_digits) + 1 + (int)cli_strlen_safe(frac_digits) + ((negative != 0) ? 1 : 0);
    pad_count = (width > total_len) ? (width - total_len) : 0;

    if (left_justify == 0)
    {
        cli_print_padding(' ', pad_count);
    }

    if (negative != 0)
    {
        CLI_putc('-');
    }
    cli_print_string_field(int_digits, 0, 0, ' ');
    CLI_putc('.');

    temp_int = (unsigned long long)frac_part;
    while (temp_int < (CLI_PRINTF_FLOAT_SCALE / 10UL))
    {
        CLI_putc('0');
        temp_int *= 10UL;
        if (temp_int == 0U)
        {
            break;
        }
    }
    cli_print_string_field(frac_digits, 0, 0, ' ');

    if (left_justify != 0)
    {
        cli_print_padding(' ', pad_count);
    }
}

/* Print a formatted integer argument according to the parsed length modifier. */
static void cli_handle_signed_format(va_list ap, int length_mod, int width,
                                     int left_justify, int zero_pad)
{
    long long value;
    unsigned long long magnitude;
    int negative;

    if (length_mod >= 2)
    {
        value = va_arg(ap, long long);
    }
    else if (length_mod == 1)
    {
        value = va_arg(ap, long);
    }
    else
    {
        value = va_arg(ap, int);
    }

    negative = (value < 0) ? 1 : 0;
    if (negative != 0)
    {
        magnitude = (unsigned long long)(-(value + 1LL)) + 1ULL;
    }
    else
    {
        magnitude = (unsigned long long)value;
    }

    cli_print_integer_field(magnitude, negative, CLI_PRINTF_BASE_10, width,
                            left_justify, zero_pad, 0);
}

/* Print a formatted unsigned integer argument according to the parsed length modifier. */
static void cli_handle_unsigned_format(va_list ap, int length_mod, int width,
                                       int left_justify, int zero_pad, unsigned int base,
                                       int uppercase)
{
    unsigned long long value;

    if (length_mod >= 2)
    {
        value = va_arg(ap, unsigned long long);
    }
    else if (length_mod == 1)
    {
        value = va_arg(ap, unsigned long);
    }
    else
    {
        value = va_arg(ap, unsigned int);
    }

    cli_print_integer_field(value, 0, base, width, left_justify, zero_pad, uppercase);
}

/* Write a VT100 timestamp column when enabled. */
static void CLI_col0_timestamp(void)
{
    uint32_t now;
    uint32_t divisor;
    uint32_t dividend;
    int zero;

    if (CLI_common.timestamps == 0)
    {
        return;
    }

    now = CLI_time_now();
    divisor = 100000000U;
    zero = ' ';

    while (divisor != 0U)
    {
        if (divisor == 1000U)
        {
            zero = '0';
        }
        if (divisor == 100U)
        {
            CLI_putc_raw('.');
        }
        dividend = now / divisor;
        if (dividend == 0U)
        {
            CLI_putc_raw(zero);
        }
        else
        {
            CLI_putc_raw((int)('0' + dividend));
            zero = '0';
        }
        now -= dividend * divisor;
        divisor /= 10U;
    }
    CLI_putc_raw(' ');
    CLI_putc_raw('|');
    CLI_putc_raw(' ');
}

/* Write a character with CLI newline and tab handling. */
void CLI_putc(int c)
{
    if (c == '\t')
    {
        do
        {
            CLI_putc(' ');
        } while ((CLI_common.col_num & 3) != 0);
        return;
    }

    if (CLI_common.col_num == 0)
    {
        CLI_col0_timestamp();
    }

    if (c == '\n')
    {
        CLI_putc_raw('\r');
        CLI_common.col_num = 0;
    }
    else
    {
        CLI_common.col_num += 1;
    }

    CLI_putc_raw(c);
}

/* Write a string without appending a newline. */
void CLI_puts_no_nl(const char *s)
{
    while ((s != NULL) && (*s != '\0'))
    {
        CLI_putc((int)*s);
        ++s;
    }
}

/* Write a string followed by a newline. */
void CLI_puts(const char *s)
{
    CLI_puts_no_nl(s);
    CLI_putc('\n');
}

/* Print formatted text to the CLI console. */
void CLI_vprintf(const char *fmt, va_list ap)
{
    int width;
    int left_justify;
    int zero_pad;
    int length_mod;
    int parsing;
    int ch;

    while ((fmt != NULL) && (*fmt != '\0'))
    {
        if (*fmt != '%')
        {
            CLI_putc((int)*fmt);
            ++fmt;
            continue;
        }

        ++fmt;
        if (*fmt == '%')
        {
            CLI_putc('%');
            ++fmt;
            continue;
        }

        width = 0;
        left_justify = 0;
        zero_pad = 0;
        length_mod = 0;
        parsing = 1;

        while ((parsing != 0) && (*fmt != '\0'))
        {
            switch (*fmt)
            {
                case '-':
                    left_justify = 1;
                    ++fmt;
                    break;
                case '0':
                    zero_pad = 1;
                    ++fmt;
                    break;
                default:
                    parsing = 0;
                    break;
            }
        }

        if (*fmt == '*')
        {
            width = va_arg(ap, int);
            if (width < 0)
            {
                left_justify = 1;
                width = -width;
            }
            ++fmt;
        }
        else
        {
            while ((*fmt >= '0') && (*fmt <= '9'))
            {
                width = (width * 10) + ((int)*fmt - (int)'0');
                ++fmt;
            }
        }

        while (*fmt == 'l')
        {
            ++length_mod;
            ++fmt;
        }

        switch (*fmt)
        {
            case 'c':
                ch = va_arg(ap, int);
                cli_print_string_field((const char[2]){(char)ch, '\0'}, width, left_justify, ' ');
                break;
            case 's':
                cli_print_string_field(va_arg(ap, const char *), width, left_justify, ' ');
                break;
            case 'd':
            case 'i':
                cli_handle_signed_format(ap, length_mod, width, left_justify, zero_pad);
                break;
            case 'u':
                cli_handle_unsigned_format(ap, length_mod, width, left_justify, zero_pad,
                                           CLI_PRINTF_BASE_10, 0);
                break;
            case 'x':
                cli_handle_unsigned_format(ap, length_mod, width, left_justify, zero_pad,
                                           CLI_PRINTF_BASE_16, 0);
                break;
            case 'X':
                cli_handle_unsigned_format(ap, length_mod, width, left_justify, zero_pad,
                                           CLI_PRINTF_BASE_16, 1);
                break;
            case 'p':
                CLI_puts_no_nl("0x");
                cli_print_integer_field((unsigned long long)(uintptr_t)va_arg(ap, void *), 0,
                                        CLI_PRINTF_BASE_16, width, left_justify, 1, 0);
                break;
            case 'f':
                cli_print_float_field(va_arg(ap, double), width, left_justify);
                break;
            case '\0':
                return;
            default:
                CLI_putc('%');
                CLI_putc((int)*fmt);
                break;
        }

        if (*fmt == '\0')
        {
            break;
        }
        ++fmt;
    }
}

/* Variadic CLI printf wrapper. */
void CLI_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    CLI_vprintf(fmt, ap);
    va_end(ap);
}

/* Read a byte while filtering out spuriously received NUL characters. */
static int get_key_ignore_null(int timeout)
{
    int result;

    result = CLI_getkey_raw(timeout);
    if (result == EOF)
    {
        return result;
    }

    return (result == 0) ? 0x7fffffff : result;
}

/* Append a key code to the internal decode buffer. */
static void append_key(int k)
{
    int x;

    for (x = 0; x < (CLI_MAX_UNGET - 1); ++x)
    {
        if (CLI_common.key_decode_buf[x] == 0)
        {
            CLI_common.key_decode_buf[x] = k;
            break;
        }
    }
    CLI_common.key_decode_buf[CLI_MAX_UNGET - 1] = 0;
}

/* Parse a decimal value embedded in an ANSI escape sequence. */
static int cli_sequence_atoi(const int32_t *ip, int *idx)
{
    int v;
    int ch;
    int digits;

    v = 0;
    digits = 0;
    for (;;)
    {
        ch = (int)ip[*idx];
        if ((ch >= '0') && (ch <= '9'))
        {
            v = (v * 10) + ch - '0';
            *idx += 1;
            ++digits;
        }
        else
        {
            break;
        }
    }

    return (digits == 0) ? -1 : v;
}

/* Decode a buffered VT100 escape sequence into a single key code. */
static void decode_sequence(void)
{
    int idx;
    int groups;
    int tmp;
    int result;
    int done;

    if (CLI_common.key_decode_buf[0] != 0x1b)
    {
        return;
    }

    done = 0;
    result = 0;

    if (CLI_common.key_decode_buf[1] == 'O')
    {
        if ((CLI_common.key_decode_buf[2] >= 'P') && (CLI_common.key_decode_buf[2] <= 'S'))
        {
            result = _KEYCODE32('P', 'F', CLI_common.key_decode_buf[1], CLI_common.key_decode_buf[2]);
            done = 1;
        }
    }
    else if (CLI_common.key_decode_buf[1] == '[')
    {
        result = '[';
        idx = 2;
        groups = 0;

        while (groups < 3)
        {
            if (idx >= (CLI_MAX_UNGET - 1))
            {
                return;
            }

            tmp = cli_sequence_atoi(&CLI_common.key_decode_buf[0], &idx);
            if (tmp < 0)
            {
                break;
            }

            ++groups;
            result = (result * 256) + tmp;
            if (CLI_common.key_decode_buf[idx] == ';')
            {
                ++idx;
            }
            else
            {
                break;
            }
        }

        result = (result * 256) + CLI_common.key_decode_buf[idx];
        if (groups > 1)
        {
            return;
        }
        done = 1;
    }
    else
    {
        return;
    }

    if (done != 0)
    {
        (void)memset((void *)&CLI_common.key_decode_buf, 0, sizeof(CLI_common.key_decode_buf));
        CLI_common.key_decode_buf[0] = result;
    }
}

/* Push a key back onto the decode buffer. */
void CLI_ungetkey(int key)
{
    if ((key != 0) && (key != EOF))
    {
        append_key(key);
    }
}

/* Peek at the next key, decoding VT100 escape sequences as required. */
int CLI_getkey_peek(int mSec_delay)
{
    int result;

    result = CLI_common.key_decode_buf[0];
    if (result != 0)
    {
        return result;
    }

    result = get_key_ignore_null(mSec_delay);
    if (result == EOF)
    {
        return EOF;
    }

    CLI_common.key_decode_buf[0] = result;
    CLI_common.key_decode_buf[1] = 0;

    if (result != 0x1b)
    {
        return result;
    }

    result = get_key_ignore_null(CLI_KEY_SEQUENCE_DELAY);
    if (result == EOF)
    {
        return CLI_common.key_decode_buf[0];
    }
    append_key(result);

    if (result == 'O')
    {
        result = get_key_ignore_null(CLI_KEY_SEQUENCE_DELAY);
        if (result != EOF)
        {
            append_key(result);
            decode_sequence();
        }
        return CLI_common.key_decode_buf[0];
    }

    if (result != '[')
    {
        return CLI_common.key_decode_buf[0];
    }

    result = get_key_ignore_null(CLI_KEY_SEQUENCE_DELAY);
    while ((result >= 0x30) && (result <= 0x3f))
    {
        append_key(result);
        result = get_key_ignore_null(CLI_KEY_SEQUENCE_DELAY);
    }

    while ((result >= 0x20) && (result < 0x2f))
    {
        append_key(result);
        result = get_key_ignore_null(CLI_KEY_SEQUENCE_DELAY);
    }

    if ((result >= 0x40) && (result <= 0x7e))
    {
        append_key(result);
        decode_sequence();
    }

    return CLI_common.key_decode_buf[0];
}

/* Fetch and remove the next key from the decode buffer. */
int CLI_getkey(int nMsec_delay)
{
    int r;
    int x;

    r = CLI_getkey_peek(nMsec_delay);
    if (r == EOF)
    {
        return EOF;
    }

    r = CLI_common.key_decode_buf[0];
    for (x = 0; x < (CLI_MAX_UNGET - 1); ++x)
    {
        CLI_common.key_decode_buf[x] = CLI_common.key_decode_buf[x + 1];
    }
    CLI_common.key_decode_buf[CLI_MAX_UNGET - 1] = 0;

    return r;
}

/* Main CLI thread that reads bytes and dispatches commands. */
static void CLI_task(void *pParameter)
{
    int k;

    (void)pParameter;
    CLI_sleep((int)CLI_BANNER_DELAY_MS);

    CLI_common.timestamps = 0;
    CLI_printf("#*******************\n");
    CLI_printf("Command Line Interface\n");
    CLI_printf("%s %s\n", __DATE__, __TIME__);
    CLI_printf("CORE-V MCU ThreadX CLI Demo\n");
    CLI_printf("#*******************\n");
    CLI_print_prompt();

    for (;;)
    {
        k = CLI_getkey(CLI_RX_TIMEOUT_MS);
        if (k != EOF)
        {
            CLI_rx_byte(k);
        }
    }
}

/* Initialize the CLI core and spawn the CLI task through the compatibility layer. */
void CLI_start_task(const struct cli_cmd_entry *pMainMenu)
{
    BaseType_t status;

    CLI_init(pMainMenu);
    status = xTaskCreate(CLI_task, "CLI", CLI_TASK_STACK_MULTIPLIER * CLI_TASK_STACKSIZE,
                         NULL, (UBaseType_t)(tskIDLE_PRIORITY + 2U), &cli_task_handle);
    if (status != pdPASS)
    {
        for (;;)
        {
        }
    }
}

/***************************************************************************
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/
#include "powertrain_display.h"

#include "jsmn.h"
#include "ssd1306.h"

#include <ctype.h>
#include <string.h>

static void draw_heading(void)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(2, 0);
    ssd1306_WriteString("Powertrain mode", Font_7x10, White);
}

void powertrain_display_waiting(void)
{
    draw_heading();
    ssd1306_SetCursor(2, 26);
    ssd1306_WriteString("Waiting for mode", Font_7x10, White);
    ssd1306_UpdateScreen();
}

static void draw_mode(const char *mode, size_t length)
{
    /* Short names, including "Performance", fit on one large-text line.
     * Longer names wrap in the smaller font, with ellipsis if necessary. */
    FontDef font = length <= (SSD1306_WIDTH - 4) / Font_11x18.FontWidth
                       ? Font_11x18 : Font_7x10;
    size_t columns = (SSD1306_WIDTH - 4) / font.FontWidth;
    size_t capacity = columns * ((SSD1306_HEIGHT - 22) / font.FontHeight);
    size_t visible = length < capacity ? length : capacity;

    draw_heading();
    for (size_t i = 0; i < visible; ++i)
    {
        if (i % columns == 0)
        {
            ssd1306_SetCursor(2, 22 + (i / columns) * font.FontHeight);
        }
        unsigned char ch = (unsigned char)mode[i];
        if (length > capacity && i >= capacity - 3)
        {
            ch = '.';
        }
        /* The OLED font contains printable ASCII only. */
        ssd1306_WriteChar(ch >= 32 && ch <= 126 ? (char)ch : '?', font, White);
    }
    ssd1306_UpdateScreen();
}

bool powertrain_display_update(const char *payload, size_t payload_length)
{
    jsmn_parser parser;
    jsmntok_t tokens[16];

    if (payload == NULL || payload_length == 0)
    {
        return false;
    }

    jsmn_init(&parser);
    int count = jsmn_parse(&parser, payload, payload_length, tokens,
                         sizeof(tokens) / sizeof(tokens[0]));
    if (count < 3 || tokens[0].type != JSMN_OBJECT ||
        tokens[count - 1].end > tokens[0].end)
    {
        return false;
    }

    /* Walk top-level key/value pairs, skipping nested metadata. */
    for (int i = 1; i + 1 < count;)
    {
        const jsmntok_t *key = &tokens[i];
        const jsmntok_t *value = &tokens[i + 1];
        if (key->type != JSMN_STRING || key->size != 1)
        {
            return false;
        }
        if (key->end - key->start == 4 &&
            memcmp(payload + key->start, "Mode", 4) == 0)
        {
            if (value->type != JSMN_STRING)
            {
                return false;
            }
            const char *mode = payload + value->start;
            size_t length = (size_t)(value->end - value->start);
            while (length > 0 && isspace((unsigned char)*mode))
            {
                ++mode;
                --length;
            }
            while (length > 0 && isspace((unsigned char)mode[length - 1]))
            {
                --length;
            }
            if (length == 0)
            {
                return false;
            }
            draw_mode(mode, length);
            return true;
        }

        int value_end = value->end;
        i += 2;
        while (i < count && tokens[i].start < value_end)
        {
            ++i;
        }
    }
    return false;
}

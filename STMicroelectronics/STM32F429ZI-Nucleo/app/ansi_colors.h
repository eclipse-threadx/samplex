/* 
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors:
 *     Ali Eissa - 2026 version.
 */

#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

/* ANSI Terminal Escape Codes for Colored Serial Output */
#define ANSI_RESET          "\x1b[0m"
#define ANSI_BOLD           "\x1b[1m"

/* Standard Primary Colors for Banners */
#define ANSI_RED            "\x1b[31m"
#define ANSI_GREEN          "\x1b[32m"
#define ANSI_YELLOW         "\x1b[33m"
#define ANSI_BLUE           "\x1b[34m"
#define ANSI_MAGENTA        "\x1b[35m"
#define ANSI_CYAN           "\x1b[36m"
#define ANSI_WHITE          "\x1b[37m"

/* Subsystem Tags: Muted Slate Gray (256-color 243) to push them into visual background */
#define TAG_SYSTEM          "\x1b[38;5;243m[System]"
#define TAG_HAL             "\x1b[38;5;243m[HAL]"
#define TAG_SENSOR          "\x1b[38;5;243m[Sensor Thread]"
#define TAG_LOGGER          "\x1b[38;5;243m[Logger Thread]"
#define TAG_NETWORK         "\x1b[38;5;243m[NetX]"
#define TAG_NET_THREAD      "\x1b[38;5;243m[Network Thread]"
#define TAG_DNS             "\x1b[38;5;243m[DNS]"
#define TAG_MQTT            "\x1b[38;5;243m[MQTT]"
#define TAG_STATUS          "\x1b[38;5;243m[Status Thread]"
#define TAG_ECHO            "\x1b[38;5;243m[Echo]"

/* Message Colors: Soft, pastel feedback colors */
#define MSG_INFO            "\x1b[38;5;250m"  /* Soft White/Gray for normal logs */
#define MSG_SUCCESS         "\x1b[38;5;114m"  /* Soft Pastel Green for success */
#define MSG_WARNING         "\x1b[38;5;215m"  /* Muted Gold/Orange for warnings */
#define MSG_ERROR           "\x1b[38;5;203m"  /* Muted Coral/Red for failures */
#define MSG_METRIC          "\x1b[38;5;111m"  /* Soft Sky Blue for data/measurements */

#endif /* ANSI_COLORS_H */

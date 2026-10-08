/***************************************************************************
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * AI Disclosure: Portions of this file were generated with Codex (GPT-6).
 * The AI-generated portions may be considered public domain (CC0-1.0)
 * and not subject to the project's licence. Human review is required.
 *
 * SPDX-License-Identifier: MIT and CC0-1.0
 **************************************************************************/
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "tx_api.h"

#define MQTT_CLIENT_STACK_SIZE 5120
#define MQTT_THREAD_PRIORTY 2
#define MQTT_KEEP_ALIVE_TIMER 300
void mqtt_thread_entry(ULONG thread_input);

#endif

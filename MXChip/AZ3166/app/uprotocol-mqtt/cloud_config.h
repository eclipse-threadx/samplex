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

#ifndef _CLOUD_CONFIG_H
#define _CLOUD_CONFIG_H

#include "nx_api.h"

typedef enum
{
    None         = 0,
    WEP          = 1,
    WPA_PSK_TKIP = 2,
    WPA2_PSK_AES = 3
} WiFi_Mode;

// ----------------------------------------------------------------------------
// WiFi connection config
// ----------------------------------------------------------------------------
#define HOSTNAME      "eclipse-threadx"  //Change to unique hostname.
#define WIFI_SSID     "SDV-imx95-A"
#define WIFI_PASSWORD "eclipsesdv"
#define WIFI_MODE     WPA2_PSK_AES

// ----------------------------------------------------------------------------
// MQTT Config
// ----------------------------------------------------------------------------
#define MQTT_CLIENT_NAME     "ThreadXAZ3166" //Change to unique name.
// imx95-a.local: use its IPv4 address directly; no username or password required.
#define MQTT_LOCAL_BROKER_IP (IP_ADDRESS(192, 168, 60, 1))
#define MQTT_BROKER_PORT      1883
#define MQTT_SUBSCRIBE_TOPIC "vehicle/powertrain/mode"
#define MQTT_PUBLISH_TOPIC   MQTT_CLIENT_NAME "/telemetry"

// ----------------------------------------------------------------------------
// MQTT Support infrastructure
// ----------------------------------------------------------------------------
// The OLED and telemetry sensors share the same I2C bus.
extern TX_MUTEX mqtt_i2c_mutex;
extern TX_EVENT_FLAGS_GROUP mqtt_app_flag;
#define MQTT_RECEIVE_EVENT 1
#define MQTT_MESSAGE_READY 2
#define MQTT_ALL_EVENTS    3


#endif // _CLOUD_CONFIG_H

/***************************************************************************
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/
#include "sensor.h"
#include "tx_api.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifndef _TELEMETRY_H
#define _TELEMETRY_H

#define TELEMETRY_BUFFER_SIZE 256

// Log telemetry if needed
//#define LOG_TELEMETRY

// Sensor data
typedef struct{
    float pressure_hPa;
    float temperature_degC;
    float humidity_perc;
    float acceleration_mg[3];
    float magnetic_mG[3];
} sensor_data;

void telemetry_thread_entry(ULONG parameter);
void get_current_telemetry_string(char* output);

#endif // _TELEMETRY_H

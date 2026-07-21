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

#include "sensor.h"
#include "hts221_reg.h"
#include "board_init.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Private variables */
static uint8_t mock_mode_active = 0;
static uint8_t whoamI = 0;

/* Calibration coefficients for HTS221 */
typedef struct {
  float x0;
  float y0;
  float x1;
  float y1;
} lin_t;

static lin_t lin_hum;
static lin_t lin_temp;

/* Linear interpolation helper */
static float linear_interpolation(lin_t *lin, int16_t x)
{
    if (fabsf(lin->x1 - lin->x0) < 0.001f)
    {
        return lin->y0;
    }
    return ((lin->y1 - lin->y0) * x + ((lin->x1 * lin->y0) - (lin->x0 * lin->y1))) / (lin->x1 - lin->x0);
}

/* Platform I2C read/write callbacks */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    /* HTS221 requires setting the MSB (0x80) of the register address for multi-byte writes */
    if (len > 1)
    {
        reg |= 0x80;
    }
    if (HAL_I2C_Mem_Write((I2C_HandleTypeDef*)handle, HTS221_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 100) == HAL_OK)
    {
        return 0;
    }
    return -1;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    /* HTS221 requires setting the MSB (0x80) of the register address for multi-byte reads */
    if (len > 1)
    {
        reg |= 0x80;
    }
    if (HAL_I2C_Mem_Read((I2C_HandleTypeDef*)handle, HTS221_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 100) == HAL_OK)
    {
        return 0;
    }
    return -1;
}

/* Initialize ST device context */
static stmdev_ctx_t dev_ctx = {
    .write_reg = platform_write,
    .read_reg = platform_read,
    .handle = &hi2c1
};

/* Initialize the sensor */
sensor_status_t sensor_init(void)
{
    printf("[Sensor] Initializing I2C sensor...\r\n");

    /* Check if the physical device is responsive on the I2C bus */
    if (HAL_I2C_IsDeviceReady(&hi2c1, HTS221_I2C_ADDRESS, 3, 100) != HAL_OK)
    {
        printf("[Sensor] WARNING: Physical HTS221 not detected on I2C1 (no ack). Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    /* Read WHO_AM_I register */
    whoamI = 0;
    if (hts221_device_id_get(&dev_ctx, &whoamI) != 0 || whoamI != HTS221_ID)
    {
        printf("[Sensor] WARNING: WHO_AM_I failed (got 0x%02X, expected 0x%02X). Switching to MOCK mode.\r\n", whoamI, HTS221_ID);
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    printf("[Sensor] Physical HTS221 detected (WHO_AM_I = 0x%02X).\r\n", whoamI);

    /* Read humidity calibration coefficients */
    if (hts221_hum_adc_point_0_get(&dev_ctx, &lin_hum.x0) != 0)
    {
        printf("[Sensor] WARNING: H0_T0_OUT read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_hum_rh_point_0_get(&dev_ctx, &lin_hum.y0) != 0)
    {
        printf("[Sensor] WARNING: H0_rH read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_hum_adc_point_1_get(&dev_ctx, &lin_hum.x1) != 0)
    {
        printf("[Sensor] WARNING: H1_T0_OUT read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_hum_rh_point_1_get(&dev_ctx, &lin_hum.y1) != 0)
    {
        printf("[Sensor] WARNING: H1_rH read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    /* Read temperature calibration coefficients */
    if (hts221_temp_adc_point_0_get(&dev_ctx, &lin_temp.x0) != 0)
    {
        printf("[Sensor] WARNING: T0_OUT read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_temp_deg_point_0_get(&dev_ctx, &lin_temp.y0) != 0)
    {
        printf("[Sensor] WARNING: T0_degC read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_temp_adc_point_1_get(&dev_ctx, &lin_temp.x1) != 0)
    {
        printf("[Sensor] WARNING: T1_OUT read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }
    if (hts221_temp_deg_point_1_get(&dev_ctx, &lin_temp.y1) != 0)
    {
        printf("[Sensor] WARNING: T1_degC read failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    /* Enable Block Data Update (BDU) - prevents reading partial data */
    if (hts221_block_data_update_set(&dev_ctx, PROPERTY_ENABLE) != 0)
    {
        printf("[Sensor] WARNING: Enable BDU write failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    /* Set Output Data Rate to 1Hz */
    if (hts221_data_rate_set(&dev_ctx, HTS221_ODR_1Hz) != 0)
    {
        printf("[Sensor] WARNING: Set ODR write failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    /* Power on the device */
    if (hts221_power_on_set(&dev_ctx, PROPERTY_ENABLE) != 0)
    {
        printf("[Sensor] WARNING: Power on write failed. Switching to MOCK mode.\r\n");
        mock_mode_active = 1;
        return mock_sensor_init();
    }

    printf("[Sensor] Physical HTS221 initialized successfully.\r\n");
    mock_mode_active = 0;
    return SENSOR_OK;
}

/* Read sensor data */
sensor_status_t sensor_read(float *temp, float *hum)
{
    if (mock_mode_active)
    {
        return mock_sensor_read(temp, hum);
    }

    /* Physical sensor read */
    hts221_status_reg_t status;
    int32_t ret = hts221_status_get(&dev_ctx, &status);
    if (ret != 0)
    {
        return SENSOR_ERROR;
    }

    /* Wait/check if new data is ready */
    if (status.t_da && status.h_da)
    {
        union {
            int16_t i16;
            uint8_t u8[2];
        } raw_data;

        /* Read raw humidity */
        memset(raw_data.u8, 0, sizeof(raw_data.u8));
        if (hts221_humidity_raw_get(&dev_ctx, raw_data.u8) == 0)
        {
            float rh = linear_interpolation(&lin_hum, raw_data.i16);
            if (rh < 0.0f) rh = 0.0f;
            if (rh > 100.0f) rh = 100.0f;
            *hum = rh;
        }
        else
        {
            return SENSOR_ERROR;
        }

        /* Read raw temperature */
        memset(raw_data.u8, 0, sizeof(raw_data.u8));
        if (hts221_temperature_raw_get(&dev_ctx, raw_data.u8) == 0)
        {
            *temp = linear_interpolation(&lin_temp, raw_data.i16);
        }
        else
        {
            return SENSOR_ERROR;
        }

        return SENSOR_OK;
    }

    /* If data wasn't ready yet, return last known values or wait */
    return SENSOR_ERROR;
}

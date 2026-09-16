/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 *    Assisted-by: Google DeepMind Antigravity (Gemini 3.8 Flash)
 */

#include "bsp/selftest.h"
#include "board_config.h"
#include "fsl_common.h"
#include <stddef.h>

typedef struct {
    bsp_selftest_report_fn report;
    void                  *context;
    unsigned               failures;
} selftest_state_t;

static void check(selftest_state_t *state, int passed, const char *message)
{
    if (passed == 0)
    {
        state->failures++;
    }
    state->report(passed, message, state->context);
}

unsigned bsp_self_test(bsp_selftest_report_fn report, void *context)
{
    selftest_state_t state;

    if (report == NULL)
    {
        return 1U;
    }

    state.report   = report;
    state.context  = context;
    state.failures = 0U;

    /* 1. System core clock sanity check */
    check(&state, (SystemCoreClock >= 10000000UL), "Core clock initialized");

    /* 2. Board name verified */
    check(&state, (sizeof(BSP_BOARD_NAME) > 1), "Board identification valid");

    return state.failures;
}

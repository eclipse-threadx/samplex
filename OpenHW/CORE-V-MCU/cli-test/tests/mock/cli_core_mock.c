/***************************************************************************/
/* Copyright (C) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 ***************************************************************************/

#include "cli.h"

void CLI_uint32_required(const char *name, uint32_t *puthere)
{
    (void)name;
    *puthere = 0U;
}

void CLI_submenu_handler(const struct cli_cmd_entry *pEntry)
{
    (void)pEntry;
}

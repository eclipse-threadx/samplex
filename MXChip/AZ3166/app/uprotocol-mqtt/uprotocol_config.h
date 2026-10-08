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

#ifndef UPROTOCOL_CONFIG_H
#define UPROTOCOL_CONFIG_H

/* Demo identities; coordinate these with the powertrain publisher.
 * Telemetry entity/version/authority must match the local identity. */
#ifndef UPROTOCOL_LOCAL_URI
#define UPROTOCOL_LOCAL_URI "up://az3166/AB/1/0"
#endif
#ifndef UPROTOCOL_TELEMETRY_URI
#define UPROTOCOL_TELEMETRY_URI "up://az3166/AB/1/8001"
#endif
#ifndef UPROTOCOL_POWERTRAIN_URI
#define UPROTOCOL_POWERTRAIN_URI "up://powertrain/BC/1/8001"
#endif
#define UPROTOCOL_TELEMETRY_TTL_MS 10000u
#define UPROTOCOL_RECEIVE_SIZE 512u
#define UPROTOCOL_MAXIMUM_PACKET_SIZE 2048u

#endif

// Goodix 5395 driver for libfprint

// Copyright (C) 2022 Anton Turko <anton.turko@proton.me>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#pragma once

#define GOODIX_5395_TIMEOUT_IN_MS 500

//command

#define CMD_GOODIX_5395_PING 0

#define GOODIX_5395_INTERFACE (1)
#define GOODIX_5395_EP_IN (0x81)
#define GOODIX_5395_EP_OUT (0x3)

#define GOODIX_5395_FIRMWARE_VERSION ("GF5288_HTSEC_APP_100(11|20)")

#define GOODIX_5395_PSK_FLAGS (0xbb020003)

#define GOODIX_5395_RESET_NUMBER (5121)

static const FpIdEntry id_table[] = {
    {.vid = 0x27c6, .pid = 0x5395},
    {.vid = 0x27c6, .pid = 0x5385},
    {.vid = 0x27c6, .pid = 0x5335},
};

// Goodix 5395 driver protocol for libfprint

// Copyright (C) 2022 Anton Turko <anton.turko@proton.me>
// Copyright (C) 2022 Juri Sacchetta <jurisacchetta@gmail,com>

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

#include <glib.h>
#include <fp-image.h>

#define GOODIX_EP_OUT_MAX_BUF_SIZE 0x40
#define GOODIX_EP_IN_MAX_BUF_SIZE 0x40

typedef struct __attribute__((__packed__)) _GoodixDevicePack {
    guint8 cmd;
    guint16 length;
} GoodixDevicePack;

typedef struct __attribute__((__packed__)) _GoodixMessage {
    guint8 category;
    guint8 command;
    GByteArray *payload;
} GoodixMessage;

typedef struct __attribute__((__packed__)) _GoodixProductionRead {
    guint8 status;
    guint32 message_read_type;
    guint32 payload_size;
} GoodixProductionRead;

void fpi_goodix_protocol_debug_data(const char *debug_string, guint8 *data, guint32 length);
gchar *fpi_goodix_protocol_data_to_str(guint8 *data, guint32 length);

void fpi_goodix_protocol_encode(GoodixMessage *message, gboolean calc_checksum, gboolean pad_data,
                                guint8 **data, guint32 *data_len);
gboolean fpi_goodix_protocol_decode(guint8 *data, GoodixMessage **message, GError **error);
gboolean fpi_goodix_protocol_check_ack(GoodixMessage *message, GError **error);
int fpi_goodix_protocol_decode_u32(guint8 *data, uint length);
GoodixMessage *fpi_goodix_protocol_create_message(guint8 category, guint8 command, guint8 *payload, guint32 length);
GoodixMessage *fpi_goodix_protocol_create_message_byte_array(guint8 category, guint8 command, GByteArray *payload);
GArray* fpi_goodix_protocol_decode_image(const GByteArray *image);
gboolean fpi_goodix_protocol_verify_otp_hash(const guint8 *otp, guint otp_length, const guint8 otp_hash[]);
void fpi_goodix_protocol_free_message(GoodixMessage *message);
GByteArray *fpi_goodix_protocol_generate_fdt_base(const GByteArray *fdt_data);
void fpi_goodix_protocol_write_pgm(const GArray *image, const guint width, const guint height, const char *path);
FpImage *fpi_goodix_protocol_convert_image(const GSList *raw_images, const GArray *calib_image, const guint width, const guint height);
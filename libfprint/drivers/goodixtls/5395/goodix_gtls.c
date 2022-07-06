// Goodix Tls driver for libfprint

// Copyright (C) 2022 Juri Sacchetta <jurisacchetta@gmail.com>

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
#include "goodix_gtls.h"

#include <glib.h>

#include "goodix_gtls.h"
#include "crypto_utils.h"

GoodixGTLSParams *fpi_goodix_device_gtls_init_params(void) {
    GoodixGTLSParams *params = g_malloc0(sizeof(GoodixGTLSParams));
    params->state = 0;
    params->client_random = g_byte_array_new();
    params->server_random = g_byte_array_new();
    params->server_identity = g_byte_array_new();
    params->symmetric_key = g_byte_array_new();
    params->symmetric_iv = g_byte_array_new();
    params->hmac_key = g_byte_array_new();
    params->psk = g_byte_array_new();
    guchar psk[32] = {0};
    g_byte_array_append(params->psk, psk, 32);
    return params;
}

GByteArray *fpi_goodix_gtls_create_hello_message() {
    GByteArray *res = g_byte_array_new();
    GRand *rand = g_rand_new();
    for (int i = 0; i < 8; i++) {
        guint32 r = g_rand_int(rand);
        g_byte_array_append(res, &r, sizeof(guint32));
    }
    g_rand_free(rand);
    return res;
}

void fpi_goodix_gtls_decode_server_hello(GoodixGTLSParams *params, GByteArray *recv_mcu_payload) {
    g_byte_array_append(params->server_random, recv_mcu_payload->data, 0x20);
    g_byte_array_remove_range(recv_mcu_payload, 0, 0x20);
    g_byte_array_append(params->server_identity, recv_mcu_payload->data, 0x20);
    g_byte_array_free(recv_mcu_payload, TRUE);
}

gboolean fpi_goodix_gtls_derive_key(GoodixGTLSParams *params) {
    GByteArray *random_data = g_byte_array_new();
    g_byte_array_append(random_data, params->client_random->data, params->client_random->len);
    g_byte_array_append(random_data, params->server_random->data, params->server_random->len);
    GByteArray *session_key = crypto_utils_derive_key(params->psk, random_data, 0x44);
    g_byte_array_free(random_data, TRUE);
    
    g_byte_array_append(params->symmetric_key, session_key->data, 0x10);
    g_byte_array_remove_range(session_key, 0, 0x10);
    
    g_byte_array_append(params->symmetric_iv, session_key->data, 0x10);
    g_byte_array_remove_range(session_key, 0, 0x10);
    
    g_byte_array_append(params->hmac_key, session_key->data, 0x20);
    g_byte_array_remove_range(session_key, 0, 0x20);
    
    params->hmac_client_counter_init = (guint16)(session_key->data[0] | session_key->data[1] << 8);
    g_byte_array_remove_range(session_key, 0, 0x2);

    params->hmac_server_counter_init = (guint16)(session_key->data[0] | session_key->data[1] << 8);
    g_byte_array_remove_range(session_key, 0, 0x2);

    g_assert(session_key->len == 0);
    g_byte_array_free(session_key, TRUE);

    GByteArray *random = g_byte_array_new();
    g_byte_array_append(random, params->client_random->data, params->client_random->len);
    g_byte_array_append(random, params->server_random->data, params->server_random->len);
    params->client_identity = crypto_utils_HMAC_SHA256(params->hmac_key, random);

    if (memcmp(params->client_identity->data, params->server_identity->data, params->client_identity->len) != 0
        || params->client_identity->len != params->server_identity->len ) {
        return FALSE;
    }
    return TRUE;
}

GByteArray *fpi_goodix_gtls_decrypt_sensor_data(GoodixGTLSParams *params, GByteArray *encrypted_message, GError **error) {
    guint32 data_type = encrypted_message->data[0] | encrypted_message->data[1] << 0x8 | encrypted_message->data[2] << 0x10 | encrypted_message->data[3] << 0x100;
    if (data_type != 0xAA01) {
        return NULL;
        // TODO  raise Exception("Unexpected data type")
    }
    guint32 msg_length = encrypted_message->data[4] | encrypted_message->data[5] << 0x8 | encrypted_message->data[6] << 0x10 | encrypted_message->data[7] << 0x100;
    if (msg_length != encrypted_message->len) {
        return NULL;
        // TODO  raise Exception("Length mismatch")
    }
    g_byte_array_remove_range(encrypted_message, 0, 8);
    GByteArray *encrypted_payload = g_byte_array_new();
    g_byte_array_append(encrypted_payload, encrypted_message->data, encrypted_message->len - 0x20);

    GByteArray *payload_hmac = g_byte_array_new();
    g_byte_array_append(payload_hmac, &(encrypted_message->data[encrypted_message->len - 0x20]), 0x20);
    // TODO fp_dbg("HMAC for encrypted payload: %s", fpi_goodix_protocol_data_to_str(payload_hmac->data, payload_hmac->len));

    GByteArray *gea_encrypted_data = g_byte_array_new();
    for (size_t block_idx = 0; block_idx < 15; block_idx++) {
        if (block_idx % 2 == 0) {
            if (block_idx == 0) {
                g_byte_array_append(gea_encrypted_data, encrypted_payload->data, 0x3A7);
                g_byte_array_remove_range(encrypted_payload, 0, 0x3A7);
            } else if (block_idx == 14) {
                g_assert(gea_encrypted_data->len == 0x3A7 + 0x3F0 * 13);
                g_byte_array_append(gea_encrypted_data, encrypted_payload->data, encrypted_payload->len);

            } else {
                g_byte_array_append(gea_encrypted_data, encrypted_payload->data, 0x3F0);
                g_byte_array_remove_range(encrypted_payload, 0, 0x3F0);
            }

        } else {
            GByteArray *temp = g_byte_array_new();
            g_byte_array_append(temp, encrypted_payload->data, 0x3F0);
            g_byte_array_remove_range(encrypted_payload, 0, 0x3F0);
            GByteArray *decrypt = crypo_utils_AES_128_cbc_decrypt(temp, params->symmetric_key, params->symmetric_iv, error);
            g_byte_array_free(temp, TRUE);
            g_byte_array_append(gea_encrypted_data, decrypt->data, decrypt->len);
            g_byte_array_free(decrypt, TRUE);
        }
    }
    GByteArray *hmac_data = g_byte_array_new();
    g_byte_array_append(hmac_data, &(params->hmac_server_counter), 4);
    g_byte_array_append(hmac_data, &(gea_encrypted_data->data[gea_encrypted_data->len - 0x400]), 0x400);
    GByteArray *computed_hmac = crypto_utils_HMAC_SHA256(params->hmac_key, hmac_data);
    if (computed_hmac->len != payload_hmac->len || memcmp(computed_hmac->data, payload_hmac->data, computed_hmac->len)) {
        // TODO raise Exception("HMAC verification failed")
        return NULL;
    }
    // TODO fp_dbg("Encrypted payload HMAC verified");
    params->hmac_server_counter = (params->hmac_server_counter + 1) & 0xFFFFFFFF;

    // fp_dbg("HMAC server counter is now: %s", fpi_goodix_protocol_data_to_str(params->hmac_server_counter, 2));

    if (gea_encrypted_data->len < 5) {
        //   TODO      raise Exception("Encrypted payload too short")
        return NULL;
    }
    // The first five bytes are always discarded (alignment?)
    g_byte_array_remove_range(gea_encrypted_data, 0, 5);
    guint8 *ptr = &(gea_encrypted_data->data[gea_encrypted_data->len - 4]);
    guint msg_gea_crc = *ptr << 24 | (ptr)[1] << 16 | ptr[2] << 8 | ptr[3];
    printf("msg_gea_crc: %x", msg_gea_crc);
    msg_gea_crc = ptr[0] * 0x100 + ptr[1] + ptr[2] * 0x1000000 + ptr[3] * 0x10000;
    printf("msg_gea_crc: %x", msg_gea_crc);
    g_byte_array_remove_range(gea_encrypted_data, gea_encrypted_data->len - 4, 4);

    // fp_dbg("GEA data CRC: %s", fpi_goodix_protocol_data_to_str(&msg_gea_crc, 4));
    //  TODO
    guint computed_gea_crc = crypto_utils_crc32_mpeg2_calc((gea_encrypted_data->data), gea_encrypted_data->len);
    if(computed_gea_crc != msg_gea_crc) {
          //          raise Exception("CRC check failed")
          return NULL;
    }
    //      logging.debug("GEA data CRC verified")
    gint32 gea_key = gea_encrypted_data->data[0] | gea_encrypted_data->data[1] << 0x10 | gea_encrypted_data->data[2] << 0x100 | gea_encrypted_data->data[3] << 0x1000;
    // fp_dbg("GEA key: %x", gea_key);
    return ctypto_utils_gea_decrypt(gea_key, gea_encrypted_data);
}
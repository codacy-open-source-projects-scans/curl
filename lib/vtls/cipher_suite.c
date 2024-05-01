/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Jan Venekamp, <jan@venekamp.net>
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
#include "curl_setup.h"

#if defined(USE_MBEDTLS)
#include "cipher_suite.h"
#include "curl_printf.h"
#include "strcase.h"
#include <string.h>

/*
 * To support the CURLOPT_SSL_CIPHER_LIST option on SSL backends
 * that do not support it natively, but do support setting a list of
 * IANA ids, we need a list of all supported cipher suite names
 * (openssl and IANA) to be able to look up the IANA ids.
 *
 * To keep the binary size of this list down we compress each entry
 * down to 2 + 6 bytes using the C preprocessor.
 */

/*
 * mbedTLS NOTE: mbedTLS has mbedtls_ssl_get_ciphersuite_id() to
 * convert a string representation to an IANA id, we do not use that
 * because it does not support "standard" openssl cipher suite
 * names, nor IANA names.
 */

/* NOTE: also see tests/unit/unit3205.c */

/* Text for cipher suite parts (max 64 entries),
   keep indexes below in sync with this! */
static const char *cs_txt =
  "\0"
  "TLS" "\0"
  "WITH" "\0"
  "128" "\0"
  "256" "\0"
  "3DES" "\0"
  "8" "\0"
  "AES" "\0"
  "AES128" "\0"
  "AES256" "\0"
  "CBC" "\0"
  "CBC3" "\0"
  "CCM" "\0"
  "CCM8" "\0"
  "CHACHA20" "\0"
  "DES" "\0"
  "DHE" "\0"
  "ECDH" "\0"
  "ECDHE" "\0"
  "ECDSA" "\0"
  "EDE" "\0"
  "GCM" "\0"
  "MD5" "\0"
  "NULL" "\0"
  "POLY1305" "\0"
  "PSK" "\0"
  "RSA" "\0"
  "SHA" "\0"
  "SHA256" "\0"
  "SHA384" "\0"
  "ARIA" "\0"
  "ARIA128" "\0"
  "ARIA256" "\0"
  "CAMELLIA" "\0"
  "CAMELLIA128" "\0"
  "CAMELLIA256" "\0"
;
/* Indexes of above cs_txt */
enum {
  CS_TXT_IDX_,
  CS_TXT_IDX_TLS,
  CS_TXT_IDX_WITH,
  CS_TXT_IDX_128,
  CS_TXT_IDX_256,
  CS_TXT_IDX_3DES,
  CS_TXT_IDX_8,
  CS_TXT_IDX_AES,
  CS_TXT_IDX_AES128,
  CS_TXT_IDX_AES256,
  CS_TXT_IDX_CBC,
  CS_TXT_IDX_CBC3,
  CS_TXT_IDX_CCM,
  CS_TXT_IDX_CCM8,
  CS_TXT_IDX_CHACHA20,
  CS_TXT_IDX_DES,
  CS_TXT_IDX_DHE,
  CS_TXT_IDX_ECDH,
  CS_TXT_IDX_ECDHE,
  CS_TXT_IDX_ECDSA,
  CS_TXT_IDX_EDE,
  CS_TXT_IDX_GCM,
  CS_TXT_IDX_MD5,
  CS_TXT_IDX_NULL,
  CS_TXT_IDX_POLY1305,
  CS_TXT_IDX_PSK,
  CS_TXT_IDX_RSA,
  CS_TXT_IDX_SHA,
  CS_TXT_IDX_SHA256,
  CS_TXT_IDX_SHA384,
  CS_TXT_IDX_ARIA,
  CS_TXT_IDX_ARIA128,
  CS_TXT_IDX_ARIA256,
  CS_TXT_IDX_CAMELLIA,
  CS_TXT_IDX_CAMELLIA128,
  CS_TXT_IDX_CAMELLIA256,
  CS_TXT_LEN,
};

#define CS_ZIP_IDX(a, b, c, d, e, f, g, h)    \
{                                             \
  (uint8_t) ((a) << 2 | ((b) & 0x3F) >> 4),   \
  (uint8_t) ((b) << 4 | ((c) & 0x3F) >> 2),   \
  (uint8_t) ((c) << 6 | ((d) & 0x3F)),        \
  (uint8_t) ((e) << 2 | ((f) & 0x3F) >> 4),   \
  (uint8_t) ((f) << 4 | ((g) & 0x3F) >> 2),   \
  (uint8_t) ((g) << 6 | ((h) & 0x3F))         \
}
#define CS_ENTRY(id, a, b, c, d, e, f, g, h)  \
{                                             \
  id,                                         \
  CS_ZIP_IDX(                                 \
    CS_TXT_IDX_ ## a, CS_TXT_IDX_ ## b,       \
    CS_TXT_IDX_ ## c, CS_TXT_IDX_ ## d,       \
    CS_TXT_IDX_ ## e, CS_TXT_IDX_ ## f,       \
    CS_TXT_IDX_ ## g, CS_TXT_IDX_ ## h        \
  )                                           \
}

struct cs_entry {
  uint16_t id;
  uint8_t zip[6];
};

/* !checksrc! disable COMMANOSPACE all */
static const struct cs_entry cs_list [] = {
  CS_ENTRY(0x002F, TLS,RSA,WITH,AES,128,CBC,SHA,),
  CS_ENTRY(0x002F, AES128,SHA,,,,,,),
  CS_ENTRY(0x0035, TLS,RSA,WITH,AES,256,CBC,SHA,),
  CS_ENTRY(0x0035, AES256,SHA,,,,,,),
  CS_ENTRY(0x003C, TLS,RSA,WITH,AES,128,CBC,SHA256,),
  CS_ENTRY(0x003C, AES128,SHA256,,,,,,),
  CS_ENTRY(0x003D, TLS,RSA,WITH,AES,256,CBC,SHA256,),
  CS_ENTRY(0x003D, AES256,SHA256,,,,,,),
  CS_ENTRY(0x009C, TLS,RSA,WITH,AES,128,GCM,SHA256,),
  CS_ENTRY(0x009C, AES128,GCM,SHA256,,,,,),
  CS_ENTRY(0x009D, TLS,RSA,WITH,AES,256,GCM,SHA384,),
  CS_ENTRY(0x009D, AES256,GCM,SHA384,,,,,),
  CS_ENTRY(0xC004, TLS,ECDH,ECDSA,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0xC004, ECDH,ECDSA,AES128,SHA,,,,),
  CS_ENTRY(0xC005, TLS,ECDH,ECDSA,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0xC005, ECDH,ECDSA,AES256,SHA,,,,),
  CS_ENTRY(0xC009, TLS,ECDHE,ECDSA,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0xC009, ECDHE,ECDSA,AES128,SHA,,,,),
  CS_ENTRY(0xC00A, TLS,ECDHE,ECDSA,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0xC00A, ECDHE,ECDSA,AES256,SHA,,,,),
  CS_ENTRY(0xC00E, TLS,ECDH,RSA,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0xC00E, ECDH,RSA,AES128,SHA,,,,),
  CS_ENTRY(0xC00F, TLS,ECDH,RSA,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0xC00F, ECDH,RSA,AES256,SHA,,,,),
  CS_ENTRY(0xC013, TLS,ECDHE,RSA,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0xC013, ECDHE,RSA,AES128,SHA,,,,),
  CS_ENTRY(0xC014, TLS,ECDHE,RSA,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0xC014, ECDHE,RSA,AES256,SHA,,,,),
  CS_ENTRY(0xC023, TLS,ECDHE,ECDSA,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0xC023, ECDHE,ECDSA,AES128,SHA256,,,,),
  CS_ENTRY(0xC024, TLS,ECDHE,ECDSA,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0xC024, ECDHE,ECDSA,AES256,SHA384,,,,),
  CS_ENTRY(0xC025, TLS,ECDH,ECDSA,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0xC025, ECDH,ECDSA,AES128,SHA256,,,,),
  CS_ENTRY(0xC026, TLS,ECDH,ECDSA,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0xC026, ECDH,ECDSA,AES256,SHA384,,,,),
  CS_ENTRY(0xC027, TLS,ECDHE,RSA,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0xC027, ECDHE,RSA,AES128,SHA256,,,,),
  CS_ENTRY(0xC028, TLS,ECDHE,RSA,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0xC028, ECDHE,RSA,AES256,SHA384,,,,),
  CS_ENTRY(0xC029, TLS,ECDH,RSA,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0xC029, ECDH,RSA,AES128,SHA256,,,,),
  CS_ENTRY(0xC02A, TLS,ECDH,RSA,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0xC02A, ECDH,RSA,AES256,SHA384,,,,),
  CS_ENTRY(0xC02B, TLS,ECDHE,ECDSA,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0xC02B, ECDHE,ECDSA,AES128,GCM,SHA256,,,),
  CS_ENTRY(0xC02C, TLS,ECDHE,ECDSA,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0xC02C, ECDHE,ECDSA,AES256,GCM,SHA384,,,),
  CS_ENTRY(0xC02D, TLS,ECDH,ECDSA,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0xC02D, ECDH,ECDSA,AES128,GCM,SHA256,,,),
  CS_ENTRY(0xC02E, TLS,ECDH,ECDSA,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0xC02E, ECDH,ECDSA,AES256,GCM,SHA384,,,),
  CS_ENTRY(0xC02F, TLS,ECDHE,RSA,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0xC02F, ECDHE,RSA,AES128,GCM,SHA256,,,),
  CS_ENTRY(0xC030, TLS,ECDHE,RSA,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0xC030, ECDHE,RSA,AES256,GCM,SHA384,,,),
  CS_ENTRY(0xC031, TLS,ECDH,RSA,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0xC031, ECDH,RSA,AES128,GCM,SHA256,,,),
  CS_ENTRY(0xC032, TLS,ECDH,RSA,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0xC032, ECDH,RSA,AES256,GCM,SHA384,,,),
  CS_ENTRY(0xCCA8, TLS,ECDHE,RSA,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCA8, ECDHE,RSA,CHACHA20,POLY1305,,,,),
  CS_ENTRY(0xCCA9, TLS,ECDHE,ECDSA,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCA9, ECDHE,ECDSA,CHACHA20,POLY1305,,,,),

  CS_ENTRY(0x0001, TLS,RSA,WITH,NULL,MD5,,,),
  CS_ENTRY(0x0001, NULL,MD5,,,,,,),
  CS_ENTRY(0x0002, TLS,RSA,WITH,NULL,SHA,,,),
  CS_ENTRY(0x0002, NULL,SHA,,,,,,),
  CS_ENTRY(0x002C, TLS,PSK,WITH,NULL,SHA,,,),
  CS_ENTRY(0x002C, PSK,NULL,SHA,,,,,),
  CS_ENTRY(0x002D, TLS,DHE,PSK,WITH,NULL,SHA,,),
  CS_ENTRY(0x002D, DHE,PSK,NULL,SHA,,,,),
  CS_ENTRY(0x002E, TLS,RSA,PSK,WITH,NULL,SHA,,),
  CS_ENTRY(0x002E, RSA,PSK,NULL,SHA,,,,),
  CS_ENTRY(0x0033, TLS,DHE,RSA,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0x0033, DHE,RSA,AES128,SHA,,,,),
  CS_ENTRY(0x0039, TLS,DHE,RSA,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0x0039, DHE,RSA,AES256,SHA,,,,),
  CS_ENTRY(0x003B, TLS,RSA,WITH,NULL,SHA256,,,),
  CS_ENTRY(0x003B, NULL,SHA256,,,,,,),
  CS_ENTRY(0x0067, TLS,DHE,RSA,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0x0067, DHE,RSA,AES128,SHA256,,,,),
  CS_ENTRY(0x006B, TLS,DHE,RSA,WITH,AES,256,CBC,SHA256),
  CS_ENTRY(0x006B, DHE,RSA,AES256,SHA256,,,,),
  CS_ENTRY(0x008C, TLS,PSK,WITH,AES,128,CBC,SHA,),
  CS_ENTRY(0x008C, PSK,AES128,CBC,SHA,,,,),
  CS_ENTRY(0x008D, TLS,PSK,WITH,AES,256,CBC,SHA,),
  CS_ENTRY(0x008D, PSK,AES256,CBC,SHA,,,,),
  CS_ENTRY(0x0090, TLS,DHE,PSK,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0x0090, DHE,PSK,AES128,CBC,SHA,,,),
  CS_ENTRY(0x0091, TLS,DHE,PSK,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0x0091, DHE,PSK,AES256,CBC,SHA,,,),
  CS_ENTRY(0x0094, TLS,RSA,PSK,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0x0094, RSA,PSK,AES128,CBC,SHA,,,),
  CS_ENTRY(0x0095, TLS,RSA,PSK,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0x0095, RSA,PSK,AES256,CBC,SHA,,,),
  CS_ENTRY(0x009E, TLS,DHE,RSA,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0x009E, DHE,RSA,AES128,GCM,SHA256,,,),
  CS_ENTRY(0x009F, TLS,DHE,RSA,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0x009F, DHE,RSA,AES256,GCM,SHA384,,,),
  CS_ENTRY(0x00A8, TLS,PSK,WITH,AES,128,GCM,SHA256,),
  CS_ENTRY(0x00A8, PSK,AES128,GCM,SHA256,,,,),
  CS_ENTRY(0x00A9, TLS,PSK,WITH,AES,256,GCM,SHA384,),
  CS_ENTRY(0x00A9, PSK,AES256,GCM,SHA384,,,,),
  CS_ENTRY(0x00AA, TLS,DHE,PSK,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0x00AA, DHE,PSK,AES128,GCM,SHA256,,,),
  CS_ENTRY(0x00AB, TLS,DHE,PSK,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0x00AB, DHE,PSK,AES256,GCM,SHA384,,,),
  CS_ENTRY(0x00AC, TLS,RSA,PSK,WITH,AES,128,GCM,SHA256),
  CS_ENTRY(0x00AC, RSA,PSK,AES128,GCM,SHA256,,,),
  CS_ENTRY(0x00AD, TLS,RSA,PSK,WITH,AES,256,GCM,SHA384),
  CS_ENTRY(0x00AD, RSA,PSK,AES256,GCM,SHA384,,,),
  CS_ENTRY(0x00AE, TLS,PSK,WITH,AES,128,CBC,SHA256,),
  CS_ENTRY(0x00AE, PSK,AES128,CBC,SHA256,,,,),
  CS_ENTRY(0x00AF, TLS,PSK,WITH,AES,256,CBC,SHA384,),
  CS_ENTRY(0x00AF, PSK,AES256,CBC,SHA384,,,,),
  CS_ENTRY(0x00B0, TLS,PSK,WITH,NULL,SHA256,,,),
  CS_ENTRY(0x00B0, PSK,NULL,SHA256,,,,,),
  CS_ENTRY(0x00B1, TLS,PSK,WITH,NULL,SHA384,,,),
  CS_ENTRY(0x00B1, PSK,NULL,SHA384,,,,,),
  CS_ENTRY(0x00B2, TLS,DHE,PSK,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0x00B2, DHE,PSK,AES128,CBC,SHA256,,,),
  CS_ENTRY(0x00B3, TLS,DHE,PSK,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0x00B3, DHE,PSK,AES256,CBC,SHA384,,,),
  CS_ENTRY(0x00B4, TLS,DHE,PSK,WITH,NULL,SHA256,,),
  CS_ENTRY(0x00B4, DHE,PSK,NULL,SHA256,,,,),
  CS_ENTRY(0x00B5, TLS,DHE,PSK,WITH,NULL,SHA384,,),
  CS_ENTRY(0x00B5, DHE,PSK,NULL,SHA384,,,,),
  CS_ENTRY(0x00B6, TLS,RSA,PSK,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0x00B6, RSA,PSK,AES128,CBC,SHA256,,,),
  CS_ENTRY(0x00B7, TLS,RSA,PSK,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0x00B7, RSA,PSK,AES256,CBC,SHA384,,,),
  CS_ENTRY(0x00B8, TLS,RSA,PSK,WITH,NULL,SHA256,,),
  CS_ENTRY(0x00B8, RSA,PSK,NULL,SHA256,,,,),
  CS_ENTRY(0x00B9, TLS,RSA,PSK,WITH,NULL,SHA384,,),
  CS_ENTRY(0x00B9, RSA,PSK,NULL,SHA384,,,,),
  CS_ENTRY(0x1301, TLS,AES,128,GCM,SHA256,,,),
  CS_ENTRY(0x1302, TLS,AES,256,GCM,SHA384,,,),
  CS_ENTRY(0x1303, TLS,CHACHA20,POLY1305,SHA256,,,,),
  CS_ENTRY(0x1304, TLS,AES,128,CCM,SHA256,,,),
  CS_ENTRY(0x1305, TLS,AES,128,CCM,8,SHA256,,),
  CS_ENTRY(0xC001, TLS,ECDH,ECDSA,WITH,NULL,SHA,,),
  CS_ENTRY(0xC001, ECDH,ECDSA,NULL,SHA,,,,),
  CS_ENTRY(0xC006, TLS,ECDHE,ECDSA,WITH,NULL,SHA,,),
  CS_ENTRY(0xC006, ECDHE,ECDSA,NULL,SHA,,,,),
  CS_ENTRY(0xC00B, TLS,ECDH,RSA,WITH,NULL,SHA,,),
  CS_ENTRY(0xC00B, ECDH,RSA,NULL,SHA,,,,),
  CS_ENTRY(0xC010, TLS,ECDHE,RSA,WITH,NULL,SHA,,),
  CS_ENTRY(0xC010, ECDHE,RSA,NULL,SHA,,,,),
  CS_ENTRY(0xC035, TLS,ECDHE,PSK,WITH,AES,128,CBC,SHA),
  CS_ENTRY(0xC035, ECDHE,PSK,AES128,CBC,SHA,,,),
  CS_ENTRY(0xC036, TLS,ECDHE,PSK,WITH,AES,256,CBC,SHA),
  CS_ENTRY(0xC036, ECDHE,PSK,AES256,CBC,SHA,,,),
  CS_ENTRY(0xCCAB, TLS,PSK,WITH,CHACHA20,POLY1305,SHA256,,),
  CS_ENTRY(0xCCAB, PSK,CHACHA20,POLY1305,,,,,),

  CS_ENTRY(0xC09C, TLS,RSA,WITH,AES,128,CCM,,),
  CS_ENTRY(0xC09C, AES128,CCM,,,,,,),
  CS_ENTRY(0xC09D, TLS,RSA,WITH,AES,256,CCM,,),
  CS_ENTRY(0xC09D, AES256,CCM,,,,,,),
  CS_ENTRY(0xC0A0, TLS,RSA,WITH,AES,128,CCM,8,),
  CS_ENTRY(0xC0A0, AES128,CCM8,,,,,,),
  CS_ENTRY(0xC0A1, TLS,RSA,WITH,AES,256,CCM,8,),
  CS_ENTRY(0xC0A1, AES256,CCM8,,,,,,),
  CS_ENTRY(0xC0AC, TLS,ECDHE,ECDSA,WITH,AES,128,CCM,),
  CS_ENTRY(0xC0AC, ECDHE,ECDSA,AES128,CCM,,,,),
  CS_ENTRY(0xC0AD, TLS,ECDHE,ECDSA,WITH,AES,256,CCM,),
  CS_ENTRY(0xC0AD, ECDHE,ECDSA,AES256,CCM,,,,),
  CS_ENTRY(0xC0AE, TLS,ECDHE,ECDSA,WITH,AES,128,CCM,8),
  CS_ENTRY(0xC0AE, ECDHE,ECDSA,AES128,CCM8,,,,),
  CS_ENTRY(0xC0AF, TLS,ECDHE,ECDSA,WITH,AES,256,CCM,8),
  CS_ENTRY(0xC0AF, ECDHE,ECDSA,AES256,CCM8,,,,),

  /* entries marked ns are "non-standard", they are not in openssl */
  CS_ENTRY(0x0041, TLS,RSA,WITH,CAMELLIA,128,CBC,SHA,),
  CS_ENTRY(0x0041, CAMELLIA128,SHA,,,,,,),
  CS_ENTRY(0x0045, TLS,DHE,RSA,WITH,CAMELLIA,128,CBC,SHA),
  CS_ENTRY(0x0045, DHE,RSA,CAMELLIA128,SHA,,,,),
  CS_ENTRY(0x0084, TLS,RSA,WITH,CAMELLIA,256,CBC,SHA,),
  CS_ENTRY(0x0084, CAMELLIA256,SHA,,,,,,),
  CS_ENTRY(0x0088, TLS,DHE,RSA,WITH,CAMELLIA,256,CBC,SHA),
  CS_ENTRY(0x0088, DHE,RSA,CAMELLIA256,SHA,,,,),
  CS_ENTRY(0x00BA, TLS,RSA,WITH,CAMELLIA,128,CBC,SHA256,),
  CS_ENTRY(0x00BA, CAMELLIA128,SHA256,,,,,,),
  CS_ENTRY(0x00BE, TLS,DHE,RSA,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0x00BE, DHE,RSA,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0x00C0, TLS,RSA,WITH,CAMELLIA,256,CBC,SHA256,),
  CS_ENTRY(0x00C0, CAMELLIA256,SHA256,,,,,,),
  CS_ENTRY(0x00C4, TLS,DHE,RSA,WITH,CAMELLIA,256,CBC,SHA256),
  CS_ENTRY(0x00C4, DHE,RSA,CAMELLIA256,SHA256,,,,),
  CS_ENTRY(0xC037, TLS,ECDHE,PSK,WITH,AES,128,CBC,SHA256),
  CS_ENTRY(0xC037, ECDHE,PSK,AES128,CBC,SHA256,,,),
  CS_ENTRY(0xC038, TLS,ECDHE,PSK,WITH,AES,256,CBC,SHA384),
  CS_ENTRY(0xC038, ECDHE,PSK,AES256,CBC,SHA384,,,),
  CS_ENTRY(0xC039, TLS,ECDHE,PSK,WITH,NULL,SHA,,),
  CS_ENTRY(0xC039, ECDHE,PSK,NULL,SHA,,,,),
  CS_ENTRY(0xC03A, TLS,ECDHE,PSK,WITH,NULL,SHA256,,),
  CS_ENTRY(0xC03A, ECDHE,PSK,NULL,SHA256,,,,),
  CS_ENTRY(0xC03B, TLS,ECDHE,PSK,WITH,NULL,SHA384,,),
  CS_ENTRY(0xC03B, ECDHE,PSK,NULL,SHA384,,,,),
  CS_ENTRY(0xC03C, TLS,RSA,WITH,ARIA,128,CBC,SHA256,),
  CS_ENTRY(0xC03C, ARIA128,SHA256,,,,,,), /* ns */
  CS_ENTRY(0xC03D, TLS,RSA,WITH,ARIA,256,CBC,SHA384,),
  CS_ENTRY(0xC03D, ARIA256,SHA384,,,,,,), /* ns */
  CS_ENTRY(0xC044, TLS,DHE,RSA,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC044, DHE,RSA,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC045, TLS,DHE,RSA,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC045, DHE,RSA,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC048, TLS,ECDHE,ECDSA,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC048, ECDHE,ECDSA,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC049, TLS,ECDHE,ECDSA,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC049, ECDHE,ECDSA,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC04A, TLS,ECDH,ECDSA,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC04A, ECDH,ECDSA,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC04B, TLS,ECDH,ECDSA,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC04B, ECDH,ECDSA,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC04C, TLS,ECDHE,RSA,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC04C, ECDHE,ARIA128,SHA256,,,,,), /* ns */
  CS_ENTRY(0xC04D, TLS,ECDHE,RSA,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC04D, ECDHE,ARIA256,SHA384,,,,,), /* ns */
  CS_ENTRY(0xC04E, TLS,ECDH,RSA,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC04E, ECDH,ARIA128,SHA256,,,,,), /* ns */
  CS_ENTRY(0xC04F, TLS,ECDH,RSA,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC04F, ECDH,ARIA256,SHA384,,,,,), /* ns */
  CS_ENTRY(0xC050, TLS,RSA,WITH,ARIA,128,GCM,SHA256,),
  CS_ENTRY(0xC050, ARIA128,GCM,SHA256,,,,,),
  CS_ENTRY(0xC051, TLS,RSA,WITH,ARIA,256,GCM,SHA384,),
  CS_ENTRY(0xC051, ARIA256,GCM,SHA384,,,,,),
  CS_ENTRY(0xC052, TLS,DHE,RSA,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC052, DHE,RSA,ARIA128,GCM,SHA256,,,),
  CS_ENTRY(0xC053, TLS,DHE,RSA,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC053, DHE,RSA,ARIA256,GCM,SHA384,,,),
  CS_ENTRY(0xC05C, TLS,ECDHE,ECDSA,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC05C, ECDHE,ECDSA,ARIA128,GCM,SHA256,,,),
  CS_ENTRY(0xC05D, TLS,ECDHE,ECDSA,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC05D, ECDHE,ECDSA,ARIA256,GCM,SHA384,,,),
  CS_ENTRY(0xC05E, TLS,ECDH,ECDSA,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC05E, ECDH,ECDSA,ARIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC05F, TLS,ECDH,ECDSA,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC05F, ECDH,ECDSA,ARIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC060, TLS,ECDHE,RSA,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC060, ECDHE,ARIA128,GCM,SHA256,,,,),
  CS_ENTRY(0xC061, TLS,ECDHE,RSA,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC061, ECDHE,ARIA256,GCM,SHA384,,,,),
  CS_ENTRY(0xC062, TLS,ECDH,RSA,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC062, ECDH,ARIA128,GCM,SHA256,,,,), /* ns */
  CS_ENTRY(0xC063, TLS,ECDH,RSA,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC063, ECDH,ARIA256,GCM,SHA384,,,,), /* ns */
  CS_ENTRY(0xC064, TLS,PSK,WITH,ARIA,128,CBC,SHA256,),
  CS_ENTRY(0xC064, PSK,ARIA128,SHA256,,,,,), /* ns */
  CS_ENTRY(0xC065, TLS,PSK,WITH,ARIA,256,CBC,SHA384,),
  CS_ENTRY(0xC065, PSK,ARIA256,SHA384,,,,,), /* ns */
  CS_ENTRY(0xC066, TLS,DHE,PSK,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC066, DHE,PSK,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC067, TLS,DHE,PSK,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC067, DHE,PSK,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC068, TLS,RSA,PSK,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC068, RSA,PSK,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC069, TLS,RSA,PSK,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC069, RSA,PSK,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC06A, TLS,PSK,WITH,ARIA,128,GCM,SHA256,),
  CS_ENTRY(0xC06A, PSK,ARIA128,GCM,SHA256,,,,),
  CS_ENTRY(0xC06B, TLS,PSK,WITH,ARIA,256,GCM,SHA384,),
  CS_ENTRY(0xC06B, PSK,ARIA256,GCM,SHA384,,,,),
  CS_ENTRY(0xC06C, TLS,DHE,PSK,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC06C, DHE,PSK,ARIA128,GCM,SHA256,,,),
  CS_ENTRY(0xC06D, TLS,DHE,PSK,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC06D, DHE,PSK,ARIA256,GCM,SHA384,,,),
  CS_ENTRY(0xC06E, TLS,RSA,PSK,WITH,ARIA,128,GCM,SHA256),
  CS_ENTRY(0xC06E, RSA,PSK,ARIA128,GCM,SHA256,,,),
  CS_ENTRY(0xC06F, TLS,RSA,PSK,WITH,ARIA,256,GCM,SHA384),
  CS_ENTRY(0xC06F, RSA,PSK,ARIA256,GCM,SHA384,,,),
  CS_ENTRY(0xC070, TLS,ECDHE,PSK,WITH,ARIA,128,CBC,SHA256),
  CS_ENTRY(0xC070, ECDHE,PSK,ARIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC071, TLS,ECDHE,PSK,WITH,ARIA,256,CBC,SHA384),
  CS_ENTRY(0xC071, ECDHE,PSK,ARIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC072, TLS,ECDHE,ECDSA,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC072, ECDHE,ECDSA,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0xC073, TLS,ECDHE,ECDSA,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC073, ECDHE,ECDSA,CAMELLIA256,SHA384,,,,),
  CS_ENTRY(0xC074, TLS,ECDH,ECDSA,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC074, ECDH,ECDSA,CAMELLIA128,SHA256,,,,), /* ns */
  CS_ENTRY(0xC075, TLS,ECDH,ECDSA,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC075, ECDH,ECDSA,CAMELLIA256,SHA384,,,,), /* ns */
  CS_ENTRY(0xC076, TLS,ECDHE,RSA,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC076, ECDHE,RSA,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0xC077, TLS,ECDHE,RSA,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC077, ECDHE,RSA,CAMELLIA256,SHA384,,,,),
  CS_ENTRY(0xC078, TLS,ECDH,RSA,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC078, ECDH,CAMELLIA128,SHA256,,,,,), /* ns */
  CS_ENTRY(0xC079, TLS,ECDH,RSA,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC079, ECDH,CAMELLIA256,SHA384,,,,,), /* ns */
  CS_ENTRY(0xC07A, TLS,RSA,WITH,CAMELLIA,128,GCM,SHA256,),
  CS_ENTRY(0xC07A, CAMELLIA128,GCM,SHA256,,,,,), /* ns */
  CS_ENTRY(0xC07B, TLS,RSA,WITH,CAMELLIA,256,GCM,SHA384,),
  CS_ENTRY(0xC07B, CAMELLIA256,GCM,SHA384,,,,,), /* ns */
  CS_ENTRY(0xC07C, TLS,DHE,RSA,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC07C, DHE,RSA,CAMELLIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC07D, TLS,DHE,RSA,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC07D, DHE,RSA,CAMELLIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC086, TLS,ECDHE,ECDSA,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC086, ECDHE,ECDSA,CAMELLIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC087, TLS,ECDHE,ECDSA,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC087, ECDHE,ECDSA,CAMELLIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC088, TLS,ECDH,ECDSA,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC088, ECDH,ECDSA,CAMELLIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC089, TLS,ECDH,ECDSA,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC089, ECDH,ECDSA,CAMELLIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC08A, TLS,ECDHE,RSA,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC08A, ECDHE,CAMELLIA128,GCM,SHA256,,,,), /* ns */
  CS_ENTRY(0xC08B, TLS,ECDHE,RSA,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC08B, ECDHE,CAMELLIA256,GCM,SHA384,,,,), /* ns */
  CS_ENTRY(0xC08C, TLS,ECDH,RSA,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC08C, ECDH,CAMELLIA128,GCM,SHA256,,,,), /* ns */
  CS_ENTRY(0xC08D, TLS,ECDH,RSA,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC08D, ECDH,CAMELLIA256,GCM,SHA384,,,,), /* ns */
  CS_ENTRY(0xC08E, TLS,PSK,WITH,CAMELLIA,128,GCM,SHA256,),
  CS_ENTRY(0xC08E, PSK,CAMELLIA128,GCM,SHA256,,,,), /* ns */
  CS_ENTRY(0xC08F, TLS,PSK,WITH,CAMELLIA,256,GCM,SHA384,),
  CS_ENTRY(0xC08F, PSK,CAMELLIA256,GCM,SHA384,,,,), /* ns */
  CS_ENTRY(0xC090, TLS,DHE,PSK,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC090, DHE,PSK,CAMELLIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC091, TLS,DHE,PSK,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC091, DHE,PSK,CAMELLIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC092, TLS,RSA,PSK,WITH,CAMELLIA,128,GCM,SHA256),
  CS_ENTRY(0xC092, RSA,PSK,CAMELLIA128,GCM,SHA256,,,), /* ns */
  CS_ENTRY(0xC093, TLS,RSA,PSK,WITH,CAMELLIA,256,GCM,SHA384),
  CS_ENTRY(0xC093, RSA,PSK,CAMELLIA256,GCM,SHA384,,,), /* ns */
  CS_ENTRY(0xC094, TLS,PSK,WITH,CAMELLIA,128,CBC,SHA256,),
  CS_ENTRY(0xC094, PSK,CAMELLIA128,SHA256,,,,,),
  CS_ENTRY(0xC095, TLS,PSK,WITH,CAMELLIA,256,CBC,SHA384,),
  CS_ENTRY(0xC095, PSK,CAMELLIA256,SHA384,,,,,),
  CS_ENTRY(0xC096, TLS,DHE,PSK,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC096, DHE,PSK,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0xC097, TLS,DHE,PSK,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC097, DHE,PSK,CAMELLIA256,SHA384,,,,),
  CS_ENTRY(0xC098, TLS,RSA,PSK,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC098, RSA,PSK,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0xC099, TLS,RSA,PSK,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC099, RSA,PSK,CAMELLIA256,SHA384,,,,),
  CS_ENTRY(0xC09A, TLS,ECDHE,PSK,WITH,CAMELLIA,128,CBC,SHA256),
  CS_ENTRY(0xC09A, ECDHE,PSK,CAMELLIA128,SHA256,,,,),
  CS_ENTRY(0xC09B, TLS,ECDHE,PSK,WITH,CAMELLIA,256,CBC,SHA384),
  CS_ENTRY(0xC09B, ECDHE,PSK,CAMELLIA256,SHA384,,,,),
  CS_ENTRY(0xC09E, TLS,DHE,RSA,WITH,AES,128,CCM,),
  CS_ENTRY(0xC09E, DHE,RSA,AES128,CCM,,,,),
  CS_ENTRY(0xC09F, TLS,DHE,RSA,WITH,AES,256,CCM,),
  CS_ENTRY(0xC09F, DHE,RSA,AES256,CCM,,,,),
  CS_ENTRY(0xC0A2, TLS,DHE,RSA,WITH,AES,128,CCM,8),
  CS_ENTRY(0xC0A2, DHE,RSA,AES128,CCM8,,,,),
  CS_ENTRY(0xC0A3, TLS,DHE,RSA,WITH,AES,256,CCM,8),
  CS_ENTRY(0xC0A3, DHE,RSA,AES256,CCM8,,,,),
  CS_ENTRY(0xC0A4, TLS,PSK,WITH,AES,128,CCM,,),
  CS_ENTRY(0xC0A4, PSK,AES128,CCM,,,,,),
  CS_ENTRY(0xC0A5, TLS,PSK,WITH,AES,256,CCM,,),
  CS_ENTRY(0xC0A5, PSK,AES256,CCM,,,,,),
  CS_ENTRY(0xC0A6, TLS,DHE,PSK,WITH,AES,128,CCM,),
  CS_ENTRY(0xC0A6, DHE,PSK,AES128,CCM,,,,),
  CS_ENTRY(0xC0A7, TLS,DHE,PSK,WITH,AES,256,CCM,),
  CS_ENTRY(0xC0A7, DHE,PSK,AES256,CCM,,,,),
  CS_ENTRY(0xC0A8, TLS,PSK,WITH,AES,128,CCM,8,),
  CS_ENTRY(0xC0A8, PSK,AES128,CCM8,,,,,),
  CS_ENTRY(0xC0A9, TLS,PSK,WITH,AES,256,CCM,8,),
  CS_ENTRY(0xC0A9, PSK,AES256,CCM8,,,,,),
  CS_ENTRY(0xC0AA, TLS,PSK,DHE,WITH,AES,128,CCM,8),
  CS_ENTRY(0xC0AA, DHE,PSK,AES128,CCM8,,,,),
  CS_ENTRY(0xC0AB, TLS,PSK,DHE,WITH,AES,256,CCM,8),
  CS_ENTRY(0xC0AB, DHE,PSK,AES256,CCM8,,,,),
  CS_ENTRY(0xCCAA, TLS,DHE,RSA,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCAA, DHE,RSA,CHACHA20,POLY1305,,,,),
  CS_ENTRY(0xCCAC, TLS,ECDHE,PSK,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCAC, ECDHE,PSK,CHACHA20,POLY1305,,,,),
  CS_ENTRY(0xCCAD, TLS,DHE,PSK,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCAD, DHE,PSK,CHACHA20,POLY1305,,,,),
  CS_ENTRY(0xCCAE, TLS,RSA,PSK,WITH,CHACHA20,POLY1305,SHA256,),
  CS_ENTRY(0xCCAE, RSA,PSK,CHACHA20,POLY1305,,,,),
};
#define CS_LIST_LEN (sizeof(cs_list) / sizeof(cs_list[0]))

static int cs_str_to_zip(const char *cs_str, size_t cs_len,
                         uint8_t zip[6])
{
  uint8_t indexes[8] = {0};
  const char *entry, *cur;
  const char *nxt = cs_str;
  const char *end = cs_str + cs_len;
  char separator = '-';
  int idx, i = 0;
  size_t len;

  /* split the cipher string by '-' or '_' */
  if(strncasecompare(cs_str, "TLS", 3))
    separator = '_';

  do {
    if(i == 8)
      return -1;

    /* determine the length of the part */
    cur = nxt;
    for(; nxt < end && *nxt != '\0' && *nxt != separator; nxt++);
    len = nxt - cur;

    /* lookup index for the part (skip empty string at 0) */
    for(idx = 1, entry = cs_txt + 1; idx < CS_TXT_LEN; idx++) {
      size_t elen = strlen(entry);
      if(elen == len && strncasecompare(entry, cur, len))
        break;
      entry += elen + 1;
    }
    if(idx == CS_TXT_LEN)
      return -1;

    indexes[i++] = (uint8_t) idx;
  } while(nxt < end && *(nxt++) != '\0');

  /* zip the 8 indexes into 48 bits */
  zip[0] = (uint8_t) (indexes[0] << 2 | (indexes[1] & 0x3F) >> 4);
  zip[1] = (uint8_t) (indexes[1] << 4 | (indexes[2] & 0x3F) >> 2);
  zip[2] = (uint8_t) (indexes[2] << 6 | (indexes[3] & 0x3F));
  zip[3] = (uint8_t) (indexes[4] << 2 | (indexes[5] & 0x3F) >> 4);
  zip[4] = (uint8_t) (indexes[5] << 4 | (indexes[6] & 0x3F) >> 2);
  zip[5] = (uint8_t) (indexes[6] << 6 | (indexes[7] & 0x3F));

  return 0;
}

static int cs_zip_to_str(const uint8_t zip[6],
                         char *buf, size_t buf_size)
{
  uint8_t indexes[8] = {0};
  const char *entry;
  char separator = '-';
  int idx, i, r;
  size_t len = 0;

  /* unzip the 8 indexes */
  indexes[0] = zip[0] >> 2;
  indexes[1] = ((zip[0] << 4) & 0x3F) | zip[1] >> 4;
  indexes[2] = ((zip[1] << 2) & 0x3F) | zip[2] >> 6;
  indexes[3] = ((zip[2] << 0) & 0x3F);
  indexes[4] = zip[3] >> 2;
  indexes[5] = ((zip[3] << 4) & 0x3F) | zip[4] >> 4;
  indexes[6] = ((zip[4] << 2) & 0x3F) | zip[5] >> 6;
  indexes[7] = ((zip[5] << 0) & 0x3F);

  if(indexes[0] == CS_TXT_IDX_TLS)
    separator = '_';

  for(i = 0; i < 8 && indexes[i] != 0 && len < buf_size; i++) {
    if(indexes[i] >= CS_TXT_LEN)
      return -1;

    /* lookup the part string for the index (skip empty string at 0) */
    for(idx = 1, entry = cs_txt + 1; idx < indexes[i]; idx++) {
      size_t elen = strlen(entry);
      entry += elen + 1;
    }

    /* append the part string to the buffer */
    if(i > 0)
      r = msnprintf(&buf[len], buf_size - len, "%c%s", separator, entry);
    else
      r = msnprintf(&buf[len], buf_size - len, "%s", entry);

    if(r < 0)
      return -1;
    len += r;
  }

  return 0;
}

uint16_t Curl_cipher_suite_lookup_id(const char *cs_str, size_t cs_len)
{
  size_t i;
  uint8_t zip[6];

  if(cs_len > 0 && cs_str_to_zip(cs_str, cs_len, zip) == 0) {
    for(i = 0; i < CS_LIST_LEN; i++) {
      if(memcmp(cs_list[i].zip, zip, sizeof(zip)) == 0)
        return cs_list[i].id;
    }
  }

  return 0;
}

static bool cs_is_separator(char c)
{
  switch(c) {
    case ' ':
    case '\t':
    case ':':
    case ',':
    case ';':
      return true;
    default:;
  }
  return false;
}

uint16_t Curl_cipher_suite_walk_str(const char **str, const char **end)
{
  /* move string pointer to first non-separator or end of string */
  for(; cs_is_separator(*str[0]); (*str)++);

  /* move end pointer to next separator or end of string */
  for(*end = *str; *end[0] != '\0' && !cs_is_separator(*end[0]); (*end)++);

  return Curl_cipher_suite_lookup_id(*str, *end - *str);
}

int Curl_cipher_suite_get_str(uint16_t id, char *buf, size_t buf_size,
                              bool prefer_rfc)
{
  size_t i, j = CS_LIST_LEN;
  int r = -1;

  for(i = 0; i < CS_LIST_LEN; i++) {
    if(cs_list[i].id != id)
      continue;
    if((cs_list[i].zip[0] >> 2 != CS_TXT_IDX_TLS) == !prefer_rfc) {
      j = i;
      break;
    }
    if(j == CS_LIST_LEN)
      j = i;
  }

  if(j < CS_LIST_LEN)
    r = cs_zip_to_str(cs_list[j].zip, buf, buf_size);

  if(r < 0)
    msnprintf(buf, buf_size, "TLS_UNKNOWN_0x%04x", id);

  return r;
}

#endif /* defined(USE_MBEDTLS) */

/**
 *  Copyright 2009-2014 MongoDB, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef __MCON_MINI_BSON_H__
#define __MCON_MINI_BSON_H__

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

mcon_str *bson_create_ping_packet(mongo_connection *con);
mcon_str *bson_create_ismaster_packet(mongo_connection *con);
mcon_str *bson_create_buildinfo_packet(mongo_connection *con);
mcon_str *bson_create_rs_status_packet(mongo_connection *con);
mcon_str *bson_create_getnonce_packet(mongo_connection *con);
mcon_str *bson_create_authenticate_packet(mongo_connection *con, char *mechanism, char *database, char *username, char *nonce, char *key);
mcon_str *bson_create_saslstart_packet(mongo_connection *con, char *database, char *mechanism, char *payload, int payload_len);
mcon_str *bson_create_saslcontinue_packet(mongo_connection *con, char *database, int32_t conversation_id, char *payload, int payload_len);

char *bson_skip_field_name(char *data);
int bson_find_field_as_array(char *buffer, char *field, char **data);
int bson_find_field_as_document(char *buffer, char *field, char **data);
int bson_find_field_as_double(char *buffer, char *field, double *data);
int bson_find_field_as_bool(char *buffer, char *field, unsigned char *data);
int bson_find_field_as_int32(char *buffer, char *field, int32_t *data);
int bson_find_field_as_int64(char *buffer, char *field, int64_t *data);
int bson_find_field_as_string(char *buffer, char *field, char **data);
int bson_find_field_as_stringl(char *buffer, char *field, char **data, int32_t *length, int duplicate);

int bson_array_find_next_string(char **buffer, char **field, char **data);
int bson_array_find_next_int32(char **buffer, char **field, int32_t *data);

#if defined(__cplusplus)
}
#endif 

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */

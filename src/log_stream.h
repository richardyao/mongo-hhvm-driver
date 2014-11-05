
#ifndef __LOG_STREAM_H__
#define __LOG_STREAM_H__

#include "stream/php_streams.h"
#include "stream/php_network.h"

#include "mcon/types.h"
//#include "php_mongo.h"
//#include "api/write.h"

#define MONGO_STREAM_NOTIFY_TYPE_IO_INIT        100
#define MONGO_STREAM_NOTIFY_TYPE_LOG            200

#define MONGO_STREAM_NOTIFY_IO_READ             111
#define MONGO_STREAM_NOTIFY_IO_WRITE            112
#define MONGO_STREAM_NOTIFY_IO_PROGRESS         PHP_STREAM_NOTIFY_PROGRESS
#define MONGO_STREAM_NOTIFY_IO_COMPLETED        PHP_STREAM_NOTIFY_COMPLETED

#define MONGO_STREAM_NOTIFY_LOG_INSERT          211
#define MONGO_STREAM_NOTIFY_LOG_QUERY           212
#define MONGO_STREAM_NOTIFY_LOG_UPDATE          213
#define MONGO_STREAM_NOTIFY_LOG_DELETE          214
#define MONGO_STREAM_NOTIFY_LOG_GETMORE         215
#define MONGO_STREAM_NOTIFY_LOG_KILLCURSOR      216
#define MONGO_STREAM_NOTIFY_LOG_BATCHINSERT     217
#define MONGO_STREAM_NOTIFY_LOG_RESPONSE_HEADER 218
#define MONGO_STREAM_NOTIFY_LOG_WRITE_REPLY     219
#define MONGO_STREAM_NOTIFY_LOG_CMD_INSERT      220
#define MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE      221
#define MONGO_STREAM_NOTIFY_LOG_CMD_DELETE      222
#define MONGO_STREAM_NOTIFY_LOG_WRITE_BATCH     223

//void php_mongo_stream_notify_io(mongo_server_options *opts, int code, int sofar, int max);
//void mongo_log_stream_insert(mongo_connection *connection, zval *document, zval *options);
//void mongo_log_stream_query(mongo_connection *connection, mongo_cursor *cursor);
//void mongo_log_stream_update(mongo_connection *connection, zval *ns, zval *criteria, zval *newobj, zval *options, int flags);
//void mongo_log_stream_delete(mongo_connection *connection, zval *ns, zval *criteria, zval *options, int flags);
//void mongo_log_stream_getmore(mongo_connection *connection, mongo_cursor *cursor);
//void mongo_log_stream_killcursor(mongo_connection *connection, int64_t cursor_id);
//void mongo_log_stream_batchinsert(mongo_connection *connection, zval *docs, zval *options, int flags);
//void mongo_log_stream_response_header(mongo_connection *connection, mongo_cursor *cursor);
//void mongo_log_stream_write_reply(mongo_connection *connection, mongo_msg_header *header, php_mongo_reply *reply);
//void mongo_log_stream_cmd_insert(mongo_connection *connection, zval *document, php_mongo_write_options *write_options, int message_length, int request_id, char *ns);
//void mongo_log_stream_cmd_update(mongo_connection *connection, php_mongo_write_update_args *update_options, php_mongo_write_options *write_options, int message_length, int request_id, char *ns);
//void mongo_log_stream_cmd_delete(mongo_connection *connection, php_mongo_write_delete_args *delete_options, php_mongo_write_options *write_options, int message_length, int request_id, char *ns);
//void mongo_log_stream_write_batch(mongo_connection *connection, php_mongo_write_options *write_options, int request_id, zval *batch);

#endif

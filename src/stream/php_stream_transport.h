
#ifndef STREAM_PHP_STREAM_TRANSPORT_H 
#define STREAM_PHP_STREAM_TRANSPORT_H

#include <sys/types.h>    
#include <sys/socket.h>

typedef php_stream *(php_stream_transport_factory_func)(const char *proto, size_t protolen,
		const char *resourcename, size_t resourcenamelen,
		const char *persistent_id, int options, int flags,
		struct timeval *timeout,
		php_stream_context *context STREAMS_DC);
typedef php_stream_transport_factory_func *php_stream_transport_factory;

int php_stream_xport_register(const char *protocol, php_stream_transport_factory factory);
int php_stream_xport_unregister(const char *protocol);

#define STREAM_XPORT_CLIENT			0
#define STREAM_XPORT_SERVER			1

#define STREAM_XPORT_CONNECT		2
#define STREAM_XPORT_BIND			4
#define STREAM_XPORT_LISTEN			8
#define STREAM_XPORT_CONNECT_ASYNC	16

/* Open a client or server socket connection */
php_stream *php_stream_xport_create(const char *name, size_t namelen, int options,
		int flags, const char *persistent_id,
		struct timeval *timeout,
		php_stream_context *context,
		char **error_string,
		int *error_code);

/* Bind the stream to a local address */
int php_stream_xport_bind(php_stream *stream,
		const char *name, size_t namelen,
		char **error_text);

/* Connect to a remote address */
int php_stream_xport_connect(php_stream *stream,
		const char *name, size_t namelen,
		int asynchronous,
		struct timeval *timeout,
		char **error_text,
		int *error_code);

/* Prepare to listen */
int php_stream_xport_listen(php_stream *stream,
		int backlog,
		char **error_text);

/* Get the next client and their address as a string, or the underlying address
 * structure.  You must efree either of these if you request them */
int php_stream_xport_accept(php_stream *stream, php_stream **client,
		char **textaddr, int *textaddrlen,
		void **addr, socklen_t *addrlen,
		struct timeval *timeout,
		char **error_text);

/* Get the name of either the socket or it's peer */
int php_stream_xport_get_name(php_stream *stream, int want_peer,
		char **textaddr, int *textaddrlen,
		void **addr, socklen_t *addrlen);

enum php_stream_xport_send_recv_flags {
	STREAM_OOB = 1,
	STREAM_PEEK = 2
};

/* Similar to recv() system call; read data from the stream, optionally
 * peeking, optionally retrieving OOB data */
int php_stream_xport_recvfrom(php_stream *stream, char *buf, size_t buflen,
		long flags, void **addr, socklen_t *addrlen,
		char **textaddr, int *textaddrlen);

/* Similar to send() system call; send data to the stream, optionally
 * sending it as OOB data */
int php_stream_xport_sendto(php_stream *stream, const char *buf, size_t buflen,
		long flags, void *addr, socklen_t addrlen);

typedef enum {
	STREAM_SHUT_RD,
	STREAM_SHUT_WR,
	STREAM_SHUT_RDWR
} stream_shutdown_t;

/* Similar to shutdown() system call; shut down part of a full-duplex
 * connection */
int php_stream_xport_shutdown(php_stream *stream, stream_shutdown_t how);


/* Structure definition for the set_option interface that the above functions wrap */

typedef struct _php_stream_xport_param {
	enum {
		STREAM_XPORT_OP_BIND, STREAM_XPORT_OP_CONNECT,
		STREAM_XPORT_OP_LISTEN, STREAM_XPORT_OP_ACCEPT,
		STREAM_XPORT_OP_CONNECT_ASYNC,
		STREAM_XPORT_OP_GET_NAME,
		STREAM_XPORT_OP_GET_PEER_NAME,
		STREAM_XPORT_OP_RECV,
		STREAM_XPORT_OP_SEND,
		STREAM_XPORT_OP_SHUTDOWN
	} op;
	unsigned int want_addr:1;
	unsigned int want_textaddr:1;
	unsigned int want_errortext:1;
	unsigned int how:2;

	struct {
		char *name;
		size_t namelen;
		int backlog;
		struct timeval *timeout;
		struct sockaddr *addr;
		socklen_t addrlen;
		char *buf;
		size_t buflen;
		long flags;
	} inputs;
	struct {
		php_stream *client;
		int returncode;
		struct sockaddr *addr;
		socklen_t addrlen;
		char *textaddr;
		long textaddrlen;

		char *error_text;
		int error_code;
	} outputs;
} php_stream_xport_param;

/* Because both client and server streams use the same mechanisms
   for encryption we use the LSB to denote clients.
*/
typedef enum {
	STREAM_CRYPTO_METHOD_SSLv2_CLIENT = (1 << 1 | 1),
	STREAM_CRYPTO_METHOD_SSLv3_CLIENT = (1 << 2 | 1),
	STREAM_CRYPTO_METHOD_SSLv23_CLIENT = ((1 << 1) | (1 << 2) | 1),
	STREAM_CRYPTO_METHOD_TLSv1_0_CLIENT = (1 << 3 | 1),
	STREAM_CRYPTO_METHOD_TLSv1_1_CLIENT = (1 << 4 | 1),
	STREAM_CRYPTO_METHOD_TLSv1_2_CLIENT = (1 << 5 | 1),
	STREAM_CRYPTO_METHOD_TLS_CLIENT = ((1 << 3) | (1 << 4) | (1 << 5) | 1),
	STREAM_CRYPTO_METHOD_ANY_CLIENT = ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | 1),
	STREAM_CRYPTO_METHOD_SSLv2_SERVER = (1 << 1),
	STREAM_CRYPTO_METHOD_SSLv3_SERVER = (1 << 2),
	STREAM_CRYPTO_METHOD_SSLv23_SERVER = ((1 << 1) | (1 << 2)),
	STREAM_CRYPTO_METHOD_TLSv1_0_SERVER = (1 << 3),
	STREAM_CRYPTO_METHOD_TLSv1_1_SERVER = (1 << 4),
	STREAM_CRYPTO_METHOD_TLSv1_2_SERVER = (1 << 5),
	STREAM_CRYPTO_METHOD_TLS_SERVER = ((1 << 3) | (1 << 4) | (1 << 5)),
	STREAM_CRYPTO_METHOD_ANY_SERVER = ((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))
} php_stream_xport_crypt_method_t;

/* These functions provide crypto support on the underlying transport */

int php_stream_xport_crypto_setup(php_stream *stream, php_stream_xport_crypt_method_t crypto_method, php_stream *session_stream);
int php_stream_xport_crypto_enable(php_stream *stream, int activate);

typedef struct _php_stream_xport_crypto_param {
	enum {
		STREAM_XPORT_CRYPTO_OP_SETUP,
		STREAM_XPORT_CRYPTO_OP_ENABLE
	} op;
	struct {
		int activate;
		php_stream_xport_crypt_method_t method;
		php_stream *session;
	} inputs;
	struct {
		int returncode;
	} outputs;
} php_stream_xport_crypto_param;

/* HashTable *php_stream_xport_get_hash(void); */
php_stream_transport_factory_func php_stream_generic_socket_factory;

#endif // STREAM_PHP_STREAM_TRANSPORT_H

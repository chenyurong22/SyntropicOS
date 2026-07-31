/**
 * @file syn_tls.h
 * @brief Native Zero-Heap TLS 1.3 Protocol Engine & Transport Adapter (RFC 8446).
 *
 * Implements TLS 1.3 Client & Server handshakes and encrypted record layer.
 * Supported modes:
 * - Pre-Shared Key (PSK / PSK-DHE)
 * - Raw Public Key (RPK) - RFC 7250
 * - X.509 Server Authentication & Mutual TLS (mTLS)
 *
 * Plugs into standard SYN_Transport interface.
 * @ingroup syn_net
 */

#ifndef SYN_TLS_H
#define SYN_TLS_H

#include "syntropic/crypto/syn_chacha20poly1305.h"
#include "syntropic/crypto/syn_hkdf.h"
#include "syntropic/crypto/syn_x25519.h"
#include "syntropic/crypto/syn_x509.h"
#include "syntropic/net/syn_transport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum TLS record payload size in bytes (2048). */
#define SYN_TLS_RECORD_MAX_PAYLOAD 2048U
/** @brief Length of TLS 1.3 secret keys in bytes (32). */
#define SYN_TLS_SECRET_LEN 32U

/** TLS 1.3 Handshake State Machine States */
typedef enum {
    SYN_TLS_STATE_UNINITIALIZED = 0,
    SYN_TLS_STATE_CLIENT_HELLO_SENT,
    SYN_TLS_STATE_SERVER_HELLO_RECEIVED,
    SYN_TLS_STATE_HANDSHAKE_KEYS_DERIVED,
    SYN_TLS_STATE_CERTIFICATE_VERIFIED,
    SYN_TLS_STATE_FINISHED_SENT,
    SYN_TLS_STATE_ESTABLISHED,
    SYN_TLS_STATE_ERROR
} SYN_TLS_State;

/** TLS 1.3 Authentication Mode */
typedef enum {
    SYN_TLS_AUTH_MODE_PSK = 0,
    SYN_TLS_AUTH_MODE_RAW_PUBKEY,
    SYN_TLS_AUTH_MODE_X509_SERVER,
    SYN_TLS_AUTH_MODE_MTLS
} SYN_TLS_AuthMode;

/** TLS 1.3 Engine Configuration */
typedef struct {
    SYN_TLS_AuthMode mode;   /**< PSK, RPK, X.509, or mTLS */
    const char *server_name; /**< SNI hostname (optional) */

    /* PSK configuration */
    const uint8_t *psk_identity; /**< PSK identity bytes */
    size_t psk_identity_len;     /**< PSK identity length */
    const uint8_t *psk_secret;   /**< 32-byte pre-shared secret */
    size_t psk_secret_len;       /**< Secret length */

    /* Raw Public Key configuration */
    const uint8_t *peer_pubkey; /**< 32-byte raw peer X25519/Ed25519 public key */
    size_t peer_pubkey_len;     /**< Raw peer public key length */

    /* X.509 & mTLS configuration */
    const SYN_X509_Cert *root_ca;   /**< Trusted Root CA cert for server validation */
    const uint8_t *client_cert_der; /**< Client certificate DER bytes (for mTLS) */
    size_t client_cert_len;         /**< Client certificate length */
    const uint8_t *client_privkey;  /**< Client private key (32 bytes) */
} SYN_TLS_Config;

/** TLS 1.3 Engine Context (Caller-owned, zero-heap). */
typedef struct {
    SYN_TLS_State state;                 /**< Current handshake state */
    SYN_TLS_Config config;               /**< Engine configuration copy */
    SYN_Transport *underlying_transport; /**< Wire transport (TCP, UDP, Serial) */

    uint8_t my_privkey[SYN_TLS_SECRET_LEN];  /**< Ephemeral X25519 private key */
    uint8_t my_pubkey[SYN_TLS_SECRET_LEN];   /**< Ephemeral X25519 public key */
    uint8_t peer_pubkey[SYN_TLS_SECRET_LEN]; /**< Peer ephemeral X25519 public key */

    SYN_SHA256 transcript_hash; /**< Running handshake transcript hash */

    uint8_t client_handshake_secret[SYN_TLS_SECRET_LEN]; /**< Client handshake traffic secret */
    uint8_t server_handshake_secret[SYN_TLS_SECRET_LEN]; /**< Server handshake traffic secret */
    uint8_t client_app_secret[SYN_TLS_SECRET_LEN];       /**< Client application traffic secret */
    uint8_t server_app_secret[SYN_TLS_SECRET_LEN];       /**< Server application traffic secret */

    uint64_t client_seq_num; /**< Encryption record sequence counter */
    uint64_t server_seq_num; /**< Decryption record sequence counter */
} SYN_TLS_Context;

/**
 * @brief Initialize a TLS 1.3 context.
 *
 * @param ctx       Context to initialize.
 * @param config    Configuration.
 * @param transport Wire transport instance.
 * @return true on success.
 */
bool syn_tls_init(SYN_TLS_Context *ctx, const SYN_TLS_Config *config, SYN_Transport *transport);

/**
 * @brief Perform a non-blocking TLS 1.3 handshake step.
 *
 * @param ctx TLS 1.3 context.
 * @return true if handshake completed or progressing, false on fatal error.
 */
bool syn_tls_handshake(SYN_TLS_Context *ctx);

/**
 * @brief Encrypt and transmit application data payload over TLS 1.3.
 *
 * @param ctx  TLS 1.3 context.
 * @param data Application data buffer.
 * @param len  Application data length.
 * @return true if record encrypted and sent successfully.
 */
bool syn_tls_send(SYN_TLS_Context *ctx, const uint8_t *data, size_t len);

/**
 * @brief Receive and decrypt TLS 1.3 record payload into caller buffer.
 *
 * @param ctx     TLS 1.3 context.
 * @param data    [out] Output buffer for decrypted data.
 * @param max_len Capacity of output buffer.
 * @param out_len [out] Actual decrypted bytes written.
 * @return true if a complete record decrypted.
 */
bool syn_tls_recv(SYN_TLS_Context *ctx, uint8_t *data, size_t max_len, size_t *out_len);

/**
 * @brief Wrap a TLS 1.3 context into a standard SYN_Transport abstraction.
 *
 * @param tls_ctx  Initialized TLS 1.3 context.
 * @param tr_out   [out] Output transport instance.
 */
void syn_tls_bind_transport(SYN_TLS_Context *tls_ctx, SYN_Transport *tr_out);

#ifdef __cplusplus
}
#endif

#endif /* SYN_TLS_H */

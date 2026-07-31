/**
 * @file syn_ed25519.h
 * @brief Pure C99 Ed25519 Signature Verification (RFC 8032).
 * @ingroup syn_crypto
 */

#ifndef SYN_ED25519_H
#define SYN_ED25519_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Size of Ed25519 public key in bytes (32). */
#define SYN_ED25519_PUBLIC_KEY_SIZE 32U
/** @brief Size of Ed25519 signature in bytes (64). */
#define SYN_ED25519_SIGNATURE_SIZE 64U

/**
 * @brief Verify an Ed25519 signature (RFC 8032 Section 5.1.7).
 *
 * @param sig        64-byte signature (R || S).
 * @param msg        Message bytes.
 * @param msg_len    Message length in bytes.
 * @param public_key 32-byte Ed25519 public key.
 * @return true if valid signature, false otherwise.
 */
bool syn_ed25519_verify(const uint8_t sig[SYN_ED25519_SIGNATURE_SIZE], const uint8_t *msg,
                        size_t msg_len, const uint8_t public_key[SYN_ED25519_PUBLIC_KEY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* SYN_ED25519_H */

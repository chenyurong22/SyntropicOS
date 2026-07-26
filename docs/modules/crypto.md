# Cryptography & Security Modules

SyntropicOS provides pure C99, constant-time cryptographic primitives and AEAD ciphers designed for resource-constrained microcontrollers.

---

## Technical Specifications

| Feature | Specification |
|---|---|
| **Constant-Time** | Branchless arithmetic operations to prevent side-channel timing attacks. |
| **Memory Allocation** | **100% Zero-Heap**. All operations use caller-owned stacks or buffers. |
| **Standards Compliance** | BLAKE2s (RFC 7693), ChaCha20-Poly1305 AEAD (RFC 8439), X25519 (RFC 7748). |

---

## 1. BLAKE2s Cryptographic Hash (`crypto/syn_blake2s.h`)

BLAKE2s-256 provides high-speed cryptographic hashing and keyed-MAC authentication (HMAC replacement without HMAC overhead).

```c
#include <syntropic/crypto/syn_blake2s.h>

void hash_payload(const uint8_t *msg, size_t len, uint8_t digest[32]) {
    // One-shot BLAKE2s-256 hash
    syn_blake2s(msg, len, digest, 32);
}

void compute_mac(const uint8_t *key, size_t key_len, const uint8_t *msg, size_t msg_len, uint8_t tag[16]) {
    SYN_BLAKE2s ctx;
    syn_blake2s_init_keyed(&ctx, key, key_len, 16);
    syn_blake2s_update(&ctx, msg, msg_len);
    syn_blake2s_final(&ctx, tag);
}
```

---

## 2. ChaCha20-Poly1305 AEAD Cipher (`crypto/syn_chacha20poly1305.h`)

Authenticated Encryption with Associated Data (AEAD) per RFC 8439.

```c
#include <syntropic/crypto/syn_chacha20poly1305.h>

void encrypt_sensor_telemetry(void) {
    uint8_t key[32] = { /* 256-bit symmetric key */ };
    uint8_t nonce[12] = { /* 96-bit unique nonce */ };
    
    uint8_t payload[64] = "Temperature=24.5C;Humidity=60%";
    uint8_t ciphertext[64];
    uint8_t auth_tag[16];

    // Encrypt and authenticate payload
    syn_aead_encrypt(key, nonce,
                     NULL, 0, // Additional Authenticated Data (AAD)
                     payload, sizeof(payload),
                     ciphertext, auth_tag);
}
```

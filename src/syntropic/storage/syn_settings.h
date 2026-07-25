/**
 * @file syn_settings.h
 * @brief Persistent settings manager with change detection.
 *
 * Wraps syn_param (wear-leveled flash storage) with:
 * - Automatic CRC-16 checksum for change detection
 * - Default values initialization
 * - Load-or-default on init
 * - Dirty tracking (has data changed since last save?)
 * - Optional change callback
 *
 * Eliminates the repetitive InitSettings / LoadSettings / SaveSettings /
 * update_checksum boilerplate seen in every firmware module.
 *
 * @par Usage
 * @code
 *   typedef struct {
 *       int32_t velocity_max;
 *       int32_t acceleration;
 *       int32_t power_max;
 *   } MoverSettings;
 *
 *   static const MoverSettings defaults = { 500, 200, 80 };
 *   static MoverSettings settings;
 *   static SYN_Settings store;
 *
 *   // Init — loads from flash or applies defaults:
 *   syn_settings_init(&store, FLASH_MOVER_BASE, 2,
 *                     &settings, sizeof(settings), &defaults);
 *
 *   // Modify and save:
 *   settings.velocity_max = 600;
 *   syn_settings_save(&store);  // CRC updated, writes flash
 *
 *   // Remote sync — check if remote copy is stale:
 *   if (remote_checksum != syn_settings_checksum(&store)) {
 *       send_settings_to_remote(&settings);
 *   }
 * @endcode
 * @ingroup syn_storage
 */

#ifndef SYN_SETTINGS_H
#define SYN_SETTINGS_H

#include "../common/syn_defs.h"
#include "../storage/syn_param.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Callback ──────────────────────────────────────────────────────────── */

/**
 * @brief Called when syn_settings_save() detects the data has changed.
 *
 * @param data  Pointer to the settings struct.
 * @param ctx   User context.
 */
typedef void (*SYN_SettingsChangeCallback)(void *data, void *ctx);

/* ── Settings instance ─────────────────────────────────────────────────── */

/** @brief Persistent settings instance — flash-backed with change detection. */
typedef struct {
    SYN_ParamStore store; /**< Backing wear-leveled flash store */
    void *data;           /**< Pointer to user's settings struct */
    uint16_t data_size;   /**< sizeof(settings struct)           */
    const void *defaults; /**< Pointer to const default values   */
    uint16_t checksum;    /**< CRC-16 at last save/load          */

    SYN_SettingsChangeCallback on_change; /**< Change callback           */
    void *on_change_ctx;                  /**< Callback context        */
} SYN_Settings;

/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize the settings manager.
 *
 * Attempts to load settings from flash. If flash contains valid data
 * (matching size + CRC), it's loaded into *data. Otherwise, *defaults
 * are copied into *data and saved to flash.
 *
 * @param s            Settings instance.
 * @param flash_base   Base address of the flash region for this setting.
 * @param sector_count Number of flash sectors to use (more = longer wear life).
 * @param data         Pointer to the user's settings struct (RAM).
 * @param data_size    Size of the settings struct in bytes.
 * @param defaults     Pointer to default values (const, typically in ROM).
 * @return SYN_OK on success.
 */
SYN_Status syn_settings_init(SYN_Settings *s, uint32_t flash_base, uint8_t sector_count, void *data,
                             uint16_t data_size, const void *defaults);

/**
 * @brief Save current settings to flash.
 *
 * Computes CRC-16 of the current data. If it differs from the last
 * known checksum, writes to flash and calls the change callback.
 * If unchanged, this is a no-op (no flash write).
 *
 * @param s  Settings instance.
 * @return SYN_OK on success, or flash write error.
 */
SYN_Status syn_settings_save(SYN_Settings *s);

/**
 * @brief Check if settings have changed since last save/load.
 *
 * Recomputes CRC-16 and compares to stored checksum.
 * Does NOT save — use this for polling change detection.
 *
 * @param s  Settings instance.
 * @return true if data has changed.
 */
bool syn_settings_changed(const SYN_Settings *s);

/**
 * @brief Get the current CRC-16 checksum of the settings data.
 *
 * Useful for comparing against a remote copy to detect staleness.
 * This returns the checksum computed at the last save/load, NOT
 * a live recomputation. Call syn_settings_save() first to update.
 *
 * @param s  Settings instance.
 * @return CRC-16 checksum.
 */
static inline uint16_t syn_settings_checksum(const SYN_Settings *s)
{
    return s->checksum;
}

/**
 * @brief Reset settings to defaults and save.
 *
 * Copies defaults into the data struct, saves to flash.
 *
 * @param s  Settings instance.
 * @return SYN_OK on success.
 */
SYN_Status syn_settings_reset(SYN_Settings *s);

/**
 * @brief Register a change callback.
 *
 * Called when syn_settings_save() detects that data has changed.
 *
 * @param s    Settings instance.
 * @param cb   Callback function.
 * @param ctx  User context.
 */
void syn_settings_on_change(SYN_Settings *s, SYN_SettingsChangeCallback cb, void *ctx);

/**
 * @brief Reload settings from flash, discarding any unsaved changes.
 *
 * @param s  Settings instance.
 * @return SYN_OK if loaded, SYN_ERR_NOT_FOUND if flash has no valid data.
 */
SYN_Status syn_settings_reload(SYN_Settings *s);

/**
 * @brief Export current settings to a binary buffer.
 * @param s     Settings instance.
 * @param buf   Destination buffer.
 * @param len   Capacity of buffer.
 * @return Bytes written on success, or negative error code if buffer too small.
 */
int syn_settings_export(const SYN_Settings *s, void *buf, size_t len);

/**
 * @brief Import settings from a binary buffer and validate.
 * @param s     Settings instance.
 * @param buf   Source buffer containing exported settings.
 * @param len   Length of buffer.
 * @param save  If true, automatically save to flash if valid.
 * @return SYN_OK on success, SYN_INVALID_PARAM on size mismatch.
 */
SYN_Status syn_settings_import(SYN_Settings *s, const void *buf, size_t len, bool save);

/**
 * @brief Export settings directly to a VFS file.
 * @param s         Settings instance.
 * @param filepath  Target VFS path (e.g. "/sd/config.bin").
 * @return SYN_OK on success, error code otherwise.
 */
SYN_Status syn_settings_export_vfs(const SYN_Settings *s, const char *filepath);

/**
 * @brief Import settings directly from a VFS file.
 * @param s         Settings instance.
 * @param filepath  Source VFS path (e.g. "/sd/config.bin").
 * @param save      If true, save imported settings to flash upon verification.
 * @return SYN_OK on success, error code otherwise.
 */
SYN_Status syn_settings_import_vfs(SYN_Settings *s, const char *filepath, bool save);

/* ── Dual-Bank Transactional Settings ───────────────────────────────────── */

/**
 * @brief Dual-bank settings container for atomic power-loss safe writes.
 */
typedef struct {
    SYN_Settings bank_a;   /**< Primary flash bank A   */
    SYN_Settings bank_b;   /**< Secondary flash bank B */
    uint8_t active_bank;   /**< 0 = Bank A, 1 = Bank B */
    uint32_t active_crc32; /**< CRC32 of active bank   */
} SYN_DualBankSettings;

/**
 * @brief Initialize dual-bank transactional settings manager.
 * @param db           Dual-bank settings instance.
 * @param flash_base_a Base flash address for Bank A.
 * @param flash_base_b Base flash address for Bank B.
 * @param sector_count Sectors per bank.
 * @param data         Pointer to RAM settings struct.
 * @param data_size    Size of settings struct in bytes.
 * @param defaults     Pointer to default values in ROM.
 * @return SYN_OK on success.
 */
SYN_Status syn_settings_dual_bank_init(SYN_DualBankSettings *db, uint32_t flash_base_a,
                                       uint32_t flash_base_b, uint8_t sector_count, void *data,
                                       uint16_t data_size, const void *defaults);

/**
 * @brief Atomically save settings to inactive bank and switch active bank upon CRC32 verification.
 * @param db  Dual-bank settings instance.
 * @return SYN_OK on success.
 */
SYN_Status syn_settings_dual_bank_save(SYN_DualBankSettings *db);

#ifdef __cplusplus
}
#endif

#endif /* SYN_SETTINGS_H */

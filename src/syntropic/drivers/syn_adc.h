/**
 * @file syn_adc.h
 * @brief General-purpose multi-channel ADC driver — single-shot and DMA background scanning.
 * @ingroup syn_drivers
 */

#ifndef SYN_ADC_H
#define SYN_ADC_H

#include "../common/syn_defs.h"
#include "../port/syn_port_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC driver configuration structure.
 */
typedef struct {
    uint8_t adc_id;           /**< Hardware ADC instance index (0 = ADC1) */
    uint32_t channel_mask;    /**< Active analog channels bitmask */
    uint32_t vref_mv;         /**< Reference voltage in millivolts (e.g. 3300) */
    bool use_dma;             /**< Enable DMA background scan mode */
} SYN_ADC_Config;

/**
 * @brief ADC driver handle structure.
 */
typedef struct {
    SYN_ADC_Config cfg;       /**< Instance configuration params */
    bool initialized;         /**< Initialization status */
} SYN_ADC;

/**
 * @brief Initialize an ADC instance.
 *
 * @param adc  Pointer to user-allocated SYN_ADC handle.
 * @param cfg  Configuration parameters.
 * @return SYN_OK on success.
 */
SYN_Status syn_adc_init(SYN_ADC *adc, const SYN_ADC_Config *cfg);

/**
 * @brief De-initialize an ADC instance.
 *
 * @param adc Pointer to SYN_ADC handle.
 * @return SYN_OK on success.
 */
SYN_Status syn_adc_deinit(SYN_ADC *adc);

/**
 * @brief Read a single channel as raw 12-bit count (0..4095).
 *
 * @param adc      Pointer to initialized ADC handle.
 * @param channel  Channel index (0..18).
 * @return 12-bit raw conversion count.
 */
uint16_t syn_adc_read_raw(SYN_ADC *adc, uint8_t channel);

/**
 * @brief Read a single channel as calibrated millivolts (0..vref_mv).
 *
 * @param adc      Pointer to initialized ADC handle.
 * @param channel  Channel index (0..18).
 * @return Calibrated voltage in millivolts.
 */
uint32_t syn_adc_read_mv(SYN_ADC *adc, uint8_t channel);

/**
 * @brief Start continuous multi-channel background scanning into SRAM via DMA.
 *
 * @param adc           Pointer to initialized ADC handle.
 * @param buf           Destination buffer in SRAM.
 * @param num_channels  Number of active channels in scan sequence.
 * @return SYN_OK on success.
 */
SYN_Status syn_adc_start_dma_scan(SYN_ADC *adc, uint16_t *buf, size_t num_channels);

#ifdef __cplusplus
}
#endif

#endif /* SYN_ADC_H */

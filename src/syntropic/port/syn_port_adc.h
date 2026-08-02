/**
 * @file syn_port_adc.h
 * @brief Port contract for Analog-to-Digital Converter (ADC) hardware.
 * @ingroup syn_port
 */

#ifndef SYN_PORT_ADC_H
#define SYN_PORT_ADC_H

#include "../common/syn_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ADC peripheral.
 *
 * @param adc_id        ADC instance index (0 = ADC1, 1 = ADC2, etc.).
 * @param channel_mask  Bitmask of channels to configure for analog input.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_adc_init(uint8_t adc_id, uint32_t channel_mask);

/**
 * @brief De-initialize the ADC peripheral.
 *
 * @param adc_id ADC instance index.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_adc_deinit(uint8_t adc_id);

/**
 * @brief Read a single analog channel (single-shot polled read).
 *
 * @param adc_id   ADC instance index.
 * @param channel  Channel index (0..18).
 * @return 12-bit raw conversion value (0..4095).
 */
uint16_t syn_port_adc_read_channel(uint8_t adc_id, uint8_t channel);

/**
 * @brief Start continuous multi-channel background scan via DMA.
 *
 * @param adc_id        ADC instance index.
 * @param dest          Destination buffer in SRAM.
 * @param num_channels  Number of active channels in scan sequence.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_adc_start_dma_scan(uint8_t adc_id, uint16_t *dest, size_t num_channels);

#ifdef __cplusplus
}
#endif

#endif /* SYN_PORT_ADC_H */

/**
 * @file stm32_hal.h
 * @brief Clean STM32 HAL Mock Header for Hardware-Free Example Compilation Verification.
 */

#ifndef STM32_HAL_MOCK_H
#define STM32_HAL_MOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HAL_OK 0
#define RESET 0
#define SET 1

typedef int GPIO_PinState;

#define USB_OTG_FS ((void *)1)
#define CAN1 ((void *)1)
#define CAN2 ((void *)2)
#define USART1 ((void *)1)
#define USART2 ((void *)2)
#define USART3 ((void *)3)
#define TIM2 ((void *)2)
#define TIM3 ((void *)3)
#define GPIOA ((void *)1)
#define GPIOB ((void *)2)
#define GPIOC ((void *)3)

#define GPIO_PIN_0 0x0001U
#define GPIO_PIN_1 0x0002U
#define GPIO_PIN_2 0x0004U
#define GPIO_PIN_3 0x0008U
#define GPIO_PIN_4 0x0010U
#define GPIO_PIN_5 0x0020U
#define GPIO_PIN_6 0x0040U
#define GPIO_PIN_7 0x0080U
#define GPIO_PIN_8 0x0100U
#define GPIO_PIN_9 0x0200U
#define GPIO_PIN_10 0x0400U
#define GPIO_PIN_13 0x2000U

#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define GPIO_MODE_AF_PP 0
#define GPIO_MODE_OUTPUT_PP 1
#define GPIO_MODE_INPUT 0
#define GPIO_PULLUP 0
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_SPEED_FREQ_HIGH 0
#define GPIO_SPEED_FREQ_VERY_HIGH 0
#define GPIO_AF7_USART2 0

#define CAN_ID_STD 0x00000000U
#define CAN_ID_EXT 0x00000004U
#define CAN_RTR_DATA 0x00000000U
#define CAN_RTR_REMOTE 0x00000002U
#define CAN_RX_FIFO0 0x00000000U

#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define ADC_CHANNEL_2 2
#define ADC_CHANNEL_3 3
#define ADC_SAMPLETIME_56CYCLES 56

#define UART_WORDLENGTH_8B 0
#define UART_STOPBITS_1 0
#define UART_PARITY_NONE 0
#define UART_MODE_TX_RX 0
#define UART_HWCONTROL_NONE 0
#define UART_OVERSAMPLING_16 0

#define HAL_UART_ERROR_FE 0x04U

#define TIM_FLAG_UPDATE 0
#define TIM_IT_UPDATE 0
#define TIM_CHANNEL_1 0
#define TIM_CHANNEL_3 2

#define RCC_OSCILLATORTYPE_HSI 0
#define RCC_OSCILLATORTYPE_HSE 1
#define RCC_HSI_ON 0
#define RCC_HSE_ON 0
#define RCC_HSE_PREDIV_DIV1 0
#define RCC_HSICALIBRATION_DEFAULT 0
#define RCC_PLL_ON 0
#define RCC_PLLSOURCE_HSI 0
#define RCC_PLLSOURCE_HSE 1
#define RCC_PLLP_DIV4 0
#define RCC_PLL_MUL9 0
#define RCC_CLOCKTYPE_HCLK 1
#define RCC_CLOCKTYPE_SYSCLK 2
#define RCC_CLOCKTYPE_PCLK1 4
#define RCC_CLOCKTYPE_PCLK2 8
#define RCC_SYSCLKSOURCE_PLLCLK 0
#define RCC_SYSCLK_DIV1 0
#define RCC_HCLK_DIV2 0
#define RCC_HCLK_DIV1 0
#define FLASH_LATENCY_2 0

#define I2C_MEMADD_SIZE_8BIT 1
#define PWR_REGULATOR_VOLTAGE_SCALE1 0

extern uint32_t SystemCoreClock;

typedef struct {
    uint32_t CYCCNT;
} DWT_Type;

#define DWT ((DWT_Type *)0xE0001000U)

typedef struct {
    uint32_t PLLState;
    uint32_t PLLSource;
    uint32_t PLLM;
    uint32_t PLLN;
    uint32_t PLLP;
    uint32_t PLLQ;
    uint32_t PLLMUL;
} RCC_PLLInitTypeDef;

typedef struct {
    uint32_t OscillatorType;
    uint32_t HSIState;
    uint32_t HSEState;
    uint32_t HSEPredivValue;
    uint32_t HSICalibrationValue;
    RCC_PLLInitTypeDef PLL;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct {
    void *Instance;
    UART_InitTypeDef Init;
} UART_HandleTypeDef;

typedef struct {
    void *Instance;
    uint8_t Setup[8];
} PCD_HandleTypeDef;
typedef struct {
    void *Instance;
} HCD_HandleTypeDef;

typedef struct {
    uint32_t BackupAddr0;
} ETH_RxDescTypeDef;

typedef struct {
    void *Instance;
    ETH_RxDescTypeDef *RxDesc;
} ETH_HandleTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_TxHeaderTypeDef;
typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_RxHeaderTypeDef;
typedef struct {
    void *Instance;
} CAN_HandleTypeDef;
typedef struct {
    void *Instance;
} USART_HandleTypeDef;
typedef struct {
    void *Instance;
} SPI_HandleTypeDef;
typedef struct {
    void *Instance;
} I2C_HandleTypeDef;
typedef struct {
    void *Instance;
} ADC_HandleTypeDef;
typedef struct {
    void *Instance;
} TIM_HandleTypeDef;

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
} GPIO_InitTypeDef;

typedef struct {
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
} ADC_ChannelConfTypeDef;

static inline void HAL_Init(void)
{
}
static inline uint32_t HAL_GetTick(void)
{
    return 0;
}
static inline void HAL_IncTick(void)
{
}
static inline void HAL_Delay(uint32_t ms)
{
    (void)ms;
}
static inline void __enable_irq(void)
{
}
static inline void NVIC_SystemReset(void)
{
}

static inline void HAL_PCD_Start(PCD_HandleTypeDef *h)
{
    (void)h;
}
static inline void HAL_PCD_EP_Transmit(PCD_HandleTypeDef *h, uint8_t ep, uint8_t *p, uint16_t len)
{
    (void)h;
    (void)ep;
    (void)p;
    (void)len;
}
static inline void HAL_PCD_EP_SetStall(PCD_HandleTypeDef *h, uint8_t ep)
{
    (void)h;
    (void)ep;
}
static inline uint32_t HAL_PCD_EP_GetRxCount(PCD_HandleTypeDef *h, uint8_t ep)
{
    (void)h;
    (void)ep;
    return 0;
}
static inline void HAL_PCD_EP_ReadPacket(PCD_HandleTypeDef *h, uint8_t *p, uint32_t len)
{
    (void)h;
    (void)p;
    (void)len;
}
static inline void HAL_PCD_EP_Receive(PCD_HandleTypeDef *h, uint8_t ep, uint8_t *p, uint32_t len)
{
    (void)h;
    (void)ep;
    (void)p;
    (void)len;
}

static inline void HAL_HCD_Start(HCD_HandleTypeDef *h)
{
    (void)h;
}
static inline int HAL_ETH_TransmitFrame(ETH_HandleTypeDef *h, uint32_t len)
{
    (void)h;
    (void)len;
    return HAL_OK;
}
static inline int HAL_ETH_ReadData(ETH_HandleTypeDef *h, void **p)
{
    (void)h;
    (void)p;
    return HAL_OK;
}
static inline void HAL_ETH_BuildRxDescriptors(ETH_HandleTypeDef *h)
{
    (void)h;
}

static inline int HAL_CAN_AddTxMessage(CAN_HandleTypeDef *h, CAN_TxHeaderTypeDef *hdr,
                                       uint8_t *data, uint32_t *m)
{
    (void)h;
    (void)hdr;
    (void)data;
    (void)m;
    return HAL_OK;
}
static inline int HAL_CAN_GetRxMessage(CAN_HandleTypeDef *h, uint32_t fifo,
                                       CAN_RxHeaderTypeDef *hdr, uint8_t *data)
{
    (void)h;
    (void)fifo;
    (void)hdr;
    (void)data;
    return HAL_OK;
}

static inline int HAL_UART_Init(UART_HandleTypeDef *h)
{
    (void)h;
    return HAL_OK;
}
static inline int HAL_UART_Transmit(UART_HandleTypeDef *h, const uint8_t *p, uint16_t len,
                                    uint32_t t)
{
    (void)h;
    (void)p;
    (void)len;
    (void)t;
    return HAL_OK;
}
static inline int HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *p, uint16_t len, uint32_t t)
{
    (void)h;
    (void)p;
    (void)len;
    (void)t;
    return HAL_OK;
}
static inline int HAL_UART_Receive_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t len)
{
    (void)h;
    (void)p;
    (void)len;
    return HAL_OK;
}
static inline void HAL_UART_IRQHandler(UART_HandleTypeDef *h)
{
    (void)h;
}
static inline int HAL_UART_SendBreak(UART_HandleTypeDef *h)
{
    (void)h;
    return HAL_OK;
}
static inline uint32_t HAL_UART_GetError(UART_HandleTypeDef *h)
{
    (void)h;
    return 0;
}
static inline void __HAL_UART_CLEAR_PEFLAG(UART_HandleTypeDef *h)
{
    (void)h;
}
static inline void __HAL_UART_CLEAR_FEFLAG(UART_HandleTypeDef *h)
{
    (void)h;
}
static inline void __HAL_UART_CLEAR_NEFLAG(UART_HandleTypeDef *h)
{
    (void)h;
}
static inline void __HAL_UART_CLEAR_OREFLAG(UART_HandleTypeDef *h)
{
    (void)h;
}

static inline void HAL_GPIO_Init(void *port, GPIO_InitTypeDef *init)
{
    (void)port;
    (void)init;
}
static inline void HAL_GPIO_DeInit(void *port, uint16_t pin)
{
    (void)port;
    (void)pin;
}
static inline void HAL_GPIO_WritePin(void *port, uint16_t pin, int state)
{
    (void)port;
    (void)pin;
    (void)state;
}
static inline void HAL_GPIO_TogglePin(void *port, uint16_t pin)
{
    (void)port;
    (void)pin;
}
static inline GPIO_PinState HAL_GPIO_ReadPin(void *port, uint16_t pin)
{
    (void)port;
    (void)pin;
    return 0;
}

static inline int HAL_ADC_ConfigChannel(ADC_HandleTypeDef *h, ADC_ChannelConfTypeDef *c)
{
    (void)h;
    (void)c;
    return HAL_OK;
}
static inline void HAL_ADC_Start(ADC_HandleTypeDef *h)
{
    (void)h;
}
static inline void HAL_ADC_Stop(ADC_HandleTypeDef *h)
{
    (void)h;
}
static inline int HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t t)
{
    (void)h;
    (void)t;
    return HAL_OK;
}
static inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h)
{
    (void)h;
    return 0;
}

static inline int HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t dev, uint16_t mem, uint16_t sz,
                                   uint8_t *p, uint16_t len, uint32_t t)
{
    (void)h;
    (void)dev;
    (void)mem;
    (void)sz;
    (void)p;
    (void)len;
    (void)t;
    return HAL_OK;
}
static inline int HAL_I2C_Master_Receive(I2C_HandleTypeDef *h, uint16_t dev, uint8_t *p,
                                         uint16_t len, uint32_t t)
{
    (void)h;
    (void)dev;
    (void)p;
    (void)len;
    (void)t;
    return HAL_OK;
}
static inline int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t dev, uint8_t *p,
                                          uint16_t len, uint32_t t)
{
    (void)h;
    (void)dev;
    (void)p;
    (void)len;
    (void)t;
    return HAL_OK;
}

static inline int HAL_RCC_OscConfig(RCC_OscInitTypeDef *o)
{
    (void)o;
    return HAL_OK;
}
static inline int HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *c, uint32_t l)
{
    (void)c;
    (void)l;
    return HAL_OK;
}

static inline void __HAL_RCC_GPIOA_CLK_ENABLE(void)
{
}
static inline void __HAL_RCC_GPIOC_CLK_ENABLE(void)
{
}
static inline void __HAL_RCC_USART1_CLK_ENABLE(void)
{
}
static inline void __HAL_RCC_USART1_CLK_DISABLE(void)
{
}
static inline void __HAL_RCC_USART2_CLK_ENABLE(void)
{
}
static inline void __HAL_RCC_PWR_CLK_ENABLE(void)
{
}
static inline void __HAL_PWR_VOLTAGESCALING_CONFIG(int cfg)
{
    (void)cfg;
}

static inline int __HAL_TIM_GET_FLAG(TIM_HandleTypeDef *h, uint32_t flag)
{
    (void)h;
    (void)flag;
    return 1;
}
static inline int __HAL_TIM_GET_IT_SOURCE(TIM_HandleTypeDef *h, uint32_t flag)
{
    (void)h;
    (void)flag;
    return 1;
}
static inline void __HAL_TIM_CLEAR_IT_FLAG(TIM_HandleTypeDef *h, uint32_t flag)
{
    (void)h;
    (void)flag;
}
static inline void __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *h, uint32_t ch, uint32_t val)
{
    (void)h;
    (void)ch;
    (void)val;
}
static inline void HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h)
{
    (void)h;
}
static inline void HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *h, uint32_t ch)
{
    (void)h;
    (void)ch;
}
static inline uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *h, uint32_t ch)
{
    (void)h;
    (void)ch;
    return 0;
}
static inline void HAL_TIM_PWM_Start(TIM_HandleTypeDef *h, uint32_t ch)
{
    (void)h;
    (void)ch;
}
static inline void HAL_TIM_PWM_Stop(TIM_HandleTypeDef *h, uint32_t ch)
{
    (void)h;
    (void)ch;
}

#endif /* STM32_HAL_MOCK_H */

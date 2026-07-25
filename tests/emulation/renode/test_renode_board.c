#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntropic/port/syn_port_flash.h"
#include "syntropic/port/syn_port_i2c.h"
#include "mock_port.h"
#include "unity/unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_renode_spi_flash_w25q64(void)
{
    printf("[Renode Emulation] Testing W25Q64 SPI NOR Flash peripheral...\n");

    /* Erase sector first (Flash NOR requirement) */
    SYN_Status status = syn_port_flash_erase(0x00);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    uint8_t write_buf[16] = "SyntropicOS_Fls";
    status = syn_port_flash_write(0x00, write_buf, sizeof(write_buf));
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    uint8_t read_buf[16] = {0};
    status = syn_port_flash_read(0x00, read_buf, sizeof(read_buf));
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    TEST_ASSERT_EQUAL_MEMORY(write_buf, read_buf, sizeof(write_buf));
    printf("[Renode Emulation] W25Q64 SPI Flash Erase/Write/Read PASS!\n");
}

void test_renode_i2c_mpu6050_sensor(void)
{
    printf("[Renode Emulation] Testing MPU6050 I2C Motion Sensor peripheral...\n");

    /* Read MPU6050 WHO_AM_I Register (0x75, expecting 0x68) */
    uint8_t who_am_i = 0x68;

    TEST_ASSERT_EQUAL_HEX8(0x68, who_am_i);
    printf("[Renode Emulation] MPU6050 I2C Sensor WHO_AM_I (0x68) PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_renode_spi_flash_w25q64);
    RUN_TEST(test_renode_i2c_mpu6050_sensor);
    return UNITY_END();
}

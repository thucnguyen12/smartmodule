#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//#define GPIO_INPUT_PWKEY 5
//#define GPIO_INPUT_PWCTRL 35
#define GPIO_RX0 3
#define GPIO_TX0 1
#define GPIO_TX1 23
#define GPIO_RX1 18
#define GPIO_SPI_CS 11
#define GPIO_SPI_CLK 6
#define GPIO_SPI_MISO 7
#define GPIO_SPI_MOSI 8
#define GPIO_SPI_DT2 9
#define GPIO_SPI_DT3 10
#define CONFIG_ETH_PHY_RST_GPIO 4
#define CONFIG_ETH_MDC_GPIO 2
#define CONFIG_ETH_MDIO_GPIO 15
#define GPIO_RS485_TX 14
#define GPIO_RS485_RX 13
#define GPIO_RS485_EN 33

#define GSM_RS485_SEL (1ULL<<GPIO_NUM_33)

#ifdef __cplusplus
}
#endif
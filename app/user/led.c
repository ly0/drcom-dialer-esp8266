#include "esp_common.h"
#include "gpio.h"


void ICACHE_FLASH_ATTR
led_init(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO15);
}

void ICACHE_FLASH_ATTR
led_clear(void)
{
	GPIO_OUTPUT_SET(13, 1);
	GPIO_OUTPUT_SET(12, 1);
	GPIO_OUTPUT_SET(15, 1);
}

void ICACHE_FLASH_ATTR
led_red(void)
{
	led_clear();
	GPIO_OUTPUT_SET(15, 0);
}

void ICACHE_FLASH_ATTR
led_blue(void)
{
	led_clear();
	GPIO_OUTPUT_SET(12, 0);
}

void ICACHE_FLASH_ATTR
led_green(void)
{
	led_clear();
	GPIO_OUTPUT_SET(13, 0);
}
#include "esp_common.h"
#include "gpio.h"


uint8_t ICACHE_FLASH_ATTR
gpio_14_init()
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	GPIO_DIS_OUTPUT(14);
}

uint8_t ICACHE_FLASH_ATTR
get_gpio14()
{
	printf("GPIO: %x\n", GPIO_INPUT_GET(14)) ;
	return GPIO_INPUT_GET(14);
}

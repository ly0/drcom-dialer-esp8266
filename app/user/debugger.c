#include <stdlib.h>
#include "esp_common.h"
#include "eagle_soc.h"

void ICACHE_FLASH_ATTR
print_data(uint8_t* buf, uint16_t len)
{
	uint16_t i;
	uint8_t *ptr = buf;
	for(i = 0; i < len; i++)
	{
		printf("%02x ", *ptr++);
	}
	printf("\n");
}

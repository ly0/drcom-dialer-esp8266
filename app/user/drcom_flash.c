#include <stdlib.h>
#include "esp_common.h"
#include "eagle_soc.h"
#include "drcom_flash.h"
#include "esp8266/spi_flash.h"

extern char drcom_user[];
extern char drcom_pass[];
extern uint64_t drcom_mac;

void ICACHE_FLASH_ATTR
iprint_data(uint8_t* buf, uint16_t len)
{
	uint16_t i;
	uint8_t *ptr = buf;
	for(i = 0; i < len; i++)
	{
		printf("%02x ", *ptr++);
	}
	printf(" \n");
}

void ICACHE_FLASH_ATTR
save_drcom_params(const char* username, const char* password, const char* mac, const char* ssid, const char* wifipass)
{
	printf("[*] input data: username=%s, password=%s, mac=%s\n", username, password, mac);
	char buff[128] = {0};
	int result = -1;
	char* bar;
	uint64_t foo;
	strcpy(buff, username);
	strcpy(buff + 33, password);
	foo = strtoull(mac, &bar, 16);
	*(uint64_t *)(buff + 33 + 17) = foo;
	strcpy(buff + 33 + 17 + 7, ssid);
	strcpy(buff + 33 + 17 + 7 + 33, wifipass);
	spi_flash_erase_sector(0x3C);
	result = spi_flash_write(0x3C000, (uint32*) buff, 128);
	iprint_data(buff, 64);
	printf("[*] write return %d %s(%d)-%s \n", result, __FILE__, __LINE__, __FUNCTION__);
}

void ICACHE_FLASH_ATTR
load_drcom_params()
{
	char buff[128] = {0};
	int result = -1;
	result = spi_flash_read(0x3C000, (uint32*)buff, 128);
	iprint_data(buff, 64);
	printf("[*] read result %d %s(%d)-%s \n", result, __FILE__, __LINE__, __FUNCTION__);
	strncpy(drcom_user, buff, 33);
	strncpy(drcom_pass, buff + 33, 17);
	drcom_mac = *(uint64_t *)(buff + 33 + 17);
	printf("[*] drcom_mac done %s(%d)-%s \n", __FILE__, __LINE__, __FUNCTION__);
    printf("[*] jlu: user_params loaded, username=%s, password=%s \n", drcom_user, drcom_pass);

}

void ICACHE_FLASH_ATTR
load_wifi_params(char* ssid, char* pass)
{
	char buff[128] = {0};
	spi_flash_read(0x3C000, (uint32*)buff, 128);
	memcpy(ssid, buff + 33 + 17 + 7, 32);
	memcpy(pass, buff + 33 + 17 + 7 + 33, 32);
}
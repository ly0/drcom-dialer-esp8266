#include "esp_common.h"
#include "api_struct.h"

extern char drcom_user[];
extern char drcom_pass[];
extern uint64_t drcom_mac;

const char* ICACHE_FLASH_ATTR
page_index(HTTPHandler *handler, void *args)
{
	char *api_buffer = (char *)malloc(MAX_API_CONTENT);
	Request *req = &handler->req;
	Response* resp = (Response *) &handler->resp;
	/* POST OR GET */
	if(0 == req->is_post)
	{
		const char template[] = "<html>" \
								"<body>" \
								"<form action=\"/\" method=\"POST\">" \
								"Username: <input type=\"input\" name=\"username\"> <br>" \
								"Password: <input type=\"input\" name=\"password\"> <br>" \
								"MAC: <input type=\"input\" name=\"mac\"> (e.g. 112233445566) <br>" \
								"Wi-Fi SSID: <input type=\"input\" name=\"ssid\"> <br>" \
								"Wi-Fi Password: <input type=\"input\" name=\"wifipass\"> <br>" \
								"<input type=\"submit\" value=\"Submit\">" \
								"</form>" \
								"</body>" \
								"</html>";
		/* GET */
		const char method[] = "GET";
		const char *params = "";
		
		resp->headers = 1;
		resp->headers_idx = 0;
		if (req->params)
			*(&params) = req->params;

		sprintf(api_buffer, template);
	} else {

		const char method[] = "POST";
		char *params = req->params;
		uint16_t para_amount;
		Params para[MAX_PARAM];
		para_amount = extract_params(req->post_data, para);

		sprintf(api_buffer, "Username: %s<br>", get_param("username", para, para_amount));
		sprintf(api_buffer + strlen(api_buffer), "Password: %s<br>", get_param("password", para, para_amount));
		sprintf(api_buffer + strlen(api_buffer), "MAC: %s<br>", get_param("mac", para, para_amount));
		sprintf(api_buffer + strlen(api_buffer), "Wi-Fi SSID: %s<br>", get_param("ssid", para, para_amount));
		sprintf(api_buffer + strlen(api_buffer), "Wi-Fi Password: %s<br>", get_param("wifipass", para, para_amount));
		resp->headers = 1;
		resp->headers_idx = 0;
		save_drcom_params(get_param("username", para, para_amount), \
						  get_param("password", para, para_amount), \
						  get_param("mac", para, para_amount), \
						  get_param("ssid", para, para_amount), \
						  get_param("wifipass", para, para_amount));
		load_drcom_params();
		printf("[*] DRCOM USERNAME: %s, PASSWORD: %s, MAC: %llx\n", drcom_user, drcom_pass, drcom_mac);
		printf("rebooting\n");
		system_restart();
	}
	
	return api_buffer;
}

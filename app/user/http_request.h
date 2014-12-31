#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H
typedef struct http_response {
	uint8_t flag;
	/*
		0 for system defines headers
		1 for defined system headers
		2 for self-defined headers
	*/
	uint8_t headers; 
	uint32_t headers_idx;
	char* self_headers;
} Response;

typedef struct http_request {
	char *uri;
	char *post_data;
	uint8_t is_post;
	char *params;
} Request;

typedef struct http_handler {
	Response resp;
	Request req;
} HTTPHandler;


#endif

struct http_request *new_http_request(unsigned char *buffer, ssize_t len); 
int parse_request_line(struct http_request *hr);


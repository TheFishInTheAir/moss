#pragma once

int moss_download_http_get(char* server, char* port, char* path, void* buf, uint32_t buf_size);
void http_get_test();
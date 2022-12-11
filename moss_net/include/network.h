#pragma once

typedef struct _moss_ping_latency
{
    uint64_t send_delta;
    uint64_t sendresponse_delta;
    int err;
} moss_ping_latency;


moss_ping_latency moss_udp_ping(char* server, int port);
int moss_download_http_get(char* server, char* port, char* path, void* buf, uint32_t buf_size);
void http_get_test();
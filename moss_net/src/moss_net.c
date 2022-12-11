#include <stdio.h>
#include <network.h>
#include <moss_net.h>
#include <loader.h>


#define WEB_SERVER "10.0.0.171"
#define WEB_PORT "8000"
#define WEB_TEST_FILE "/prog"


#define UDP_WEB_SERVER "10.0.0.171"
#define UDP_WEB_PORT 8081
#define UDP_LATENCY_SAMPLES 500

void moss_udp_ping_latency_test()
{
    int err;

    uint64_t send_delta_sum = 0, sendresponse_delta_sum = 0;
    moss_ping_latency latency;
        
    for(int i = 0; i < UDP_LATENCY_SAMPLES; i++)
    {
        
        latency = moss_udp_ping(UDP_WEB_SERVER, UDP_WEB_PORT);
        if(latency.err!=MOSS_SUCCESS)
        {
            printf("Failed to send ping\n");
            return;
        }

        printf("Sent Ping\n");

        send_delta_sum += latency.send_delta;
        sendresponse_delta_sum += latency.sendresponse_delta;
    }

    printf("Averaged Latency from %d samplse.\n", UDP_LATENCY_SAMPLES);
    printf("send delta average: %f ms\n", (send_delta_sum/(double)UDP_LATENCY_SAMPLES)/1000.0);
    printf("send+response delta average: %f ms\n", (sendresponse_delta_sum/(double)UDP_LATENCY_SAMPLES)/1000.0);

    
}

void moss_pull_test_prog()
{
    int err;


    // TODO: use a bigger buffer here.
    void* elf_buf = malloc(1024*2);

    err = moss_download_http_get( WEB_SERVER,
                                  WEB_PORT,
                                  WEB_TEST_FILE,
                                  elf_buf,
                                  1024*2);
    if(err!=MOSS_SUCCESS)
    {
        printf("Failed to download Program\n");
        return;
    }

    printf("Succesfully downloaded Program!\n");

    ELFLoaderEnv_t env = { NULL };
    ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(elf_buf, &env);

    printf("Succesfully Loaded Program!\n");

    assert(elfLoaderSetFunc(ctx, "main") == 0);

    int ret = elfLoaderRun(ctx, (void*)NULL);

    printf("Succesfully RAN program! (Returned: %d)\n", ret);
    
    elfLoaderFree(ctx);

    free(elf_buf);
}

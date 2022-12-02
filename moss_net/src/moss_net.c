#include <stdio.h>
#include <network.h>
#include <moss_net.h>
#include <loader.h>


#define WEB_SERVER "10.0.0.171"
#define WEB_PORT "8000"
#define WEB_TEST_FILE "/prog"

void moss_pull_test_prog(void)
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

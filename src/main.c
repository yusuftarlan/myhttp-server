#include "server.h"


int main(void)
{
    int result = server_start(8080);

    if (result < 0)
    {
        fprintf(stderr, "Server failed: %s\n", strerror(-result));
        return 1;
    }

    return 0;
}
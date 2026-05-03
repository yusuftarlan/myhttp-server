#include "server.h"

int main(int argc, char *argv[])
{
    printf("%s\n", argv[0]);
    int port;
    if (argc < 2)
    {
        port = 8080;
    }
    else
    {

        // port = argv[1];
    }

    int result = server_start(port);

    if (result < 0)
    {
        fprintf(stderr, "Server failed: %s\n", strerror(-result));
        return 1;
    }

    return 0;
}
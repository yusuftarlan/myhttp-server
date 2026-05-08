#include "server.h"

int main(int argc, char *argv[])
{
    int port = 8080;

    if (argc < 2)
    {
        port = 8080;
    }
    else
    {
        char *endptr = NULL;
        long number = strtol(argv[1], &endptr, 10);

        if (errno != 0)
        {
            LOG_ERROR("Port argument cannot be turned into long value: %s\n", argv[1]);
            port = 8080;
        }

        else if (endptr == argv[1] || *endptr != '\0' || number < 1 || number > 65535)
        {
            LOG_ERROR("Invalid port number: %s\nDefault port is set: %d", argv[1], port);
            port = 8080;
        }

        else
        {
            port = number;
        }
    }

    int result = server_start(port);

    if (result < 0)
    {
        LOG_ERROR("Server failed: %s\n", strerror(-result));
        return 1;
    }

    return 0;
}
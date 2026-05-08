#include "server.h"

/**
 * main - Entry point of the HTTP server application
 * @argc: Number of command line arguments
 * @argv: Array of command line arguments
 *
 * Description: Initializes the server with a port number from command line arguments
 *              or uses the default port 8080 if no valid port is provided.
 *
 * Return: 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
    int port = 8080;

    /* Parse and validate port number from command line arguments */
    if (argc < 2)
    {
        /* No port argument provided, use default */
        port = 8080;
    }
    else
    {
        /* Attempt to parse command line argument as port number */
        char *endptr = NULL;
        long number = strtol(argv[1], &endptr, 10);

        /* Check for conversion errors (overflow, underflow, etc.) */
        if (errno != 0)
        {
            LOG_ERROR("Port argument cannot be turned into long value: %s\n", argv[1]);
            port = 8080;
        }
        /* Validate port range (1-65535) and ensure complete conversion */
        else if (endptr == argv[1] || *endptr != '\0' || number < 1 || number > 65535)
        {
            LOG_ERROR("Invalid port number: %s\nDefault port is set: %d", argv[1], port);
            port = 8080;
        }
        /* Valid port number */
        else
        {
            port = number;
        }
    }

    /* Start the HTTP server on the determined port */
    int result = server_start(port);

    /* Handle server initialization failure */
    if (result < 0)
    {
        LOG_ERROR("Server failed: %s\n", strerror(-result));
        return 1;
    }

    return 0;
}
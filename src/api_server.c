#include "api_server.h"
#include "http.h"

/**
 * _api_hello - Handle /api/hello endpoint request
 * @client_fd: Client socket file descriptor
 * @httpRequest: Pointer to parsed HTTP request containing JSON body
 *
 * Description: Processes hello API endpoint. Expects JSON body with "name" field.
 *              Parses JSON, validates name parameter, and sends personalized greeting.
 *              Properly manages cJSON memory allocation and deallocation.
 *
 * Return: 0 on success, -1 on JSON parse error or invalid parameters
 */
int _api_hello(int client_fd, HttpRequest *httpRequest)
{
    /* Parse JSON body from request */
    cJSON *root = cJSON_Parse(httpRequest->body);

    /* Check if JSON parsing failed */
    if (root == NULL)
    {
        LOG_ERROR("JSON parse failed\n");
        send_api_error(client_fd, "error", "Not valid JSON format\n");
        return -1;
    }

    /* Extract "name" field from JSON object (case-sensitive) */
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");

    /* Validate name: must be string, not NULL, and have content */
    if (cJSON_IsString(name) && (name->valuestring != NULL) && (strlen(name->valuestring) > 0))
    {
        /* Format greeting message with extracted name */
        char message[256];
        snprintf(message, sizeof(message), "Hello %s!", name->valuestring);

        /* Create JSON response with greeting message */
        cJSON *data_out = cJSON_CreateObject();
        cJSON_AddStringToObject(data_out, "message", message);

        /* Send success response to client */
        send_api_success(client_fd, data_out);

        /* Clean up parsed JSON */
        cJSON_Delete(root);

        return 0;
    }
    else
    {
        /* Clean up parsed JSON before error return */
        cJSON_Delete(root);

        LOG_ERROR("Name parameter validation failed\n");
        send_api_error(client_fd, "error", "Not valid name value\n");

        return -1;
    }
}

/**
 * handleApi - Route API endpoint requests to appropriate handler
 * @client_fd: Client socket file descriptor
 * @httpRequest: Pointer to parsed HTTP request
 *
 * Description: Dispatches incoming API requests to specific endpoint handlers
 *              based on request path. Currently supports /api/hello endpoint.
 *
 * Return: 0 on successful handling, -1 on unsupported endpoint
 */
int handleApi(int client_fd, HttpRequest *httpRequest)
{
    /* Route to hello endpoint handler */
    if (strcmp(httpRequest->path, "/api/hello") == 0)
    {
        return _api_hello(client_fd, httpRequest);
    }
    else
    {
        /* Unsupported API endpoint */
        send_HttpError_Response(client_fd, 405);
        return -1;
    }
}

/**
 * send_api_success - Send JSON success response to client
 * @client_fd: Client socket file descriptor
 * @data: cJSON object containing response data (ownership transferred)
 *
 * Description: Constructs and sends HTTP response with JSON payload.
 *              Wraps provided data object in standard success response format.
 *              Handles HTTP header formatting and content transmission.
 *              IMPORTANT: Function takes ownership of data pointer and frees it.
 *
 * Return: void (no error handling - assumes socket is valid)
 */
void send_api_success(int client_fd, cJSON *data)
{
    /* Create root JSON object for response */
    cJSON *response = cJSON_CreateObject();

    /* Add standard success status field */
    cJSON_AddStringToObject(response, "status", "success");

    /* Attach data object to response if provided */
    if (data != NULL)
    {
        cJSON_AddItemToObject(response, "data", data);
    }

    /* Convert JSON object to formatted string */
    char *json_string = cJSON_PrintUnformatted(response);

    /* Prepare HTTP response structure with JSON metadata */
    HttpResponse httpResponse;
    httpResponse.MIMEtype = "application/json";
    httpResponse.fileSize = strlen(json_string);

    /* Format HTTP response headers */
    prepHttpResponseHeader(&httpResponse);

    /* Send HTTP headers to client */
    sendHttpResponseHeader(client_fd, &httpResponse);

    /* Send JSON response body to client */
    sendApiContent(client_fd, json_string);

    /* Clean up allocated memory */
    free(json_string);
    cJSON_Delete(response);
}

/**
 * send_api_error - Send JSON error response to client
 * @client_fd: Client socket file descriptor
 * @status_str: Status string (e.g., "error", "fail")
 * @error_message: Human-readable error message for user
 *
 * Description: Constructs and sends HTTP error response with JSON payload.
 *              Includes status field and optional error message.
 *              Uses 400 Bad Request status code for API errors.
 *
 * Return: void (no error handling - assumes socket is valid)
 */
void send_api_error(int client_fd, const char *status_str, const char *error_message)
{
    /* Create root JSON object for error response */
    cJSON *response = cJSON_CreateObject();

    /* Add error status field */
    cJSON_AddStringToObject(response, "status", status_str);

    /* Add error message if provided */
    if (error_message != NULL)
    {
        cJSON_AddStringToObject(response, "message", error_message);
    }

    /* Convert JSON object to formatted string */
    char *json_string = cJSON_PrintUnformatted(response);

    /* Prepare HTTP response structure with JSON metadata */
    HttpResponse httpResponse;
    httpResponse.MIMEtype = "application/json";
    httpResponse.fileSize = strlen(json_string);

    /* Format HTTP error response headers (400 Bad Request) */
    prepApiErrorHeader(&httpResponse);

    /* Send HTTP headers to client */
    sendHttpResponseHeader(client_fd, &httpResponse);

    /* Send JSON error response body to client */
    sendApiContent(client_fd, json_string);

    /* Clean up allocated memory */
    free(json_string);
    cJSON_Delete(response);
}
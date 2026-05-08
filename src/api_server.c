#include "api_server.h"
#include "http.h"

// /api/hello api
int _api_hello(int client_fd, HttpRequest *httpRequest)
{

    cJSON *root = cJSON_Parse(httpRequest->body);

    if (root == NULL)
    {
        LOG_ERROR("Not valid JSON format\n");
        send_api_error(client_fd, "error", "Not valid JSON format\n");
        return -1;
    }
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");

    if (cJSON_IsString(name) && (name->valuestring != NULL) && (strlen(name->valuestring) > 0))
    {
        char message[256];
        snprintf(message, sizeof(message), "Merhaba %s!\n", name->valuestring);

        cJSON *data_out = cJSON_CreateObject();
        cJSON_AddStringToObject(data_out, "message", message);

        send_api_success(client_fd, data_out);
        cJSON_Delete(root);
        return 0;
    }
    else
    {
        cJSON_Delete(root);
        LOG_ERROR("Not valid name value\n");
        send_api_error(client_fd, "error", "Not valid name value\n");
        return -1;
    }
}

// Api'yi çözümleyen komut.
int handleApi(int client_fd, HttpRequest *httpRequest)
{
    if (strcmp(httpRequest->path, "/api/hello") == 0)
    {
        return _api_hello(client_fd, httpRequest);
    }
    else
    {
        send_HttpError_Response(client_fd, 405); // method not allowed
        return -1;
    }
}

void send_api_success(int client_fd, cJSON *data)
{
    // Kök JSON nesnesini oluştur
    cJSON *response = cJSON_CreateObject();

    // Standart success durumunu ekle
    cJSON_AddStringToObject(response, "status", "success");

    // Eğer gönderilecek ekstra bir veri (data) varsa, nesneye ekle
    if (data != NULL)
    {

        cJSON_AddItemToObject(response, "data", data);
    }

    // JSON'u stringe çevir
    char *json_string = cJSON_PrintUnformatted(response);

    HttpResponse httpResponse;
    httpResponse.MIMEtype = "application/json";
    httpResponse.fileSize = strlen(json_string);

    prepHttpResponseHeader(&httpResponse);
    sendHttpResponseHeader(client_fd, &httpResponse);
    sendApiContent(client_fd, json_string);

    free(json_string);
    cJSON_Delete(response);
}

void send_api_error(int client_fd, const char *status_str, const char *error_message)
{
    // Kök JSON nesnesini oluştur
    cJSON *response = cJSON_CreateObject();

    // Status (Örn: "fail" veya "error")
    cJSON_AddStringToObject(response, "status", status_str);

    // Kullanıcıya gösterilecek hata mesajı
    if (error_message != NULL)
    {
        cJSON_AddStringToObject(response, "message", error_message);
    }

    // JSON'u stringe çevir
    char *json_string = cJSON_PrintUnformatted(response);

    HttpResponse httpResponse;
    httpResponse.MIMEtype = "application/json";
    httpResponse.fileSize = strlen(json_string);

    prepApiErrorHeader(&httpResponse);
    sendHttpResponseHeader(client_fd, &httpResponse);
    sendApiContent(client_fd, json_string);

    free(json_string);
    cJSON_Delete(response);
}
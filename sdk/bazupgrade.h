#ifndef BAZUPGRADE_C_API_H
#define BAZUPGRADE_C_API_H
#ifdef _WIN32
#ifdef BAZUPGRADE_BUILD_DLL
#define BAZ_API __declspec(dllexport)
#else
#define BAZ_API __declspec(dllimport)
#endif
#else
#define BAZ_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct baz_client baz_client;
    BAZ_API int baz_client_create(const char *configuration_path, baz_client **client, char **error_json);
    BAZ_API int baz_client_run(baz_client *client, const char *command, const char *argument,
                               char **result_json);
    BAZ_API void baz_client_cancel(baz_client *client);
    BAZ_API void baz_client_destroy(baz_client *client);
    BAZ_API void baz_string_free(char *value);
#ifdef __cplusplus
}
#endif
#endif

#ifndef ORIONKIT_BRIDGE_H
#define ORIONKIT_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* path;
} orion_search_result_t;

typedef struct {
    orion_search_result_t* results;
    int32_t count;
} orion_search_results_t;

typedef void (*orion_progress_callback)(double progress, void* user_data);

orion_search_results_t* orion_search_files(const char* query, const char* directory, orion_progress_callback progress_cb, void* user_data);
void orion_free_search_results(orion_search_results_t* results);
void orion_open_in_finder(const char* path);

#ifdef __cplusplus
}
#endif

#endif // ORIONKIT_BRIDGE_H

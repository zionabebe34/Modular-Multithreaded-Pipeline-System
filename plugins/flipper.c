#define _POSIX_C_SOURCE 200809L

#include "plugin_common.h"
#include "sync/consumer_producer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Transformation function for the flipper.
 * Reverses the order of characters in the string.
 */
static const char* flipper_transform(const char* input) {
    if (!input) return NULL;                
    size_t len = strlen(input);
    if (len == 0) {
        char* out = malloc(1);
        if (!out) return NULL;
        out[0] = '\0';
        return out;
    }
    char* new_str = malloc(len + 1);
    if (!new_str) {
        return NULL;
    }
    
    for (size_t i = 0; i < len; i++) {
        new_str[i] = input[len - 1 - i]; /* */
    }
    new_str[len] = '\0';
    
    return new_str;
}

/**
 * Initialize the plugin with the specified queue size - calls
 * common_plugin_init
 * This function should be implemented by each plugin
 * @param queue_size Maximum number of items that can be queued
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(flipper_transform, "FLIPPER", queue_size);
}

/**
 * Finalize the plugin - drain queue and terminate thread gracefully (i.e.
 * pthread_join)
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_fini(void){
    plugin_context_t* context = get_plugin_context();
    consumer_producer_signal_finished(context->queue);
    pthread_join(context->consumer_thread, NULL);
    consumer_producer_destroy(context->queue);
    free(context->queue);
    context->queue = NULL;
    return NULL;
}

/**
 * Place work (a string) into the plugin's queue
 * @param str The string to process (plugin takes ownership if it allocates
 * new memory)
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_place_work(const char* str) {
    plugin_context_t* context = get_plugin_context();
    char* item_copy = strdup(str);
    if (item_copy == NULL) return "Memory allocation failed in plugin_place_work";
    const char* err = consumer_producer_put(context->queue, item_copy);
    if (err != NULL) {
        free(item_copy);
        return err;
    }
    return NULL;
}

/**
 * Attach this plugin to the next plugin in the chain
 * @param next_place_work Function pointer to the next plugin's place_work
 * function
 */
__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*)){
    plugin_context_t* context = get_plugin_context(); 
    if (!context) return;
    context->next_place_work = next_place_work;
}

/**
 * Wait until the plugin has finished processing all work and is ready to
 * shutdown
 * This is a blocking function used for graceful shutdown coordination
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_wait_finished(void){
    plugin_context_t* context = get_plugin_context();
    if (!context) return "Plugin context is NULL";
    int result = consumer_producer_wait_finished(context->queue);
    if (result != 0) {
        return "plugin_wait_finished: wait failed";
    }
    return NULL;
}

/**
 * Return the plugin's display name.
 * Used for identification and debugging.
 * @return Pointer to static string representing plugin name
 */
__attribute__((visibility("default")))
const char* plugin_get_name(void){
    return "FLIPPER";
}

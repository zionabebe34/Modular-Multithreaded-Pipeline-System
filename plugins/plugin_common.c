#define _POSIX_C_SOURCE 200809L
#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

plugin_context_t* get_plugin_context(void) {
    static plugin_context_t context;
    return &context;
}

void* plugin_consumer_thread(void* arg) {
    plugin_context_t* context = (plugin_context_t*)arg;
    if (!context || !context->queue) {
        return NULL;
    }
    while (1){
        char* item = consumer_producer_get(context->queue);
        if (item == NULL) {
            break;
        }

        // Check for <END> signal - if found, pass it through and break
        if ((strcmp(item, "<END>") == 0) || (strcmp(item, "END") == 0)) {
            if (context->next_place_work) {
                (void)context->next_place_work(item);
            }
            free(item);
            // Signal that this plugin is finished
            consumer_producer_signal_finished(context->queue);
            break;
        }

        const char* out = NULL;
        if (context->process_function) {
            out = context->process_function(item);
        }
        const char* pass = (out != NULL) ? out : item;

        if (context->next_place_work) {
            (void)context->next_place_work(pass);
            if (out != NULL && out != item) {
                free(item);
            }
        } else {
            if (out != NULL && out != item) {
                free((void*)out);
            }
            free(item);
        }
    }
    return NULL;
}

void log_error(plugin_context_t* context, const char* message) {
    if (!message) return;
    const char* name = context ? context->name : "plugin";
    fprintf(stderr, "[ERROR][%s] - %s\n", name, message);
    
}

void log_info(plugin_context_t* context, const char* message) {
    if (!message) return;
    const char* name = context ? context->name : "plugin";
    printf("[INFO][%s] - %s\n", name, message);
    
}

const char* common_plugin_init(const char* (*process_function)(const char*),const char* name, int queue_size) {
    plugin_context_t* context = get_plugin_context();
    if (process_function == NULL) {
        return "Process function cannot be NULL";
    }
    if (queue_size <= 0) {
        return "Queue size must be greater than 0";
    }
    context->name = name ? name : "plugin";
    context->process_function = process_function;
    context->next_place_work = NULL; 

    if (context->queue == NULL) {
        context->queue = malloc(sizeof *context->queue);
        if (!context->queue) {
            return "malloc failed for queue";
        }
    }
    const char* err = consumer_producer_init(context->queue, queue_size);
    if (err != NULL) {
        return err;
    }

    int rc = pthread_create(&context->consumer_thread, NULL, plugin_consumer_thread, context);
    if (rc != 0) {
        log_error(context, "Failed to create consumer thread");
        consumer_producer_destroy(context->queue);
        return "Failed to create consumer thread";
    }    
    return NULL;
}

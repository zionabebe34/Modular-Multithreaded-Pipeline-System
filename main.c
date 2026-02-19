#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>


typedef const char* (*plugin_init_t)(int);
typedef const char* (*plugin_fini_t)(void);
typedef const char* (*plugin_place_work_t)(const char*);
typedef void        (*plugin_attach_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_t)(void);

typedef struct {
    void* handle;
    char* name;
    plugin_init_t init;
    plugin_fini_t fini;
    plugin_place_work_t place_work;
    plugin_attach_t attach;
    plugin_wait_finished_t wait_finished;
} plugin_handle_t;

static void print_usage(void) {
    fprintf(stdout,
        "Usage: ./main <queue_size> plugin1 [plugin2 ...]\n"
        "  queue_size: Maximum number of items in each plugin's queue\n"
        "  plugin1 [plugin2 ...]: Plugins to load (in order) from: logger,typewriter,uppercaser,rotator,flipper,expander\n"
    );
}

/* Sink for the last plugin: accept work and return NULL so the pipeline can drain */
static const char* sink_place_work(const char* s) {
    (void)s; /* intentionally ignore */
    return NULL;
}


int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing arguments.\n");
        print_usage();
        return 1;
    }
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "Error: queue_size must be a positive integer.\n");
        print_usage();
        return 1;
    }
    int args_num = argc - 2;
    plugin_handle_t* plugins = calloc(args_num, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < args_num; i++) {
        const char* plugin_name = argv[i + 2];
        char so_name[256];
        snprintf(so_name, sizeof(so_name), "./output/%s.so", plugin_name);
        plugins[i].handle = dlopen(so_name, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "Error loading plugin %s\n", plugin_name);
            print_usage();

            for ( int k = 0; k < i; k++ ) {
                if ( plugins[k].handle ) {
                    dlclose( plugins[k].handle );
                }
                free( plugins[k].name );
            }
            free(plugins);
            return 1;
        }
        plugins[i].name = strdup(plugin_name);
        if (!plugins[i].name) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            for (int k = 0; k <= i; k++) {
                if (plugins[k].handle) {
                    dlclose(plugins[k].handle);
                }
                if (plugins[k].name) {
                    free(plugins[k].name);
                }
            }
            free(plugins);
            return 1;
        }
        plugins[i].init = (plugin_init_t)dlsym(plugins[i].handle, "plugin_init");
        plugins[i].fini = (plugin_fini_t)dlsym(plugins[i].handle, "plugin_fini");
        plugins[i].place_work = (plugin_place_work_t)dlsym(plugins[i].handle, "plugin_place_work");
        plugins[i].attach = (plugin_attach_t)dlsym(plugins[i].handle, "plugin_attach");
        plugins[i].wait_finished = (plugin_wait_finished_t)dlsym(plugins[i].handle, "plugin_wait_finished");

        

        if (!plugins[i].init || !plugins[i].fini || !plugins[i].place_work ||
            !plugins[i].attach || !plugins[i].wait_finished) {
            fprintf(stderr, "dlsym() failed for plugin %s: %s\n", so_name, dlerror());
            for ( int k = 0; k <= i; k++ ) {
                if ( plugins[k].handle ) {
                    dlclose( plugins[k].handle );
                }
                if ( plugins[k].name ) {
                    free( plugins[k].name );
                }
            }
            free(plugins);
            return 1;
        }
    } 

    
    for (int i = 0; i < args_num; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "Plugin %s init() failed: %s\n", plugins[i].name, err);
            for (int k = 0; k < i; k++) {
                if (plugins[k].fini) plugins[k].fini();
            }
            
            for (int k = 0; k <= i; k++) {
                if (plugins[k].handle) dlclose(plugins[k].handle);
                if (plugins[k].name)   free(plugins[k].name);
            }
            free(plugins);
            return 1;
        }
    }

    // Chain the plugins together
    for (int i = 0; i + 1 < args_num; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }
    plugins[args_num - 1].attach(sink_place_work);
     char line[1025];
    int sent_end = 0;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        // Trim \n or \r\n
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        // If sentinel, forward it into the pipeline and stop reading
        if (strcmp(line, "<END>") == 0 || strcmp(line, "END") == 0) {
            const char* err = plugins[0].place_work("<END>");
            if (err) {
                fprintf(stderr, "Error sending <END> to first plugin: %s\n", err);
            }
            sent_end = 1;
            break;
        }

        // Normal line: duplicate because plugins take ownership
        char* copy = strdup(line);
        if (!copy) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            break;
        }
        const char* err = plugins[0].place_work(copy);
        if (err) {
            free(copy);
            fprintf(stderr, "Error placing work in plugin %s: %s\n", plugins[0].name, err);
            break;
        }
    }

    // If EOF happened without <END>, inject it now so workers can exit cleanly
    if (!sent_end) {
        const char* err = plugins[0].place_work("<END>");
        if (err) {
            fprintf(stderr, "Error sending <END> to first plugin: %s\n", err);
        }
    }
    

    // Wait for all plugins to finish processing
    for (int i = 0; i < args_num; i++) {
        const char* err = plugins[i].wait_finished();
        if (err) {
            fprintf(stderr, "Plugin %s wait_finished() failed: %s\n", plugins[i].name, err);
        }
    }

    // Shutdown all plugins
    for (int i = args_num - 1; i >= 0; --i) {
        if ( plugins[i].fini ) {
            const char* err = plugins[i].fini();
            if (err) {
                fprintf(stderr, "Plugin %s fini() failed: %s\n", plugins[i].name, err);
            }
        }
        if ( plugins[i].handle ) {
            dlclose( plugins[i].handle );
        }
        if ( plugins[i].name ) {
            free( plugins[i].name );
        }
    }
    free(plugins);
    printf("Pipeline shutdown complete\n");
    return 0;
}

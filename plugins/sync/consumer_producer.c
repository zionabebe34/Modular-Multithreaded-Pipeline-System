#include "consumer_producer.h"
#include "monitor.h"
#include <stdlib.h>


const char* consumer_producer_init(consumer_producer_t* queue, int capacity) {
    if (capacity <= 0) return "Capacity must be > 0";

    queue->items = malloc(capacity * sizeof(char*));
    if (!queue->items) return "Out of memory";
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->is_finished = 0;

    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        free(queue->items);
        return "Mutex init failed";
    }
    if (monitor_init(&queue->not_full) != 0 || monitor_init(&queue->not_empty) != 0 || monitor_init(&queue->finished) != 0) {
        pthread_mutex_destroy(&queue->lock);
        free(queue->items);
        return "Monitor init failed";
    }
    return NULL;
}


void consumer_producer_destroy(consumer_producer_t* queue){
    if (queue == NULL) return;

    pthread_mutex_destroy(&queue->lock);
    monitor_destroy(&queue->not_full);
    monitor_destroy(&queue->not_empty);
    monitor_destroy(&queue->finished);

    free(queue->items);
    queue->items = NULL;

    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->is_finished = 0;
}


const char* consumer_producer_put(consumer_producer_t* queue, const char* item) {
    pthread_mutex_lock(&queue->lock);
    if (queue->is_finished == 1){
        pthread_mutex_unlock(&queue->lock);
        return "Queue is closed";
    }
    while (queue->count == queue->capacity) {
        pthread_mutex_unlock(&queue->lock);
        monitor_wait(&queue->not_full);
        pthread_mutex_lock(&queue->lock);
    }
    queue->items[queue->tail] = (char*)item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    monitor_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
    return NULL;
    
}


char* consumer_producer_get(consumer_producer_t* queue)  {
    pthread_mutex_lock(&queue->lock);

    while (queue->count == 0) {
        if (queue->is_finished){
            pthread_mutex_unlock(&queue->lock);
            return NULL;
        }
        pthread_mutex_unlock(&queue->lock);
        monitor_wait(&queue->not_empty);
        pthread_mutex_lock(&queue->lock);
    }

    char* item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    monitor_signal(&queue->not_full);

    if (queue->count == 0 && queue->is_finished){
        monitor_signal(&queue->finished);
    }

    pthread_mutex_unlock(&queue->lock);
    return item;
}


void consumer_producer_signal_finished(consumer_producer_t* queue) {
    pthread_mutex_lock(&queue->lock);
    queue->is_finished = 1; 
    monitor_signal(&queue->not_empty);
    monitor_signal(&queue->not_full);
    if (queue->count == 0) {
        monitor_signal(&queue->finished);
    }
    pthread_mutex_unlock(&queue->lock);
}


int consumer_producer_wait_finished(consumer_producer_t* queue) {
    return monitor_wait(&queue->finished);
}

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/**
 * Funzione eseguita dal thread creato.
 * Attende `wait_to_obtain_ms`, acquisisce il mutex,
 * attende `wait_to_release_ms`, poi lo rilascia.
 */
void* threadfunc(void* thread_param)
{
    if (thread_param == NULL) {
        ERROR_LOG("thread_param è NULL");
        return NULL;
    }

    struct thread_data* data = (struct thread_data*) thread_param;

    // Attesa prima di acquisire il mutex
    usleep(data->wait_to_obtain_ms * 1000);

    if (pthread_mutex_lock(data->mutex) != 0) {
        ERROR_LOG("pthread_mutex_lock fallita");
        data->thread_complete_success = false;
        return data;
    }

    // Attesa prima di rilasciare il mutex
    usleep(data->wait_to_release_ms * 1000);

    if (pthread_mutex_unlock(data->mutex) != 0) {
        ERROR_LOG("pthread_mutex_unlock fallita");
        data->thread_complete_success = false;
        return data;
    }

    data->thread_complete_success = true;
    return data;
}

/**
 * Avvia un nuovo thread che esegue `threadfunc`.
 * Alloca dinamicamente `thread_data` da passare al thread.
 */
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms, int wait_to_release_ms)
{
    if (!thread || !mutex) {
        ERROR_LOG("Parametri nulli passati a start_thread_obtaining_mutex");
        return false;
    }

    struct thread_data *data = malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Impossibile allocare memoria per thread_data");
        return false;
    }

    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, data);
    if (rc != 0) {
        ERROR_LOG("pthread_create fallita: codice %d", rc);
        free(data);
        return false;
    }

    return true;
}


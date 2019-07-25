#include "os_semaphore.h"
#include "os_mutex.h"
#include <pthread.h>
struct os_semaphore {
    os_mutex_handle_t mutex;
    pthread_cond_t  cond;
};

os_semaphore_handle_t os_semaphore_create() {
    os_semaphore_handle_t semaphore;
    semaphore = (os_semaphore_handle_t)calloc(1,sizeof(struct os_semaphore));
    if(semaphore) {
        semaphore->mutex = os_mutex_create();
        pthread_cond_init(&semaphore->cond,0);
    }
    return semaphore;
}

int os_semaphore_take(os_semaphore_handle_t self) {
    os_mutex_handle_t mutex_t= self->mutex;
    pthread_cond_t* cond_t= &self->cond;
    os_mutex_lock(mutex_t);
    pthread_cond_wait(cond_t,mutex_t);
    os_mutex_unlock(mutex_t);
}

int os_semaphore_give(os_semaphore_handle_t self) {
    os_mutex_handle_t mutex_t= self->mutex;
    pthread_cond_t* cond_t= &self->cond;
    os_mutex_lock(mutex_t);
    pthread_cond_signal(cond_t);
    os_mutex_unlock(mutex_t);
}

void os_semaphore_destroy(os_semaphore_handle_t self) {
    os_mutex_handle_t mutex_t= self->mutex;
    pthread_cond_t* cond_t= &self->cond;
    os_mutex_destroy(mutex_t);  
    pthread_cond_destroy(cond_t);
    free(self); 
}



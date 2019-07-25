#include "os_mutex.h"
#include <pthread.h>
typedef pthread_mutex_t os_mutex;
os_mutex_handle_t os_mutex_create() {
    os_mutex_handle_t mutex = (os_mutex_handle_t)calloc(1,sizeof(os_mutex));
    if(mutex) {
        pthread_mutex_init(mutex,PTHREAD_PRIO_NONE);
        return mutex; 
    }
}

int os_mutex_lock(os_mutex_handle_t self) {
	pthread_mutex_t* mutex = (pthread_mutex_t*)self;
    pthread_mutex_lock(self);
}

int os_mutex_unlock(os_mutex_handle_t self) {
	pthread_mutex_t* mutex = (pthread_mutex_t*)self;
    pthread_mutex_unlock(self);
}

void os_mutex_destroy(os_mutex_handle_t self) {
    if(self) {
        pthread_mutex_destroy(self);
        free(self);
    }
}



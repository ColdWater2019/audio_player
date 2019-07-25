#include "os_thread.h"
#include <pthread.h>
typedef pthread_t os_thread;
os_thread_handle_t os_thread_create(os_thread_cfg_t *cfg) {
   os_thread_handle_t thread = (os_thread_handle_t)calloc(1,sizeof(pthread_t));
   if(thread) {
       pthread_create(thread,0,cfg->run,cfg->args);
   }
}

void os_thread_exit(os_thread_handle_t self) {
    pthread_t* thread = self;
    if(self) {
       pthread_join(*thread,NULL);
       free(self);
    }
}



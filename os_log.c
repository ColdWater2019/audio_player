#include "os_log.h"
#include "os_mutex.h"
#include <stdio.h>
#include <stdarg.h>
#define LOG_MAX_LENGTH 2048 
char string[LOG_MAX_LENGTH];
FILE* log_file;
os_mutex_handle_t log_lock;
int os_log_init(const char *file) {
    log_lock = os_mutex_create();
    log_file = fopen(file,"wb+");
    if(!log_file) {
        return -1;
    }
    return 0;
}
void os_log_deinit() {
    fclose(log_file);
    os_mutex_destroy(log_lock);
}
void os_print_module_log(void *self, const char *func, int line, os_log_level_t level, const char *message, ...) {
    os_mutex_lock(log_lock);
    os_log_block_t* log = (os_log_block_t*)self;
    if(level >= log->level && log_file) {
        va_list list;
        fprintf(log_file,"[%s],[%d] ",func,line); 
        va_start(list, message);
        vfprintf(log_file, message, list);
        fputc(10,log_file);
        va_end(list);
    }
    os_mutex_unlock(log_lock);
}


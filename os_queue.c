#include "os_queue.h"
#include "os_mutex.h"
#include "os_semaphore.h"
#include <stdio.h>
#include "os_log.h"

os_log_create_module(os_queue,OS_LOG_LEVEL_INFO);

typedef enum {
    //等待可读
    OS_QUEUE_STATE_WAIT_READABLE = 0x1,
    //等待可写
    OS_QUEUE_STATE_WAIT_WRITABLE = 0x2,
    //被动停止
    OS_QUEUE_STATE_STOPPED = 0x4,
    //写结束
    OS_QUEUE_STATE_FINISHED = 0x8
}os_queue_state_t;

struct os_queue {
    char **item;
    size_t item_size;
    size_t item_count;
    size_t fill;
    size_t read_pos;
    size_t write_pos;
    os_mutex_handle_t lock;
    os_semaphore_handle_t read_sem;
    os_semaphore_handle_t write_sem;
    os_queue_state_t state;
};    

os_queue_handle_t os_queue_create(size_t item_count, size_t item_size) {
    os_queue_handle_t queue = (os_queue_handle_t)calloc(1,sizeof(*queue));
    int i;
    queue->item_count = item_count;
    queue->item_size = item_size;
    queue->fill = 0;
    queue->read_pos = 0;
    queue->write_pos = 0;
    queue->state = 0;
    queue->item = calloc(item_count,sizeof(char*));
    for(i=0; i<item_count; i++) {
        queue->item[i] = calloc(1,item_size);
    }
    queue->lock = os_mutex_create();
    queue->read_sem = os_semaphore_create();
    queue->write_sem = os_semaphore_create();
    return queue;
}

int os_queue_send(os_queue_handle_t self, const void *data) {
    while(1) {
        os_mutex_lock(self->lock);
        OS_LOG_I(os_queue,"self->fill:%d,self->item_count:%d\n",self->fill,self->item_count);
        if(self->state & OS_QUEUE_STATE_STOPPED) {
            os_mutex_unlock(self->lock);
            return -1;
        } 
        if(self->fill != self->item_count) {
            break;
        }
        self->state |= OS_QUEUE_STATE_WAIT_WRITABLE;        
        os_mutex_unlock(self->lock);
        os_semaphore_take(self->write_sem);
    }
    OS_LOG_I(os_queue,"self->read_pos:%d\n",self->read_pos);
    memcpy(self->item[self->read_pos],data,self->item_size); 
    self->fill++;
    self->read_pos++;
    self->read_pos = self->read_pos % self->item_count;
    if(self->state & OS_QUEUE_STATE_WAIT_READABLE) { 
        self->state &= (~OS_QUEUE_STATE_WAIT_READABLE);
        os_semaphore_give(self->read_sem);
    }
    os_mutex_unlock(self->lock);
}

int os_queue_send_font(os_queue_handle_t self, const void *data) {

}

int os_queue_receive(os_queue_handle_t self, void *data) {
    while(1) {
        os_mutex_lock(self->lock);
        OS_LOG_I(os_queue,"self->fill:%d\n",self->fill);
        if(self->state & OS_QUEUE_STATE_STOPPED) {
            os_mutex_unlock(self->lock);
            return -1;
        }
        if(self->fill) {
            break;
        }
        if(self->state & OS_QUEUE_STATE_FINISHED) {
            self->state &= (~OS_QUEUE_STATE_FINISHED);
            os_mutex_unlock(self->lock);
            return 0;
        } 
        self->state |= OS_QUEUE_STATE_WAIT_READABLE;
        os_mutex_unlock(self->lock);
        os_semaphore_take(self->read_sem);
    }
    OS_LOG_I(os_queue,"self->write_pos:%d\n",self->write_pos);
    memcpy(data,self->item[self->write_pos++],self->item_size); 
    self->fill--;
    self->write_pos = self->write_pos % self->item_count;
    OS_LOG_I(os_queue,"self->write_pos:%d after \n",self->write_pos);
    if(self->state & OS_QUEUE_STATE_WAIT_WRITABLE) { 
        self->state &= (~OS_QUEUE_STATE_WAIT_WRITABLE);
        os_semaphore_give(self->write_sem);
    }
    os_mutex_unlock(self->lock);
}

int os_queue_receive_back(os_queue_handle_t self, void *data) {

}

int os_queue_stop(os_queue_handle_t self) {
    os_mutex_lock(self->lock);
    if(!(self->state | OS_QUEUE_STATE_STOPPED)) {
        if (self->state & OS_QUEUE_STATE_WAIT_READABLE) {
            self->state &= (~OS_QUEUE_STATE_WAIT_READABLE);
            os_semaphore_give(self->read_sem);
        } 
        if (self->state & OS_QUEUE_STATE_WAIT_WRITABLE) {
            self->state &= (~OS_QUEUE_STATE_WAIT_WRITABLE);
            os_semaphore_give(self->write_sem);
        }
        self->state |= OS_QUEUE_STATE_STOPPED;
    }
    os_mutex_unlock(self->lock); 
}

bool os_queue_is_full(os_queue_handle_t self) {
    bool is_full = false;
    os_mutex_lock(self->lock);
    if(self->fill >= self->item_count) {
        is_full = true;
    }
    os_mutex_unlock(self->lock);
    return is_full;
}

bool os_queue_is_empty(os_queue_handle_t self) {
    bool is_empty = false;
    os_mutex_lock(self->lock);
    if(self->fill == 0) {
        is_empty = true;
    }
    os_mutex_unlock(self->lock);
    return is_empty;
}

int os_queue_finish(os_queue_handle_t self) {
    os_mutex_lock(self->lock);
    if(self->state & OS_QUEUE_STATE_WAIT_READABLE) {
        self->state &= (~OS_QUEUE_STATE_WAIT_READABLE);
        self->state |= OS_QUEUE_STATE_FINISHED;
        os_semaphore_give(self->read_sem);
    } else {
        self->state |= OS_QUEUE_STATE_FINISHED;
    }
    os_mutex_unlock(self->lock);
}

int os_queue_peek(os_queue_handle_t self, void *data) {
    os_mutex_lock(self->lock);
    if(self->fill) {
        memcpy(data,self->item[self->write_pos],self->item_size); 
    }
    os_mutex_unlock(self->lock);
}

void os_queue_destroy(os_queue_handle_t self) {
    if(self) {
        os_mutex_destroy(self->lock);
        os_semaphore_destroy(self->read_sem);
        os_semaphore_destroy(self->write_sem);
        while(--self->item_count) {
            free(self->item[self->item_count]);
        }
        free(self);
    }    
}



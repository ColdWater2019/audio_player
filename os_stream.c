#include "os_stream.h"
#include "os_mutex.h"
#include "os_semaphore.h"
#include "os_log.h"

os_log_create_module(os_stream,OS_LOG_LEVEL_INFO);

typedef enum {
    //空闲
    OS_STREAM_STATE_IDLE = 0x0,
    //running状态下才可写入
    OS_STREAM_STATE_RUNNING = 0x1,
    //等待可读
    OS_STREAM_STATE_WAIT_READABLE = 0x2,
    //等待可写
    OS_STREAM_STATE_WAIT_WRITABLE = 0x4,
    //终止读写
    OS_STREAM_STATE_STOPPED = 0x8,
    //写完成
    OS_STREAM_STATE_FINISHED = 0x10,
}os_stream_state_t;

struct os_stream {
    char *buf;
    size_t buf_size;
    size_t fill;
    size_t read_pos;
    size_t write_pos;
    os_mutex_handle_t lock;
    os_semaphore_handle_t read_sem;
    os_semaphore_handle_t write_sem;
    os_stream_state_t state;
};

os_stream_handle_t  os_stream_create(size_t size) {
    os_stream_handle_t stream = (os_stream_handle_t) calloc(1,sizeof(struct os_stream));
    if(stream) {
        stream->buf_size = size;
        stream->buf = calloc(1,size);
        stream->lock = os_mutex_create();
        stream->read_sem = os_semaphore_create();
        stream->write_sem = os_semaphore_create();    
    }
    return stream;
}

int os_stream_start(os_stream_handle_t self) {
    os_mutex_lock(self->lock);
    self->fill = 0;
    self->read_pos = 0;
    self->write_pos = 0;
    self->state = OS_STREAM_STATE_RUNNING; 
    os_mutex_unlock(self->lock); 
}

int os_stream_read(os_stream_handle_t self, char *data, size_t data_len) {
    size_t buf_size;
    size_t fill;
    size_t read_pos = 0;
    size_t read_size = 0;
    size_t remaining_size = data_len;
    size_t total_read_size = 0;
    char* read_buf = data;
    OS_LOG_D(os_stream,"%s data_len:%d self->state:%d\n",__func__,data_len,self->state);
    if(data_len) {
        while(1) {
            os_mutex_lock(self->lock);
            if(self->state & OS_STREAM_STATE_STOPPED) {
                total_read_size = -1;
                os_mutex_unlock(self->lock);
                break;
            }

            buf_size = self->buf_size;
            fill = self->fill;
            read_pos = self->read_pos;
            read_size = buf_size - read_pos;
            if(read_size > buf_size) {
                read_size = buf_size;   
            } 
            if(read_size >= remaining_size) {
                read_size = remaining_size;
            }
            if(read_size <= fill) {
                memcpy(read_buf,self->buf + read_pos ,read_size);
                self->fill -= read_size;
                self->read_pos = (self->read_pos + read_size) % buf_size;
                if(self->state & OS_STREAM_STATE_WAIT_WRITABLE) {
                    self->state &= (~OS_STREAM_STATE_WAIT_WRITABLE);
                    os_semaphore_give(self->write_sem);
                }
                remaining_size -= read_size;
                total_read_size += read_size;
                read_buf += read_size;
                os_mutex_unlock(self->lock);
                if(!remaining_size) {
                    break;
                }
            } else if (self->state & OS_STREAM_STATE_FINISHED) {
                if(!fill) {
                    self->state &= (~OS_STREAM_STATE_FINISHED);
                    os_mutex_unlock(self->lock);
                    break; 
                }
                read_size = fill;
                memcpy(read_buf,self->buf + read_pos, read_size);
                self->fill -= read_size;
                self->read_pos = (self->read_pos + read_size) % buf_size;
                remaining_size -= read_size;
                total_read_size += read_size;
                read_buf += read_size;
                os_mutex_unlock(self->lock);//add by cherry
                break;
            } else {
                OS_LOG_D(os_stream,"state OS_STREAM_STATE_WAIT_READABLE\n");
                self->state |= OS_STREAM_STATE_WAIT_READABLE;
                os_mutex_unlock(self->lock);
                os_semaphore_take(self->read_sem);
                os_mutex_unlock(self->lock);
                OS_LOG_D(os_stream,"state OS_STREAM_STATE_WAIT_READABLE out\n");
            }
        } 
    }   
    return total_read_size;
}

int os_stream_read2(os_stream_handle_t self, char *data, size_t data_len) {

}

int os_stream_write(os_stream_handle_t self, const char *data, size_t data_len) {
    size_t buf_size;
    size_t fill;
    size_t write_pos;
    size_t write_size;
    size_t remaining_size = data_len;
    size_t total_write_size = 0;
    char* write_buf = data;
    OS_LOG_D(os_stream,"%s data_len:%d self->state:%d\n",__func__,data_len,self->state);
    if(data_len) {
        while(1) {
            os_mutex_lock(self->lock);
            if(!(self->state & OS_STREAM_STATE_RUNNING)) {
                os_mutex_unlock(self->lock);
                return 0;
            }
            if(self->state & OS_STREAM_STATE_STOPPED) {
                total_write_size = -1;
                os_mutex_unlock(self->lock);
                break;
            }

            buf_size = self->buf_size;
            fill = self->fill;
            write_pos = self->write_pos;
            write_size = buf_size - write_pos;
            if(write_size >= buf_size) {
                write_size = buf_size;   
            } 
            if(write_size >= remaining_size) {
                write_size = remaining_size;
            }
            OS_LOG_D(os_stream,"%s,write_size:%d buf_size:%d,fill:%d write_pos:%d\n",
                    __func__,write_size,buf_size,fill,self->write_pos);
            if(write_size <= buf_size - fill) {
                memcpy(self->buf + write_pos,write_buf,write_size);
                self->fill += write_size;
                self->write_pos = (self->write_pos + write_size) % buf_size;
                if(self->state & OS_STREAM_STATE_WAIT_READABLE) {
                    self->state &= (~OS_STREAM_STATE_WAIT_READABLE);
                    os_semaphore_give(self->read_sem);
                }
                remaining_size -= write_size;
                total_write_size += write_size;
                write_buf += write_size;
                os_mutex_unlock(self->lock);
                if(!remaining_size) {
                    break;
                }
            } else {
                OS_LOG_D(os_stream,"%s,write_size:%d wait write\n",__func__,write_size);
                self->state |= OS_STREAM_STATE_WAIT_WRITABLE;
                os_mutex_unlock(self->lock);
                os_semaphore_take(self->write_sem);
            }
        } 
    }   
    return total_write_size;

}

int os_stream_write2(os_stream_handle_t self, const char *data, size_t data_len) {

}

int os_stream_finish(os_stream_handle_t self) {
    os_mutex_lock(self->lock);
    if(self->state & OS_STREAM_STATE_WAIT_READABLE) {
        self->state &= (~OS_STREAM_STATE_WAIT_READABLE);
        self->state |= (OS_STREAM_STATE_FINISHED);
        os_semaphore_give(self->read_sem);
    } else {
        self->state |= (OS_STREAM_STATE_FINISHED);
    }
    os_mutex_unlock(self->lock);
}

int os_stream_stop(os_stream_handle_t self) {
    os_mutex_lock(self->lock);
    if(!(self->state & OS_STREAM_STATE_STOPPED)) {
        if(self->state & OS_STREAM_STATE_WAIT_WRITABLE) {
            self->state &= (~OS_STREAM_STATE_WAIT_WRITABLE);
            self->state |= OS_STREAM_STATE_STOPPED;
            os_semaphore_give(self->write_sem);
        } else {
            self->state |= OS_STREAM_STATE_STOPPED;
        }
        if (self->state & OS_STREAM_STATE_WAIT_READABLE) {
            self->state &= (~OS_STREAM_STATE_WAIT_READABLE);
            os_semaphore_give(self->read_sem); 
        }

    }
    os_mutex_unlock(self->lock);
}

int os_stream_stop2(os_stream_handle_t self) {

}

int os_stream_reset(os_stream_handle_t self) {
    os_mutex_lock(self->lock);
    self->fill = 0;
    self->read_pos = 0;
    self->write_pos = 0;
    self->state = OS_STREAM_STATE_IDLE;
    os_mutex_unlock(self->lock);
}

void os_stream_destroy(os_stream_handle_t self) {
    if(self) {
        os_mutex_destroy(self->lock);
        pthread_cond_destroy(self->read_sem);
        pthread_cond_destroy(self->write_sem);
        free(self->buf);
        free(self);
    }
}



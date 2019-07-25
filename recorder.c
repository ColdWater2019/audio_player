#include "recorder.h"
#include "record_pcm.h"
#include "capture_impl.h"
#include "record_encoder.h"
#include "capture_device.h"
#include "file_writer.h"
#include <fcntl.h>
#include <stdio.h>
#include "os_stream.h"
#include "os_queue.h"
#include "os_mutex.h"
#include "os_thread.h"
#include "os_semaphore.h"
#include "duisdk.h"
#include "os_log.h"

os_log_create_module(recorder,OS_LOG_LEVEL_DEBUG);

int g_encoder_num;
record_encoder_t* g_default_encoder;
struct recorder {
    //预处理消息队列
    os_queue_handle_t write_queue;
    //解码消息队列
    os_queue_handle_t encode_queue;
    //播放消息队列
    os_queue_handle_t record_queue;

    os_stream_handle_t encode_stream;
    
    os_stream_handle_t record_stream;

    //状态锁
    os_mutex_handle_t state_lock;
    //暂停信号量
    os_semaphore_handle_t pause_sem;

    //预处理线程
    os_thread_handle_t write_task;
    //解码线程
    os_thread_handle_t encode_task;
    //播放线程
    os_thread_handle_t record_task;

    record_writer_t writer;

    const char *target;
    //事件组
    //os_event_group_handle_t event;

    //播放器状态
    recorder_state_t state;

    //播放器标签，用于区分多个实例
    const char *tag;

    //播放器内部操作回调
    recorder_listen_cb listen;
    void *userdata;

    const char *device_name;
    capture_device_t device;

    //播放pcm数据时需要设置
    int samplerate;
    int bits;
    int channels;    
};

int recorder_init() {
    g_encoder_num = 1;
    g_default_encoder = (record_encoder_t*)calloc(g_encoder_num,sizeof(*g_default_encoder));
    record_encoder_t pcm_encoder = DEFAULT_PCM_ENCODER;
//    record_encoder_t wav_encoder = DEFAULT_WAV_encoder;
//    record_encoder_t mp3_encoder = DEFAULT_MP3_encoder;
//    record_encoder_t aac_encoder = DEFAULT_AAC_encoder;
    g_default_encoder[0] = pcm_encoder;
 //   g_default_encoder[1] = wav_encoder;
 //   g_default_encoder[2] = mp3_encoder;
    

}

int recorder_register_encoder(const char *type, record_encoder_t *encoder) {

}

void* write_run(void* data) {
    recorder_handle_t recorder = (recorder_handle_t) data;
    duisdk_msg_t msg;
    duisdk_msg_t encode_msg;
    record_writer_t writer;
    record_writer_cfg_t processor_cfg;
    int res;
    char *read_buf;
    size_t read_size = 0;
    size_t frame_size = 0;
    OS_LOG_D(recorder,"writer in\n");
    while(1){
        OS_LOG_D(recorder,"writer get mssage\n");
        if(os_queue_receive(recorder->write_queue,&msg) == -1) {
            OS_LOG_D(recorder,"write_run exit");
            return 0;
        }
        OS_LOG_D(recorder,"writer queue get msg\n");
        writer = msg.recorder.writer;
        if(!writer.type) {
            writer.type = "unknown";
        }
        
        processor_cfg.target = msg.recorder.target; 
        processor_cfg.tag = recorder->tag;
        OS_LOG_D(recorder,"writer init begin writer.type:%s\n",writer.type);
        res = writer.init(&writer,&processor_cfg); 
        OS_LOG_D(recorder,"writer init after\n");
        frame_size = processor_cfg.frame_size;
        if(msg.recorder.need_free) {
            free(msg.recorder.target);
            if(!res) {
                os_mutex_lock(recorder->state_lock);
                recorder->state_lock = recorder_STATE_IDLE;
                if (recorder->listen)
                    recorder->listen(recorder, RECORD_INFO_WRITER, recorder->userdata);
                //j_os_event_group_set_bits(v1[10], v6, 2LL);
                os_mutex_unlock(recorder->state_lock); 
            }
        } else {
            if(res) {
                OS_LOG_D(recorder,"writer init go to write out\n");
                goto __write_OUT;
            }
            read_buf = malloc(frame_size);
            if(!read_buf) {
                OS_LOG_D(recorder,"can't malloc buffer");
                return -1;
            }
            do
            {
                read_size = os_stream_read(recorder->encode_stream, read_buf, frame_size);            
                OS_LOG_D(recorder,"writer read data read_size:%d\n",read_size);
                if(read_size == 0) {
                    break;
                } 
                if(read_size != frame_size) {
                    break;
                }
                OS_LOG_D(recorder,"writer read_size:%d\n",read_size);
            } while (writer.write(&writer, read_buf, read_size) != -1);
            OS_LOG_D(recorder,"write_stream was stopped");
            free(read_buf);
            writer.destroy(&writer); 
        }
    }
__write_OUT:
    OS_LOG_D(recorder,"writer out\n");
}


int encoder_input(void *userdata, char *data, size_t data_len) {
    recorder_handle_t recorder = (recorder_handle_t) userdata; 
    return os_stream_read(recorder->record_stream,data,data_len);
}
int encoder_output(void *userdata, const char *data, size_t data_len) {
    recorder_handle_t recorder = (recorder_handle_t) userdata; 
    return os_stream_write(recorder->encode_stream,data,data_len);
}
int encoder_post(void *userdata, int samplerate, int bits, int channels) {
    // only decoder need.
}

void* encoder_run(void* data) {
    recorder_handle_t recorder = (recorder_handle_t) data;
    record_encoder_cfg_t* encoder_cfg = (record_encoder_cfg_t*)calloc(1,sizeof(*encoder_cfg)); 
    char* audio_type;
    record_encoder_t encoder;
    bool is_found_encoder = false;
    int i = 0;
    bool has_post;
    record_encoder_error_t encode_res;
    duisdk_msg_t encode_msg;
    duisdk_msg_t msg;
    encoder_cfg->input = encoder_input;
    encoder_cfg->output = encoder_output;
    encoder_cfg->post = encoder_post;
    encoder_cfg->userdata = recorder;
    while(1) {
        if (os_queue_receive(recorder->encode_queue, &encode_msg) == -1 ) {
            OS_LOG_D(recorder,"encode_run can't get msg");
            return -1;
        }
        audio_type = encode_msg.recorder.type; 
        OS_LOG_D(recorder,"encode_run,get audio_type:%s\n",audio_type);
        for(i = 0;i < g_encoder_num;i++) {
            if(!strcmp(audio_type,g_default_encoder[i].type)) {
                encoder = g_default_encoder[i];
                is_found_encoder = true;
                break; 
            }
        }
        if(!is_found_encoder) {
            OS_LOG_D(recorder,"encode_run, cant found encoder\n");
            os_stream_stop(recorder->record_stream);
            os_mutex_lock(recorder->state_lock);
            if (recorder->listen)
                recorder->listen(recorder, RECORD_INFO_ENCODE, recorder->userdata);
            recorder->state = recorder_STATE_IDLE;
            os_mutex_unlock(recorder->state_lock);
        } else {
            OS_LOG_D(recorder,"encode_run init begin\n");
            if(encoder.init(&encoder,encoder_cfg)) {
                OS_LOG_D(recorder,"encode_run, encoder init fail\n");
                os_stream_stop(recorder->record_stream);
                os_mutex_lock(recorder->state_lock);
                recorder->state = recorder_STATE_IDLE;
                if (recorder->listen)
                    recorder->listen(recorder, RECORD_INFO_ENCODE, recorder->userdata);
                os_mutex_unlock(recorder->state_lock);
            }
            OS_LOG_D(recorder,"encode_run init success\n");
            os_stream_start(recorder->encode_stream);
            msg.type = CMD_RECORDER_START;
            msg.recorder.mode = RECORD_MODE_PROMPT;
            msg.recorder.target = encode_msg.recorder.target;
            msg.recorder.type = encode_msg.recorder.type;
            msg.recorder.writer = encode_msg.recorder.writer; 
            msg.recorder.need_free = false;
            msg.recorder.end_session = false;
            os_queue_send(recorder->write_queue,&msg); 

            encode_res = encoder.process(&encoder);
            OS_LOG_D(recorder,"encode_run process success\n");
            OS_LOG_D(recorder,"encode_res:%d\n",encode_res);

            switch(encode_res) {
                case RECORD_ENCODER_INPUT_ERROR:
                    OS_LOG_D(recorder,"record_encoder_INPUT_ERROR\n");
                    break;
                case RECORD_ENCODER_OUTPUT_ERROR:
                    OS_LOG_D(recorder,"record_encoder_OUTPUT_ERROR\n");
                    os_stream_stop(recorder->record_stream);
                    if(!encoder.get_post_state(&encoder)) {
                        os_mutex_lock(recorder->state_lock);
                        recorder->state = recorder_STATE_IDLE;
                        if ( recorder->listen) {
                            recorder->listen(recorder, RECORD_INFO_ENCODE, recorder->userdata);
                        }
                        os_mutex_unlock(recorder->state_lock);
                        goto _DESTROY_encoder;
                    }
                    goto _DESTROY_encoder;
                case RECORD_ENCODER_ENCODE_ERROR:
                    OS_LOG_D(recorder,"record_encoder_encode_ERROR\n");
                    os_stream_stop(recorder->record_stream);
                    break;
                default:
                    OS_LOG_D(recorder,"os_stream_finish\n");
                    os_stream_finish(recorder->encode_stream);
                    goto _DESTROY_encoder;
                    break;
            }
            if(!encoder.get_post_state(&encoder)) {
                os_mutex_lock(recorder->state_lock);
                recorder->state = recorder_STATE_IDLE;
                if ( recorder->listen) {
                    recorder->listen(recorder, RECORD_INFO_ENCODE, recorder->userdata);
                }
                os_mutex_unlock(recorder->state_lock);
            }
_DESTROY_encoder:
            OS_LOG_D(recorder,"encoder process return value:%d\n",encode_res);
            encoder.destroy(&encoder);
        }
    } 
}

void* capture_run(void* data) {
    recorder_handle_t recorder = (recorder_handle_t) data;
    capture_device_t device = recorder->device; 
    capture_device_cfg_t device_cfg;
    duisdk_msg_t msg,encode_msg;
    char* read_buf;
    size_t read_size,write_size;
    size_t frame_size;
    OS_LOG_D(recorder,"capture_run start\n");   
    while(1) {
        if(os_queue_receive(recorder->record_queue,&msg) == -1) {
            OS_LOG_D(recorder,"capture_run receive data failed\n");
            return -1; 
        }
        OS_LOG_D(recorder,"capture_run receive msg\n");   
       
        if(encode_msg.type == CMD_RECORDER_START)  {
            device_cfg.samplerate = recorder->samplerate;
            device_cfg.bits = recorder->bits;
            device_cfg.channels = recorder->channels;
            device_cfg.device_name = recorder->device_name;
            device_cfg.frame_size = 0;
            os_stream_start(recorder->record_stream);
            encode_msg.type = msg.type;
            encode_msg.recorder.mode = msg.recorder.mode;
            encode_msg.recorder.target = msg.recorder.target;
            encode_msg.recorder.type = msg.recorder.type;
            encode_msg.recorder.writer = msg.recorder.writer;
            encode_msg.recorder.need_free = msg.recorder.need_free;
            encode_msg.recorder.end_session = msg.recorder.end_session;
            encode_msg.recorder.priv_data = "pcm"; 
            os_queue_send(recorder->encode_queue,&encode_msg);

            if(device.open(&device,&device_cfg)) {
                os_stream_stop(recorder->record_stream);
                OS_LOG_D(recorder,"capture_run failed\n");
                break;
            } else {
                frame_size = device_cfg.frame_size;
                device.start(&device);
                read_buf = malloc(frame_size);
                if(!read_buf) {
                    OS_LOG_D(recorder,"capture_run, create read buf failed\n");
                    break;
                }
                memset(read_buf,0,frame_size);
                while(1) {
                    OS_LOG_D(recorder,"os_stream_read frame_size:%d\n",frame_size);
                    read_size = device.read(&device,read_buf,frame_size);
                    if(read_size == -1) {
                        os_stream_stop(recorder->record_stream);
                        break;
                    }
                    if(read_size == 0) {
                        OS_LOG_D(recorder,"do not read data\n");
                        os_stream_finish(recorder->record_stream);
                        break;
                    } 
                    write_size = os_stream_write(recorder->record_stream,read_buf,read_size);
                    memset(read_buf,0,frame_size);

                    if(write_size < frame_size) {
                        OS_LOG_D(recorder,"read read_size < frame_size, go to stop\n");
                        break;
                    }

                    if(write_size == -1) {
                        os_stream_stop(recorder->record_stream);
                        break;
                    }
                }
                free(read_buf);
                device.stop(&device);
                device.close(&device);
                os_mutex_lock(recorder->state_lock);
                recorder->state = recorder_STATE_IDLE;
                if(recorder->listen) {
                    recorder->listen(recorder,RECORD_INFO_STOP,recorder->userdata);
                }
                os_mutex_unlock(recorder->state_lock);
                OS_LOG_D(recorder,"record_run close");
            }
        } else if (encode_msg.type == CMD_RECORDER_STOP) {
                OS_LOG_D(recorder,"record_run stop");
                break; 
        }
    } 
}

recorder_handle_t recorder_create(recorder_cfg_t *cfg) {
    recorder_handle_t recorder = (recorder_handle_t) calloc(1,sizeof(*recorder));
    OS_LOG_D(recorder,"recorder_create in");   
    if(recorder) {
        recorder->write_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        recorder->encode_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        recorder->record_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        recorder->record_stream = os_stream_create(cfg->record_buf_size);
        recorder->encode_stream = os_stream_create(cfg->encode_buf_size);
        recorder->state_lock = os_mutex_create();    
        recorder->pause_sem = os_semaphore_create;
        recorder->tag = cfg->tag;
        recorder->listen = cfg->listen;
        recorder->userdata = cfg->userdata;
        recorder->device_name = cfg->device_name;
        recorder->device = cfg->device;
        recorder->state = recorder_STATE_IDLE;

        os_thread_cfg_t c = {
            .run = write_run,
            .args = recorder     
        };
        recorder->write_task = os_thread_create(&c); 
        c.run = encoder_run;
        c.args = recorder; 
        recorder->encode_task = os_thread_create(&c); 
        c.run = capture_run;
        c.args = recorder;
        recorder->record_task = os_thread_create(&c); 
    }
    OS_LOG_D(recorder,"recorder_create out");   
    return recorder;
}

int recorder_record(recorder_handle_t self, record_cfg_t *cfg) {
    recorder_handle_t recorder = self;
    recorder_state_t state;
    duisdk_msg_t msg;
    os_mutex_lock(recorder->state_lock);    
    recorder->state = recorder_STATE_RUNNING;
    os_mutex_unlock(recorder->state_lock);    
    recorder->samplerate = cfg->samplerate;
    recorder->bits = cfg->bits;
    recorder->channels = cfg->channels;
    msg.type = CMD_RECORDER_START;
    msg.recorder.mode = RECORD_MODE_PROMPT;
    msg.recorder.target = cfg->target;
    msg.recorder.type = cfg->type;
    msg.recorder.writer = cfg->writer;
    msg.recorder.need_free = cfg->need_free;
    msg.recorder.end_session = false;
    os_queue_send(recorder->record_queue,&msg);
}

recorder_state_t recorder_get_state(recorder_handle_t self) {
    recorder_state_t state;
    os_mutex_lock(self->state_lock);
    state = self->state;
    os_mutex_unlock(self->state_lock);
    return state;
}
int recorder_stop(recorder_handle_t self) {
    recorder_state_t state;
    recorder_handle_t recorder = self;
    duisdk_msg_t msg;

    int result;
    os_mutex_lock(self->state_lock);
    if(self->state) {
        os_stream_stop(self->record_stream);
        if(self->state == RECORD_INFO_PAUSED) {
            os_semaphore_give(self->pause_sem);
        }
        os_mutex_unlock(self->state_lock);
        result = 0;
        msg.type = CMD_RECORDER_STOP;
        os_queue_send(recorder->record_queue,&msg);
        OS_LOG_D(recorder,"recorder_stop stop recorder,pause/running state\n");
    } else {
        os_mutex_unlock(self->state_lock);
        OS_LOG_D(recorder,"recorder_stop stop recorder,idle state\n");
        result = 0;
    }
    return result;
}
int recorder_pause(recorder_handle_t self) {

}
int recorder_resume(recorder_handle_t self) {

}
int recorder_wait_idle(recorder_handle_t self) {

}
void recorder_destroy(recorder_handle_t self) {

}
void recorder_deinit() {

}


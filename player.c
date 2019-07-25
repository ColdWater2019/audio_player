#include "player.h"
#include "play_pcm.h"
#include "play_wav.h"
#include "play_mp3.h"
#include "playback_impl.h"
#include "play_decoder.h"
#include "playback_device.h"
#include "file_preprocessor.h"
#include <fcntl.h>
#include <stdio.h>
#include "os_stream.h"
#include "os_queue.h"
#include "os_mutex.h"
#include "os_thread.h"
#include "os_semaphore.h"
#include "duisdk.h"
#include "os_log.h"

os_log_create_module(player,OS_LOG_LEVEL_INFO);

int g_decoder_num;
play_decoder_t* g_default_decoder;
struct player {
    //预处理消息队列
    os_queue_handle_t preprocess_queue;
    //解码消息队列
    os_queue_handle_t decode_queue;
    //播放消息队列
    os_queue_handle_t play_queue;

    //预处理流缓冲
    os_stream_handle_t preprocess_stream;
    //解码流缓冲
    os_stream_handle_t decode_stream;

    //状态锁
    os_mutex_handle_t state_lock;
    //暂停信号量
    os_semaphore_handle_t pause_sem;

    //预处理线程
    os_thread_handle_t preprocess_task;
    //解码线程
    os_thread_handle_t decode_task;
    //播放线程
    os_thread_handle_t play_task;

    //事件组
    //os_event_group_handle_t event;

    //播放器状态
    player_state_t state;

    //播放器标签，用于区分多个实例
    const char *tag;

    //播放器内部操作回调
    player_listen_cb listen;
    void *userdata;

    const char *name;
    playback_device_t device;

    //播放pcm数据时需要设置
    int samplerate;
    int bits;
    int channels;    
};

int player_init() {
    g_decoder_num = 3;
    g_default_decoder = (play_decoder_t*)calloc(g_decoder_num,sizeof(*g_default_decoder));
    play_decoder_t pcm_decoder = DEFAULT_PCM_DECODER;
    play_decoder_t wav_decoder = DEFAULT_WAV_DECODER;
    play_decoder_t mp3_decoder = DEFAULT_MP3_DECODER;
//    play_decoder_t aac_decoder = DEFAULT_AAC_DECODER;
    g_default_decoder[0] = pcm_decoder;
    g_default_decoder[1] = wav_decoder;
    g_default_decoder[2] = mp3_decoder;
    

}

int player_register_decoder(const char *type, play_decoder_t *decoder) {

}

void* preprocess_run(void* data) {
    player_handle_t player = (player_handle_t) data;
    duisdk_msg_t msg;
    duisdk_msg_t decode_msg;
    play_preprocessor_t preprocessor;
    play_preprocessor_cfg_t processor_cfg;
    int res;
    char *read_buf;
    size_t read_size = 0;
    size_t frame_size = 0;
    OS_LOG_D(player,"preprocessor in\n");
    while(1){
        OS_LOG_D(player,"preprocessor get mssage\n");
        if(os_queue_receive(player->preprocess_queue,&msg) == -1) {
            OS_LOG_D(player,"preprocess_run exit");
            return 0;
        }
        OS_LOG_D(player,"preprocessor queue get msg\n");
        preprocessor = msg.player.preprocessor;
        if(!preprocessor.type) {
            preprocessor.type = "unknown";
        }
        OS_LOG_D(player,"preprocessor.type:%s\n",preprocessor.type); 
        processor_cfg.target = msg.player.target; 
        processor_cfg.tag = player->tag;
        res = preprocessor.init(&preprocessor,&processor_cfg); 
        frame_size = processor_cfg.frame_size;
        if(msg.player.need_free) {
            free(msg.player.target);
            if(!res) {
                os_mutex_lock(player->state_lock);
                player->state_lock = PLAYER_STATE_IDLE;
                if (player->listen)
                    player->listen(player, PLAY_INFO_PREPROCESS, player->userdata);
                //j_os_event_group_set_bits(v1[10], v6, 2LL);
                os_mutex_unlock(player->state_lock); 
            }
        } else {
            if(res) {
                goto __PREPROCESS_OUT;
            }
            read_buf = malloc(frame_size);
            if(!read_buf) {
                OS_LOG_D(player,"can't malloc buffer");
                return -1;
            }
            decode_msg.type = msg.type;
            decode_msg.player.mode = msg.player.mode;
            decode_msg.player.target = msg.player.target;
            decode_msg.player.preprocessor = msg.player.preprocessor;
            decode_msg.player.need_free = msg.player.need_free;
            decode_msg.player.end_session = msg.player.end_session;
            decode_msg.player.priv_data = processor_cfg.type; 
            os_stream_start(player->preprocess_stream);
            OS_LOG_D(player,"preprocessor queue send msg to decode\n");
            os_queue_send(player->decode_queue,&decode_msg);
            do
            {
                OS_LOG_D(player,"preprocessor read data frame_size:%d\n",frame_size);
                read_size = preprocessor.read(&preprocessor, read_buf, frame_size);
                OS_LOG_D(player,"preprocessor read_size:%d\n",read_size);
                if ( read_size <= 0 )
                {
                    os_stream_finish(player->preprocess_stream);
                    OS_LOG_D(player,"preprocess read return");
                    break;
                }
            } while (os_stream_write(player->preprocess_stream, read_buf, read_size) != -1 );
            OS_LOG_D(player,"preprocess_stream was stopped");
            free(read_buf);
            preprocessor.destroy(&preprocessor); 
        }
    }
__PREPROCESS_OUT:
    OS_LOG_D(player,"preprocessor out\n");
}


int decoder_input(void *userdata, char *data, size_t data_len) {
    player_handle_t player = (player_handle_t) userdata; 
    return os_stream_read(player->preprocess_stream,data,data_len);
}
int decoder_output(void *userdata, const char *data, size_t data_len) {
    player_handle_t player = (player_handle_t) userdata; 
    return os_stream_write(player->decode_stream,data,data_len);
}
int decoder_post(void *userdata, int samplerate, int bits, int channels) {
    player_handle_t player = (player_handle_t)userdata; 
    duisdk_msg_t msg;
    player->samplerate = samplerate;
    player->bits = bits;
    player->channels = channels;

    msg.type = CMD_PLAYER_PLAY;
    msg.player.mode = PLAY_MODE_PROMPT;
    msg.player.target = NULL;
    msg.player.need_free = false;
    msg.player.end_session = false;
 
    os_queue_send(player->play_queue,&msg); 
}

void* decoder_run(void* data) {
    player_handle_t player = (player_handle_t) data;
    play_decoder_cfg_t* decoder_cfg = (play_decoder_cfg_t*)calloc(1,sizeof(*decoder_cfg)); 
    char* audio_type;
    play_decoder_t decoder;
    bool is_found_decoder = false;
    int i = 0;
    bool has_post;
    play_decoder_error_t decode_res;
    duisdk_msg_t decode_msg;
    decoder_cfg->input = decoder_input;
    decoder_cfg->output = decoder_output;
    decoder_cfg->post = decoder_post;
    decoder_cfg->userdata = player;
    while(1) {
        if (os_queue_receive(player->decode_queue, &decode_msg) == -1 ) {
            OS_LOG_D(player,"decode_run can't get msg");
            return -1;
        }
        audio_type = decode_msg.player.priv_data; 
        OS_LOG_D(player,"decode_run,get audio_type:%s\n",audio_type);
        for(i = 0;i < g_decoder_num;i++) {
            if(!strcmp(audio_type,g_default_decoder[i].type)) {
                decoder = g_default_decoder[i];
                is_found_decoder = true;
                break; 
            }
        }
        if(!is_found_decoder) {
            OS_LOG_D(player,"decode_run, cant found decoder\n");
            os_stream_stop(player->preprocess_stream);
            os_mutex_lock(player->state_lock);
            if (player->listen)
                player->listen(player, PLAY_INFO_DECODE, player->userdata);
            player->state = PLAYER_STATE_IDLE;
            os_mutex_unlock(player->state_lock);
        } else {
            OS_LOG_D(player,"decode_run init begin\n");
            if(decoder.init(&decoder,decoder_cfg)) {
                OS_LOG_D(player,"decode_run, decoder init fail\n");
                os_stream_stop(player->preprocess_stream);
                os_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_IDLE;
                if (player->listen)
                    player->listen(player, PLAY_INFO_DECODE, player->userdata);
                os_mutex_unlock(player->state_lock);
            }
            OS_LOG_D(player,"decode_run init success\n");
            os_stream_start(player->decode_stream);
            OS_LOG_D(player,"decode_run start stream success\n");
            decode_res = decoder.process(&decoder);
            OS_LOG_D(player,"decode_run process success\n");
            OS_LOG_D(player,"decode_res:%d\n",decode_res);
            switch(decode_res) {
                case PLAY_DECODER_INPUT_ERROR:
                    OS_LOG_D(player,"PLAY_DECODER_INPUT_ERROR\n");
                    break;
                case PLAY_DECODER_OUTPUT_ERROR:
                    OS_LOG_D(player,"PLAY_DECODER_OUTPUT_ERROR\n");
                    os_stream_stop(player->preprocess_stream);
                    if(!decoder.get_post_state(&decoder)) {
                        os_mutex_lock(player->state_lock);
                        player->state = PLAYER_STATE_IDLE;
                        if ( player->listen) {
                            player->listen(player, PLAY_INFO_DECODE, player->userdata);
                        }
                        os_mutex_unlock(player->state_lock);
                        goto _DESTROY_DECODER;
                    }
                    goto _DESTROY_DECODER;
                case PLAY_DECODER_DECODE_ERROR:
                    OS_LOG_D(player,"PLAY_DECODER_DECODE_ERROR\n");
                    os_stream_stop(player->preprocess_stream);
                    break;
                default:
                    OS_LOG_D(player,"os_stream_finish\n");
                    os_stream_finish(player->decode_stream);
                    goto _DESTROY_DECODER;
                    break;
            }
            if(!decoder.get_post_state(&decoder)) {
                os_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_IDLE;
                if ( player->listen) {
                    player->listen(player, PLAY_INFO_DECODE, player->userdata);
                }
                os_mutex_unlock(player->state_lock);
            }
_DESTROY_DECODER:
            OS_LOG_D(player,"decoder process return value:%d\n",decode_res);
            decoder.destroy(&decoder);
        }
    } 
}

void* playback_run(void* data) {
    player_handle_t player = (player_handle_t) data;
    playback_device_t device = player->device; 
    playback_device_cfg_t device_cfg;
    duisdk_msg_t msg;
    char* read_buf;
    size_t read_size;
    size_t frame_size;
    OS_LOG_D(player,"playback_run start\n");   
    while(1) {
        if(os_queue_receive(player->play_queue,&msg) == -1) {
            OS_LOG_D(player,"playback_run receive data failed\n");
            return -1; 
        }
        OS_LOG_D(player,"playback_run receive msg\n");   
        device_cfg.samplerate = player->samplerate;
        device_cfg.bits = player->bits;
        device_cfg.channels = player->channels;
        device_cfg.name = player->name;
        device_cfg.frame_size = 0;
        if(device.open(&device,&device_cfg)) {
            os_stream_stop(player->decode_stream);
            OS_LOG_D(player,"play_run\n");
        } else {
            frame_size = device_cfg.frame_size;
            device.start(&device);
            read_buf = malloc(frame_size);
            if(!read_buf) {
                OS_LOG_D(player,"playback_run, create read buf failed\n");
            }
            memset(read_buf,0,frame_size);
            while(1) {
               OS_LOG_D(player,"os_stream_read frame_size:%d\n",frame_size);
               read_size = os_stream_read(player->decode_stream,read_buf,frame_size); 
               if(read_size == -1) {
                   break;
               }
               if(read_size == 0) {
                   OS_LOG_D(player,"do not read data\n");
                   goto __FREE_BUF; 
               }
               os_mutex_lock(player->state_lock);
               if(player->state == PLAYER_STATE_PAUSED) {
                   device.stop(&device);
                   device.close(&device);
                   if(player->listen) {
                       player->listen(player,PLAY_INFO_PAUSED,player->userdata);
                   }
                   os_mutex_unlock(player->state_lock);
                   OS_LOG_D(player,"playback_run pause\n");
                   os_semaphore_take(player->pause_sem);
                   os_mutex_lock(player->state_lock);
                   player->state = PLAYER_STATE_RUNNING;
                   if(player->listen) {
                       player->listen(player,PLAY_INFO_RESUMED,player->userdata);
                   }
                   device.open(&device,&device_cfg);
                   os_mutex_unlock(player->state_lock);
                   device.start(&device);
                   OS_LOG_D(player,"playback_run continue play\n");
               } else {
                   os_mutex_unlock(player->state_lock);
               }
               OS_LOG_D(player,"playback run ,write data:%d\n",read_size);
               device.write(&device,read_buf,read_size);
               if(read_size < frame_size) {
__FREE_BUF:
                   free(read_buf);
                   device.stop(&device);
                   device.close(&device);
                   os_mutex_lock(player->state_lock);
                   player->state = PLAYER_STATE_IDLE;
                   if(player->listen) {
                       player->listen(player,PLAY_INFO_PAUSED,player->userdata);
                   }
                   goto __PLAY_STOP;
               }
               memset(read_buf,0,frame_size);
            }
            free(read_buf);
            device.stop(&device);
            device.close(&device);
            os_mutex_lock(player->state_lock);
            player->state = PLAYER_STATE_IDLE;
            if(player->listen) {
                player->listen(player,PLAY_INFO_STOP,player->userdata);
            }

__PLAY_STOP: 
            os_mutex_unlock(player->state_lock);
            OS_LOG_D(player,"play_run close");
        }
    } 
}

player_handle_t player_create(player_cfg_t *cfg) {
    player_handle_t player = (player_handle_t) calloc(1,sizeof(*player));
    OS_LOG_D(player,"player_create in");   
    if(player) {
        player->preprocess_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        player->decode_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        player->play_queue = os_queue_create(1,sizeof(duisdk_msg_t));
        player->preprocess_stream = os_stream_create(cfg->preprocess_buf_size);
        player->decode_stream = os_stream_create(cfg->decode_buf_size);
        player->state_lock = os_mutex_create();    
        player->pause_sem = os_semaphore_create;
        player->tag = cfg->tag;
        player->listen = cfg->listen;
        player->userdata = cfg->userdata;
        player->name = cfg->name;
        player->device = cfg->device;
        player->state = PLAYER_STATE_IDLE;

        os_thread_cfg_t c = {
            .run = preprocess_run,
            .args = player     
        };
        player->preprocess_task = os_thread_create(&c); 
        c.run = decoder_run;
        c.args = player; 
        player->decode_task = os_thread_create(&c); 
        c.run = playback_run;
        c.args = player;
        player->play_task = os_thread_create(&c); 
    }
    OS_LOG_D(player,"player_create out");   
    return player;
}

int player_play(player_handle_t self, play_cfg_t *cfg) {
    player_handle_t player = self;
    player_state_t state;
    duisdk_msg_t msg;
    os_mutex_lock(player->state_lock);    
    player->state = PLAYER_STATE_RUNNING;
    os_mutex_unlock(player->state_lock);    
    player->samplerate = cfg->samplerate;
    player->bits = cfg->bits;
    player->channels = cfg->channels;
    msg.type = CMD_PLAYER_PLAY;
    msg.player.mode = PLAY_MODE_PROMPT;
    msg.player.target = cfg->target;
    msg.player.preprocessor = cfg->preprocessor;
    msg.player.need_free = cfg->need_free;
    msg.player.end_session = false;
    os_queue_send(player->preprocess_queue,&msg);
}

player_state_t player_get_state(player_handle_t self) {
    player_state_t state;
    os_mutex_lock(self->state_lock);
    state = self->state;
    os_mutex_unlock(self->state_lock);
    return state;
}
int player_stop(player_handle_t self) {
    player_state_t state;
    int result;
    os_mutex_lock(self->state_lock);
    if(self->state) {
        os_stream_stop(self->decode_stream);
        if(self->state == PLAY_INFO_PAUSED) {
            os_semaphore_give(self->pause_sem);
        }
        os_mutex_unlock(self->state_lock);
        result = 0;
        OS_LOG_D(player,"player_stop stop player,pause/running state\n");
    } else {
        os_mutex_unlock(self->state_lock);
        OS_LOG_D(player,"player_stop stop player,idle state\n");
        result = 0;
    }
    return result;
}
int player_pause(player_handle_t self) {

}
int player_resume(player_handle_t self) {

}
int player_wait_idle(player_handle_t self) {

}
void player_destroy(player_handle_t self) {

}
void player_deinit() {

}


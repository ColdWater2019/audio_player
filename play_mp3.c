#include "play_mp3.h"
#include "mp3dec.h"
#include "mp3common.h"
#include <stdio.h>
#include "os_log.h"

os_log_create_module(play_mp3,OS_LOG_LEVEL_INFO);


typedef struct _MP3PLAYERINFO {
     HMP3Decoder mpi_mp3dec;
     MP3FrameInfo mpi_frameinfo;
} MP3PLAYERINFO;

static MP3PLAYERINFO mpi;

static short pi_pcmbuf[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];

struct play_mp3 {
    bool has_post;
    char read_buf[1024];//MAINBUF_SIZE * 2 / 2];
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};
typedef struct play_mp3* play_mp3_handle_t;

FILE* file;
int play_mp3_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg) {
    printf("play_mp3_init_impl in\n");
    play_mp3_handle_t mp3 = (play_mp3_handle_t) calloc(1, sizeof(*mp3));
    if(!mp3)    return -1;
    mp3->has_post = false;
    mp3->input = cfg->input;
    mp3->output = cfg->output;
    mp3->post = cfg->post;
    mp3->userdata = cfg->userdata;
    self->userdata = (void*) mp3;
     
    mpi.mpi_mp3dec = MP3InitDecoder(); 
    file = fopen("test.pcm","a+");
    printf("play_mp3_init_impl out\n");
    return 0;
}

play_decoder_error_t play_mp3_process_impl(struct play_decoder *self) {
    play_mp3_handle_t mp3 = (play_mp3_handle_t) self->userdata;
    size_t read_bytes,skip,filled = 0,back_bytes = 0,decode_bytes,input_decode_bytes;
    size_t id3_length = 0,n,bytes_left = 0,read_offset = 0;
    int sample_rate,channels,bits,ret = 0;
    char* input_data;
    bool is_first_frame = true,can_decode = false;
    char decode_buf[4 * MAINBUF_SIZE],*decode_ptr;
    printf("play_mp3_process\n");
    
    read_bytes = mp3->input(mp3->userdata,mp3->read_buf,10);
    if(read_bytes != 10) {
        printf("read buffer failed\n");
        return -1;
    }
    n = 10;
    if(!memcmp(mp3->read_buf,"ID3",3)) {
        printf("find id3\n");
        id3_length = (mp3->read_buf[6] & 0x7f) << 21 |
            (mp3->read_buf[7] & 0x7f) << 14 |
            (mp3->read_buf[8] & 0x7f) << 7 |
            (mp3->read_buf[9] & 0x7f) ;
        printf("%s,id3_length:%d\n",__func__,id3_length);
        if(id3_length > 0) {
             while(1) {
                 read_bytes = mp3->input(mp3->userdata,mp3->read_buf,sizeof(mp3->read_buf) - n);
                 printf("id3_length:%d,read_bytes:%d\n",id3_length,read_bytes);
                 if(read_bytes < 0) {
                     printf("%s,read mp3 input failed\n",__func__);
                     return -1;
                 }
                 if(read_bytes + n >= id3_length) {
                     n = read_bytes + n - id3_length;
                     memmove(mp3->read_buf,&mp3->read_buf[id3_length],n);
                     printf("%s,skip id3_length,:%d\n",__func__,id3_length);
                     break;
                 } else {
                     id3_length -= read_bytes;
                     id3_length -= n;
                     n = 0;
                 } 
             }
        }
    } 
    
    while(1) {
        printf("play_mp3_process:1\n");
        if(bytes_left > 0) {
            memmove(mp3->read_buf,decode_ptr,bytes_left);
        } else if(bytes_left < 0) {
            printf("read one buffer");
        } 
        printf("play_mp3_process:sizeof(mp3->read_buf):%d\n",sizeof(mp3->read_buf));
        read_bytes = mp3->input(mp3->userdata,mp3->read_buf + n + bytes_left,sizeof(mp3->read_buf) - n - bytes_left);
        n = 0;
        printf("play_mp3_process:%d\n",read_bytes);
        if(read_bytes == 0 && bytes_left == 0) {
            return 0;
        } else if (read_bytes == -1) {
            return -1;
        } else {
            decode_ptr = mp3->read_buf;
            read_offset = 0;
            bytes_left += read_bytes;
            can_decode = true;         
        }
       
        if(can_decode) {
            if ((skip = MP3FindSyncWord(decode_ptr, bytes_left)) < 0) {
                decode_bytes = -1;
                printf("mp3 decode don't find sync word\n");
                break; 
            }
            printf("mp3 decode find sync word,skip:%d\n",skip);
            bytes_left  -= skip;
            decode_ptr  += skip;
            printf("MP3Decode decode_bytes:%d\n",decode_bytes);
            if (ret = MP3Decode(mpi.mpi_mp3dec, &decode_ptr, &bytes_left, pi_pcmbuf, 0)) {
                printf("mp3 decode failed,decode_bytes:%d ret:%d\n",decode_bytes,ret);
                if(ret == ERR_MP3_INDATA_UNDERFLOW) {
                    bytes_left = 0;
                    continue;
                } else if (ret == ERR_MP3_MAINDATA_UNDERFLOW) {
                    continue;
                } else if(ret <= ERR_MP3_INVALID_FRAMEHEADER) {
                    bytes_left = bytes_left - 1;
                    decode_ptr++;
                    printf("ERR_MP3_INVALID_FRAMEHEADER\n");
                    continue;
                    //while(1);
                    //goto __CONTINUE_DECODE;
                } else if (ret == ERR_MP3_INVALID_HUFFCODES) {
                    bytes_left = 0;
                    break;
                }
                decode_bytes = -1;
                //while(1);
                break; 
            }
            printf("MP3Decode success\n");
            MP3GetLastFrameInfo(mpi.mpi_mp3dec, &mpi.mpi_frameinfo);
            sample_rate = mpi.mpi_frameinfo.samprate;
            channels = mpi.mpi_frameinfo.nChans;
            bits = mpi.mpi_frameinfo.bitsPerSample;
            printf("sample_rate:%d,channels:%d,bits:%d\n",sample_rate,channels,bits);
            if (channels == 1) {
                sample_rate = sample_rate / 2;
            } else if (channels == 2) {
                sample_rate = sample_rate;
            } else {
                printf("mp3 decode channels don't support\n");
                decode_bytes = -1;
                break;
            }

            if (sample_rate < 8000 || sample_rate> 48000) {
                printf("mp3 decode sample rate don't support\n");
                filled = -1;
                break;
            }
            if (bits != 16) {
                printf("mp3 decode sample bit don't support\n");
                filled = -1;
                break;
            }
            if (is_first_frame) {
                mp3->post(mp3->userdata,sample_rate,bits,channels);
                mp3->has_post = true;
                is_first_frame = false;
                printf("post sample_rate:%d,channels:%d,bits:%d\n",sample_rate,channels,bits);
            }
            if(mpi.mpi_frameinfo.outputSamps > 0) {
                printf("decode_bytes:%d mpi.mpi_frameinfo.outputSamps:%d\n",
                        decode_bytes,mpi.mpi_frameinfo.outputSamps);
                size_t output_bytes = mpi.mpi_frameinfo.outputSamps * 2 ;
                int write_bytes = mp3->output(mp3->userdata,(char*)pi_pcmbuf,output_bytes);
                if(write_bytes == -1) {
                    printf("mp3_decode pcm write failed\n");
                    filled = -1;
                    break;
                }
                //fwrite((char*)pi_pcmbuf,1,output_bytes,file);
                printf("mp3 output write_bytes:%d\n",write_bytes);
            } 
        }
    }
    return 0;

}

bool play_mp3_get_post_state_impl(struct play_decoder *self) {
    play_mp3_handle_t mp3 = (play_mp3_handle_t) self->userdata;
    return mp3->has_post;
}

void play_mp3_destroy_impl(struct play_decoder *self) {
    play_mp3_handle_t mp3 = (play_mp3_handle_t) self->userdata;
    if (mp3) {
        free(mp3);
    }

    if(mpi.mpi_mp3dec) {
        MP3FreeDecoder(mpi.mpi_mp3dec);
        mpi.mpi_mp3dec = NULL;
    }
    fclose(file);

}



#include <stdlib.h>
#include "play_pcm.h"
#include "os_log.h"

os_log_create_module(play_pcm,OS_LOG_LEVEL_INFO);

struct play_pcm {
    bool has_post;
    char read_buf[1024];
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};
typedef struct play_pcm* play_pcm_handle_t;

int play_pcm_init(struct play_decoder *self, play_decoder_cfg_t *cfg) {
    play_pcm_handle_t pcm = (play_pcm_handle_t) calloc(1, sizeof(*pcm));
    if(!pcm)    return -1;
    pcm->has_post = false;
    pcm->input = cfg->input;
    pcm->output = cfg->output;
    pcm->post = cfg->post;
    pcm->userdata = cfg->userdata;
    self->userdata = (void*) pcm;
    return 0;
}
play_decoder_error_t play_pcm_process(struct play_decoder *self) {
    play_pcm_handle_t pcm = (play_pcm_handle_t) self->userdata;
    OS_LOG_D(play_pcm,"play_pcm_process\n");
    pcm->post(pcm->userdata,48000,16,2);
    pcm->has_post = true;
    while(1) {
        int read_bytes = pcm->input(pcm->userdata,pcm->read_buf,sizeof(pcm->read_buf));
        OS_LOG_D(play_pcm,"play_pcm_process:%d\n",read_bytes);
        if(read_bytes == 0) {
            return 0;
        } else if (read_bytes == -1) {
            return -1;
        }
        int write_bytes = pcm->output(pcm->userdata,pcm->read_buf,read_bytes);
        if(write_bytes == -1) {
            return -1;
        }
    }
    return 0;
}
bool play_pcm_get_post_state(struct play_decoder *self) {
    play_pcm_handle_t pcm = (play_pcm_handle_t) self->userdata;
    return pcm->has_post;
}
void play_pcm_destroy(struct play_decoder *self) {
    play_pcm_handle_t pcm = (play_pcm_handle_t) self->userdata;
    if (pcm) {
        free(pcm);
    }
}

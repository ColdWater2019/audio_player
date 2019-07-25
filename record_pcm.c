#include <stdlib.h>
#include "record_pcm.h"
#include "os_log.h"

os_log_create_module(record_pcm,OS_LOG_LEVEL_DEBUG);

struct record_pcm {
    bool has_post;
    char read_buf[1024];
    encode_input_t input;
    encode_output_t output;
    encode_post_t post;
    void *userdata;
};
typedef struct record_pcm* record_pcm_handle_t;

int record_pcm_init(struct record_encoder *self, record_encoder_cfg_t *cfg) {
    record_pcm_handle_t pcm = (record_pcm_handle_t) calloc(1, sizeof(*pcm));
    if(!pcm)    return -1;
    pcm->has_post = false;
    pcm->input = cfg->input;
    pcm->output = cfg->output;
    pcm->post = cfg->post;
    pcm->userdata = cfg->userdata;
    self->userdata = (void*) pcm;
    return 0;
}
record_encoder_error_t record_pcm_process(struct record_encoder *self) {
    record_pcm_handle_t pcm = (record_pcm_handle_t) self->userdata;
    OS_LOG_D(record_pcm,"record_pcm_process\n");
    while(1) {
        int read_bytes = pcm->input(pcm->userdata,pcm->read_buf,sizeof(pcm->read_buf));
        OS_LOG_D(record_pcm,"record_pcm_process:%d\n",read_bytes);
        if(read_bytes == 0) {
            OS_LOG_D(record_pcm,"pcm->input finish \n");
            return 0;
        } else if (read_bytes == -1) {
            OS_LOG_D(record_pcm,"pcm->input failed \n");
            return -1;
        }
        int write_bytes = pcm->output(pcm->userdata,pcm->read_buf,read_bytes);
        if(write_bytes == -1) {
            OS_LOG_D(record_pcm,"pcm->output failed \n");
            return -1;
        }
    }
    return 0;
}
bool record_pcm_get_post_state(struct record_encoder *self) {
    record_pcm_handle_t pcm = (record_pcm_handle_t) self->userdata;
    return pcm->has_post;
}
void record_pcm_destroy(struct record_encoder *self) {
    record_pcm_handle_t pcm = (record_pcm_handle_t) self->userdata;
    if (pcm) {
        free(pcm);
    }
}

#ifndef PLAY_PCM_H
#define PLAY_PCM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "play_decoder.h"

int play_pcm_init(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_pcm_process(struct play_decoder *self);
bool play_pcm_get_post_state(struct play_decoder *self);
void play_pcm_destroy(struct play_decoder *self);

#define DEFAULT_PCM_DECODER { \
        .type = "pcm", \
        .init = play_pcm_init, \
        .process = play_pcm_process, \
        .get_post_state = play_pcm_get_post_state, \
        .destroy = play_pcm_destroy, \
    }

#ifdef __cplusplus
}
#endif
#endif

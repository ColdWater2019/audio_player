#ifndef PLAY_MP3_H
#define PLAY_MP3_H
#ifdef __cplusplus
extern "C" {
#endif

#include "play_decoder.h"

 int play_mp3_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
 play_decoder_error_t play_mp3_process_impl(struct play_decoder *self);
 bool play_mp3_get_post_state_impl(struct play_decoder *self);
 void play_mp3_destroy_impl(struct play_decoder *self);

#define DEFAULT_MP3_DECODER { \
        .type = "mp3", \
        .init = play_mp3_init_impl, \
        .process = play_mp3_process_impl, \
        .get_post_state = play_mp3_get_post_state_impl, \
        .destroy = play_mp3_destroy_impl, \
    }

#ifdef __cplusplus
}
#endif
#endif

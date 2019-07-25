#ifndef PLAY_DECODER_H
#define PLAY_DECODER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

typedef int (*decode_input_t)(void *userdata, char *data, size_t data_len);
typedef int (*decode_output_t)(void *userdata, const char *data, size_t data_len);
typedef int (*decode_post_t)(void *userdata, int samplerate, int bits, int channels);

typedef struct {
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
} play_decoder_cfg_t;

typedef enum {
    PLAY_DECODER_SUCCESS = 0,
    //输入非正常退出
    PLAY_DECODER_INPUT_ERROR,
    //输出非正常退出
    PLAY_DECODER_OUTPUT_ERROR,
    //解码出错
    PLAY_DECODER_DECODE_ERROR,
}play_decoder_error_t;

struct play_decoder {
    //pcm/wav/mp3/aac...
    const char *type;
    int (*init)(struct play_decoder *self, play_decoder_cfg_t *cfg);
    play_decoder_error_t (*process)(struct play_decoder *self);
    bool (*get_post_state)(struct play_decoder *self);
    void (*destroy)(struct play_decoder *self);
    void *userdata;
};

typedef struct play_decoder play_decoder_t;

#ifdef __cplusplus
}
#endif
#endif

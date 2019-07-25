#ifndef RECORD_ENCODER_H
#define RECORD_ENCODER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

typedef int (*encode_input_t)(void *userdata, char *data, size_t data_len);
typedef int (*encode_output_t)(void *userdata, const char *data, size_t data_len);
typedef int (*encode_post_t)(void *userdata, int samplerate, int bits, int channels);

typedef struct {
    encode_input_t input;
    encode_output_t output;
    encode_post_t post;
    void *userdata;
} record_encoder_cfg_t;

typedef enum {
    RECORD_ENCODER_SUCCESS = 0,
    //输入非正常退出
    RECORD_ENCODER_INPUT_ERROR,
    //输出非正常退出
    RECORD_ENCODER_OUTPUT_ERROR,
    //解码出错
    RECORD_ENCODER_ENCODE_ERROR,
}record_encoder_error_t;

struct record_encoder {
    //pcm/wav/mp3/aac...
    const char *type;
    int (*init)(struct record_encoder *self, record_encoder_cfg_t *cfg);
    record_encoder_error_t (*process)(struct record_encoder *self);
    bool (*get_post_state)(struct record_encoder *self);
    void (*destroy)(struct record_encoder *self);
    void *userdata;
};

typedef struct record_encoder record_encoder_t;

#ifdef __cplusplus
}
#endif
#endif

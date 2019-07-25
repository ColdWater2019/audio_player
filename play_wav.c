#include "play_wav.h"
#include "wav_header.h"
#include <stdlib.h>
#include <string.h>
struct play_wav {
    bool has_post;
    char read_buf[1024];
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};
typedef struct play_wav* play_wav_handle_t;
int play_wav_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg)
{
    play_wav_handle_t wav = (play_wav_handle_t)calloc(1, sizeof(*wav));
    if (!wav) return -1;
    wav->input = cfg->input;
    wav->output= cfg->output;
    wav->post = cfg->post;
    wav->userdata = cfg->userdata;
    self->userdata = (void *)wav;
    return 0;
}
play_decoder_error_t play_wav_process_impl(struct play_decoder *self) {
    play_wav_handle_t wav = (play_wav_handle_t)self->userdata;
    int read_bytes;
    int write_bytes;
    struct wav_header header;
    read_bytes = wav->input(wav->userdata, (char *)&header,
            sizeof(header));
    if (read_bytes != sizeof(header)) return PLAY_DECODER_DECODE_ERROR;
    if (memcmp(header.chunk_id, "RIFF", 4) != 0 ||
            memcmp(header.format, "WAVE", 4) != 0 ||
            memcmp(header.subchunk1_id, "fmt ", 4) != 0 ||
            memcmp(header.subchunk2_id, "data", 4) != 0) {
        return PLAY_DECODER_DECODE_ERROR;
    }
    printf("wav decoder sample rate: %d, channels: %d, bits: %d",
            header.samplerate, header.num_channels, header.bits_per_sample);
    wav->post(wav->userdata, header.samplerate, header.bits_per_sample,
            header.num_channels);
    wav->has_post = true;
    while (1) {
        read_bytes = wav->input(wav->userdata, wav->read_buf,
                sizeof(wav->read_buf));
        if (read_bytes == 0) {
            return PLAY_DECODER_SUCCESS;
        } else if (read_bytes == -1) {return PLAY_DECODER_INPUT_ERROR;
        }
        write_bytes = wav->output(wav->userdata, wav->read_buf,
                read_bytes);
        if (write_bytes == -1) return PLAY_DECODER_OUTPUT_ERROR;
    }
    return PLAY_DECODER_SUCCESS;
}
bool play_wav_get_post_state_impl(struct play_decoder *self) {
    play_wav_handle_t wav = (play_wav_handle_t)self->userdata;
    return wav->has_post;
}
void play_wav_destroy_impl(struct play_decoder *self) {
    if (!self) return;
    play_wav_handle_t wav = (play_wav_handle_t)self->userdata;
    free(wav);
}

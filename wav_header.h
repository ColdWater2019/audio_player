#ifndef WAV_HEADER_H
#define WAV_HEADER_H
#ifdef __cpluspplus
extern "C" {
#endif

#include <stdint.h>

//http://soundfile.sapp.org/doc/WaveFormat/

struct wav_header {
    char chunk_id[4];   //The "RIFF" chunk descriptor
    uint32_t chunk_size;
    char format[4];     //"WAVE"

    char subchunk1_id[4];//"fmt "
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t samplerate;
    uint32_t byterate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    char subchunk2_id[4];//"data"
    uint32_t subchunk2_size;
};

int wav_header_init(struct wav_header *head, int samplerate, int bits, int channels);
int wav_header_complete(struct wav_header *head, int samples);


#ifdef __cpluspplus
}
#endif
#endif
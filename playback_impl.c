#include "playback_impl.h"
#include "os_log.h"
#include <alsa/asoundlib.h>
os_log_create_module(playback_impl,OS_LOG_LEVEL_INFO);

static snd_pcm_t *playback_handle;
static snd_pcm_uframes_t period_size;

int playback_device_open_impl(struct playback_device *self, playback_device_cfg_t *cfg) {
    OS_LOG_D(playback_impl,"%s\n",__func__);
    cfg->frame_size = 1024;
    return 0;
    char *device = getenv("USER_PLAYBACK_DEVICE");
    if (!device) {
        device = cfg->name ? cfg->name : "default";
    }
    int err = snd_pcm_open(&playback_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        OS_LOG_D(playback_impl,"snd_pcm_open \"%s\" error: %s", device, snd_strerror(err));
        return -1;
    }
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_hw_params_alloca(&hw_params);
    err = snd_pcm_hw_params_any(playback_handle, hw_params);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Broken configuration for playback: no configurations available: %s", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Access type not available for playback: %s", snd_strerror(err));
        return -1;
    }
    snd_pcm_format_t format;
    if (cfg->bits == 16) {
        format =  SND_PCM_FORMAT_S16_LE;
    } else {
        format = SND_PCM_FORMAT_S32_LE;
    }
    err = snd_pcm_hw_params_set_format(playback_handle, hw_params, format);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Sample format not available for playback: %s", snd_strerror(err));
        return -1;
    } 
    err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, cfg->channels);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Channels count (%i) not available for playback: %s", cfg->channels, snd_strerror(err));
        return -1;
    }
    unsigned int samplerate = cfg->samplerate;
    err = snd_pcm_hw_params_set_rate(playback_handle, hw_params, samplerate, 0);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Rate %iHz not available for playback: %s", samplerate, snd_strerror(err));
        return -1;
    }
    unsigned int period_time = 20000;
    err = snd_pcm_hw_params_set_period_time_near(playback_handle, hw_params, &period_time, NULL);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Unable to set period time %i for playback: %s", period_time, snd_strerror(err));
        return -1;
    }
    unsigned int period_count = 20;
    err = snd_pcm_hw_params_set_periods_max(playback_handle, hw_params, &period_count, NULL);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Unable to set period max %i for playback: %s", period_count, snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params(playback_handle, hw_params);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Unable to set hw params for playback: %s", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, NULL);
    if (err < 0) {
        snd_pcm_close(playback_handle);
        OS_LOG_D(playback_impl,"Unable to get period size for playback: %s", snd_strerror(err));
        return -1;
    }
    cfg->frame_size = period_size * cfg->channels * cfg->bits / 8;
    return 0;
}

int playback_device_start_impl(struct playback_device *self) {
    return 0;
}

int playback_device_write_impl(struct playback_device *self, const char *data, size_t data_len) {
    OS_LOG_D(playback_impl,"%s data_len:%d\n",__func__,data_len);
    FILE* test = fopen("test_bak.pcm","a+");
    fwrite(data,1,data_len,test);
    fclose(test);
    return data_len;
	int write_frames = snd_pcm_writei(playback_handle, data, period_size);
    if (write_frames < 0) {
        OS_LOG_D(playback_impl,"write_frames: %d, %s", write_frames, snd_strerror(write_frames));
    }
    if (write_frames == -EPIPE) {
        snd_pcm_prepare(playback_handle);
    }
    return data_len;

}

int playback_device_stop_impl(struct playback_device *self) {
    OS_LOG_D(playback_impl,"%s\n",__func__);
    return 0;
	snd_pcm_drain(playback_handle);
	return 0;
}

int playback_device_abort_impl(struct playback_device *self) {
    OS_LOG_D(playback_impl,"%s\n",__func__);
    return 0;
	snd_pcm_drop(playback_handle);	
	return 0;
}

void playback_device_close_impl(struct playback_device *self) {
    OS_LOG_D(playback_impl,"%s\n",__func__);
    return 0;
 	snd_pcm_close(playback_handle);
}

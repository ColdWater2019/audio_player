#include "capture_impl.h"
#include "os_log.h"
#include <alsa/asoundlib.h>
#include <unistd.h>
os_log_create_module(capture_impl,OS_LOG_LEVEL_INFO);

static snd_pcm_t *capture_handle;
static snd_pcm_uframes_t period_size;

FILE* test;
int capture_device_open_impl(struct capture_device *self, capture_device_cfg_t *cfg) {
    OS_LOG_D(capture_impl,"%s\n",__func__);
    cfg->frame_size = 1024;
    test = fopen("test_input_source.pcm","rb"); 
    if(!test) { 
        printf("fopen input file failed\n");
        return -1;
    }
    return 0;
    char *device = getenv("USER_CAPTURE_DEVICE");
    if (!device) {
        device = cfg->device_name ? cfg->device_name : "default";
    }
    int err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        OS_LOG_D(capture_impl,"snd_pcm_open \"%s\" error: %s", device, snd_strerror(err));
        return -1;
    }
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_hw_params_alloca(&hw_params);
    err = snd_pcm_hw_params_any(capture_handle, hw_params);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Broken configuration for capture: no configurations available: %s", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Access type not available for capture: %s", snd_strerror(err));
        return -1;
    }
    snd_pcm_format_t format;
    if (cfg->bits == 16) {
        format =  SND_PCM_FORMAT_S16_LE;
    } else {
        format = SND_PCM_FORMAT_S32_LE;
    }
    err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Sample format not available for capture: %s", snd_strerror(err));
        return -1;
    } 
    err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, cfg->channels);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Channels count (%i) not available for capture: %s", cfg->channels, snd_strerror(err));
        return -1;
    }
    unsigned int samplerate = cfg->samplerate;
    err = snd_pcm_hw_params_set_rate(capture_handle, hw_params, samplerate, 0);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Rate %iHz not available for capture: %s", samplerate, snd_strerror(err));
        return -1;
    }
    unsigned int period_time = 20000;
    err = snd_pcm_hw_params_set_period_time_near(capture_handle, hw_params, &period_time, NULL);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Unable to set period time %i for capture: %s", period_time, snd_strerror(err));
        return -1;
    }
    unsigned int period_count = 20;
    err = snd_pcm_hw_params_set_periods_max(capture_handle, hw_params, &period_count, NULL);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Unable to set period max %i for capture: %s", period_count, snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params(capture_handle, hw_params);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Unable to set hw params for capture: %s", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, NULL);
    if (err < 0) {
        snd_pcm_close(capture_handle);
        OS_LOG_D(capture_impl,"Unable to get period size for capture: %s", snd_strerror(err));
        return -1;
    }
    cfg->frame_size = period_size * cfg->channels * cfg->bits / 8;
    return 0;
}

int capture_device_start_impl(struct capture_device *self) {
    return 0;
}

int capture_device_read_impl(struct capture_device *self, char *data, size_t data_len) {
    OS_LOG_D(capture_impl,"%s data_len:%d\n",__func__,data_len);
    usleep(23000);
    if(!test)
        return 0;
    return fread(data,1,data_len,test);
    return data_len;
	int write_frames = snd_pcm_writei(capture_handle, data, period_size);
    if (write_frames < 0) {
        OS_LOG_D(capture_impl,"write_frames: %d, %s", write_frames, snd_strerror(write_frames));
    }
    if (write_frames == -EPIPE) {
        snd_pcm_prepare(capture_handle);
    }
    return data_len;

}

int capture_device_stop_impl(struct capture_device *self) {
    OS_LOG_D(capture_impl,"%s\n",__func__);
    return 0;
	snd_pcm_drain(capture_handle);
	return 0;
}

int capture_device_abort_impl(struct capture_device *self) {
    OS_LOG_D(capture_impl,"%s\n",__func__);
    return 0;
	snd_pcm_drop(capture_handle);	
	return 0;
}

void capture_device_close_impl(struct capture_device *self) {
    OS_LOG_D(capture_impl,"%s\n",__func__);
    fclose(test);
    return 0;
 	snd_pcm_close(capture_handle);
}

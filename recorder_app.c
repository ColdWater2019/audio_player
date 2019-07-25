#include "recorder.h"
#include "capture_impl.h"
#include "file_writer.h"
#include "os_log.h"
#include <stdio.h>

os_log_create_module(recorder_app,OS_LOG_LEVEL_DEBUG);

static void recorder_listen(recorder_handle_t recorder, record_info_t info, void *userdata) {
    OS_LOG_D(recorder_app,"============================%p---%d\n",recorder,  info);
}

int main(int argc, char **argv) {
    os_log_init("test.log");
    recorder_init();

    //recorder_register_encoder("aac", DEFAULT_AAC_encodeR);

    recorder_cfg_t cfg_one = {
        .record_buf_size = 1024 * 20,
        .encode_buf_size = 1024 * 10,
        .tag = "one",
        .device_name = "default",
        .device = DEFAULT_CAPTURE_DEVICE,
        .listen = recorder_listen
    };

    recorder_cfg_t cfg_two = {
        .record_buf_size = 1024 * 20,
        .encode_buf_size = 1024 * 10,
        .tag = "two",
        .device_name = "default",
        .device = DEFAULT_CAPTURE_DEVICE,
        .listen = recorder_listen
    };
    OS_LOG_D(recorder_app,"recorder create in\n");
    recorder_handle_t recorder_one = recorder_create(&cfg_one);
    recorder_handle_t recorder_two = recorder_create(&cfg_two);

    record_cfg_t c = {
        .target = "test_recorder.pcm",
        .type = "pcm",
        .need_free = false,
        .writer = DEFAULT_FILE_WRITER,
        .samplerate = 48000,
        .bits = 16,
        .channels = 2
    };
    OS_LOG_D(recorder_app,"recorder create out\n");

    OS_LOG_D(recorder_app,"recorder record in\n");
    recorder_record(recorder_one, &c);
    recorder_stop(recorder_one);
    OS_LOG_D(recorder_app,"recorder record out\n");
    while(1);
    sleep(30);
    return -1; 
    OS_LOG_D(recorder_app,"pause in");
    recorder_pause(recorder_one);
    OS_LOG_D(recorder_app,"pause out");
    
    c.target = "test.mp3";
    recorder_record(recorder_two, &c);
    sleep(20);
    recorder_pause(recorder_two);
    recorder_resume(recorder_one);

    recorder_wait_idle(recorder_one);

    recorder_resume(recorder_two);
    recorder_wait_idle(recorder_two);

    recorder_destroy(recorder_one);
    recorder_destroy(recorder_two);
    recorder_deinit();
    return 0;
}

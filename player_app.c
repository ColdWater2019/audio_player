#include <stdio.h>
#include "os_log.h"
#include "player.h"
#include "playback_impl.h"
#include "file_preprocessor.h"
#include "http_preprocessor.h"

os_log_create_module(player_app,OS_LOG_LEVEL_DEBUG);

static void player_listen(player_handle_t player, play_info_t info, void *userdata) {
    OS_LOG_D(player_app,"%p---%d\n", player, info);
}

int main(int argc, char **argv) {
    os_log_init("test.log");
    player_init();

    player_cfg_t cfg_one = {
        .preprocess_buf_size = 1024 * 5,
        .decode_buf_size = 1024 * 5,
        .tag = "one",
        .name = "default",
        .device = DEFAULT_PLAYBACK_DEVICE,
        .listen = player_listen
    };

    player_cfg_t cfg_two = {
        .preprocess_buf_size = 1024 * 5,
        .decode_buf_size = 1024 * 5,
        .tag = "two",
        .name = "default",
        .device = DEFAULT_PLAYBACK_DEVICE,
        .listen = player_listen
    };

    OS_LOG_D(player_app,"player_create()\n");
    player_handle_t player_one = player_create(&cfg_one);
    player_handle_t player_two = player_create(&cfg_two);

    play_cfg_t c = {
        .target = "speech.mp3",
        //.target = "http://res.iot.baidu.com:80/api/v1/tts/qc0hlM_XVbGBWPuA2nbpbEqJ0ezHaXj1NF1NwW5dmPjEGvnGU-jSDq1bFfc4PEe8FWS_2Hy3kqtnhzQ3y6BzOYwXjssMKrznEIJJZClo0K4.mp3",
        //.target = "http://qnod.qingting.fm/mp3/5246399_128.mp3?from=dumi&logid=1544694727416-0a5497a4-74ef-4fc8-9e45-7907b9751916_3#1_0",
        //.target = "http://fdfs.xmcdn.com/group25/M09/A4/98/wKgJNlh927TxG-efAAd4SITX7J0271.mp3",
        //.target = "http://image.kaolafm.net/mz2/aac_32/20150614/68459905-b2d6-472a-8fe6-1ce841a23ff0.aac",
        //.target = "1.mp3",
        .need_free = false,
        //.preprocessor = DEFAULT_HTTP_PREPROCESSOR,
        .preprocessor = DEFAULT_FILE_PREPROCESSOR,
        .samplerate = 48000,
        .bits = 16,
        .channels = 2
    };

    OS_LOG_D(player_app,"player_play()\n");
    player_play(player_one, &c);
    
    OS_LOG_D(player_app,"pause in");
    player_pause(player_one);
    OS_LOG_D(player_app,"pause out");
    
    c.target = "test.mp3";
    player_play(player_two, &c);
    sleep(20);
    player_pause(player_two);
    player_resume(player_one);

    player_wait_idle(player_one);

    player_resume(player_two);
    player_wait_idle(player_two);

    player_destroy(player_one);
    player_destroy(player_two);
    player_deinit();
    return 0;
}

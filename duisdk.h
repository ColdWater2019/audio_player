#ifndef DUISDK_H
#define DUISDK_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include "playback_device.h"
#include "capture_device.h"
#include "play_preprocessor.h"
#include "record_writer.h"

#ifndef DUISDK_EXPORT
#ifdef __GNUC__
#define DUISDK_EXPORT  __attribute ((visibility("default")))
#else
#define DUISDK_EXPORT 
#endif
#endif

//状态机各个状态描述
typedef enum {
	//空闲状态
    STATE_IDLE = 0,
	//等待唤醒状态
    STATE_WAIT_FOR_WAKEUP,
	//唤醒提示状态
    STATE_WAKEUP_PROMPT,
	//等待开始说话状态
    STATE_WAIT_FOR_SPEECH_START,
    //开始说话超时提示状态
    STATE_SPEECH_START_TIMEOUT_PROMPT,
	//拾音状态
    STATE_SPEECHING,
	//等待语音结果状态
    STATE_WAIT_FOR_DDS,
} duisdk_state_t;

//duisdk定义的各类消息或者命令
//开发者可以监听的消息有
//  1.INFO_WAKEUP_MAJOR
//  2.INFO_WAKEUP_MINOR
//  3.INFO_WAKEUP_DOA
//  4.INFO_DDS_RESPONSE
//  5.CMD_SYSTEM_RESET
//  6.INFO_MEDIA_COMPLETE
//  7.CMD_RECORDER_STOP     //此消息在录音设备关闭后触发
//  8.INFO_STATE_INFO
typedef enum {
    //开启录音命令，只在IDLE状态下响应此命令
    CMD_RECORDER_START,
    //关闭录音命令，只在WAIT_FOR_WAKEUP响应此命令
    CMD_RECORDER_STOP,
    //将状态机设置到WAIT_FOR_WAKEUP状态
    CMD_SYSTEM_RESET,
    //外部触发即手动进入VAD检测过程
    //只在WAIT_FOR_WAKEUP状态下响应此命令，进入语音识别过程
    CMD_EXTERNAL_TRIGGER,
    //内部命令
    CMD_WAKEUP_START,
    //内部命令
    CMD_WAKEUP_STOP,
    //内部命令
    CMD_VAD_START,
    //内部命令
    CMD_VAD_STOP,
    //主唤醒
    INFO_WAKEUP_MAJOR,
    //快捷唤醒
    INFO_WAKEUP_MINOR,
    //唤醒角度信息（只针对麦克风阵列）
    INFO_WAKEUP_DOA,
    //检测到开始说话
    INFO_SPEECH_BEGIN,
    //检测开始说话超时（默认5秒）
    INFO_SPEECH_BEGIN_TIMEOUT,
    //多次检测到开始说话超时
    INFO_SPEECH_BEGIN_TIMEOUT_REPEAT,
    //检测到说话结束
    INFO_SPEECH_END,
    //检测说话结束超时（默认20秒）
    INFO_SPEECH_END_TIMEOUT,
    //继续对话（多轮对话）
    INFO_SPEECH_CONTINUE,
    //提示音播放结束
    INFO_PROMPT_COMPLETE,
    //云端结果
    INFO_DDS_RESPONSE,
    //播放请求
    CMD_PLAYER_PLAY,
    //音乐播放结束
    INFO_MEDIA_COMPLETE,
    //状态机状态
    INFO_STATE_INFO
} duisdk_msg_type_t;

//云端识别结果信息
typedef enum {
    //网络错误
    DDS_TYPE_ERROR,
    //语音识别结果
    DDS_TYPE_ASR,
    //语义结果
    DDS_TYPE_NLG,
} duisdk_dds_type_t;

typedef enum {
    //提示音
    PLAY_MODE_PROMPT = 0,
    //合成音 
    PLAY_MODE_TTS,
    //音乐
    PLAY_MODE_MEDIA,
    //提示音
    RECORD_MODE_PROMPT,
    RECORD_MODE_TTS,
    RECORD_MODE_MEDIA
} duisdk_play_mode_t;

typedef struct {
    duisdk_msg_type_t type;
    union {
        struct {
            //唤醒角度
            int doa;
            //唤醒词索引
            int index;
            //主唤醒词
            bool major;
            //记录上一次唤醒时间
            struct timeval last_major_time;
            struct timeval last_minor_time;
            //记录此次唤醒的时间
            struct timeval cur_major_time;
            struct timeval cur_minor_time;
        } wakeup;
        struct {
            //是否清除计数
            bool clear;
            //当前超时次数
            int timeout_count;
        } vad;
        struct {
            duisdk_dds_type_t type;
            int error_id;
            //识别结果或者语义结果
            char *result;
        } dds;
        struct {
            duisdk_play_mode_t mode;
            char *target;
            //指明target指针指向的内存是否需要释放
            bool need_free;
            //播放预处理设置
            play_preprocessor_t preprocessor;
            bool end_session;
            char *priv_data;
        } player;
        struct {
            duisdk_play_mode_t mode;
            char *target;
            char *type;
            //指明target指针指向的内存是否需要释放
            bool need_free;
            //播放预处理设置
            record_writer_t writer;
            bool end_session;
            char *priv_data;
        } recorder;
        struct {
            duisdk_state_t state;
        } fsm;
    };
} duisdk_msg_t;

//开发者通过此回调获取唤醒以及识别信息
typedef void (*duisdk_listen_cb)(duisdk_msg_t *msg, void *userdata);

//初始化sdk
DUISDK_EXPORT int duisdk_init(const char *cfg, duisdk_listen_cb listen, void *userdata);

//注册录音设备
DUISDK_EXPORT int duisdk_register_capture(capture_device_t *device);
//注册播放设备
DUISDK_EXPORT int duisdk_register_playback(playback_device_t *device);

//外部触发
DUISDK_EXPORT int duisdk_external_trigger();

//开启录音
DUISDK_EXPORT int duisdk_recorder_start();

//关闭录音
DUISDK_EXPORT int duisdk_recorder_stop();

DUISDK_EXPORT int duisdk_system_reset();

//native api response
//result可以为NULL或者json字符串
DUISDK_EXPORT int duisdk_native_response(const char *result);

//开发者mode只能是PLAY_MODE_MEDIA
DUISDK_EXPORT int duisdk_player_play(duisdk_msg_t *msg);

DUISDK_EXPORT int duisdk_player_pause();

DUISDK_EXPORT int duisdk_player_resume();

DUISDK_EXPORT int duisdk_player_stop();

DUISDK_EXPORT void duisdk_exit();

#ifdef __cplusplus
}
#endif
#endif

LOCAL_CFLAGS 	:= -rdynamic -g -O2 
LOCAL_CFLAGS  += -lhxmp3 -Ihelix-mp3 
LOCAL_LDFLAGS += -Wl,-rpath,../
LOCAL_LDFLAGS +=  -lpthread -Lhelix-mp3/  
LOCAL_LDFLAGS += -lasound


CFLAGS += $(LOCAL_CFLAGS)

audio_player:  os_log.o player.o play_pcm.o play_wav.o wav_header.o playback_impl.o capture_impl.o file_preprocessor.o http_preprocessor.o os_stream.o os_queue.o os_mutex.o os_semaphore.o os_thread.o  play_mp3.o  capture_impl.o recorder.o record_pcm.o file_writer.o player_app.o

	$(CC) -o $@ $^ $(CFLAGS) $(LOCAL_LDFLAGS) 

clean:
	rm -f audio_player 
	rm -f *.o

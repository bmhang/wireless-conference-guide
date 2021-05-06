
CFLAGS = -g -Wall
INCLUDES = -I/usr/include/opus -I/usr/include/speex

all: rtp_opus_streamer rtp_speex_streamer

rtp_opus_streamer: rtp_opus_streamer.c
	gcc $(CFLAGS) -o rtp_opus_streamer rtp_opus_streamer.c $(INCLUDES) -lm -lopus

rtp_speex_streamer: rtp_speex_streamer.c
	gcc $(CFLAGS) -o rtp_speex_streamer rtp_speex_streamer.c $(INCLUDES) -lm -lspeex

clean:
	rm -f rtp_opus_streamer rtp_speex_streamer

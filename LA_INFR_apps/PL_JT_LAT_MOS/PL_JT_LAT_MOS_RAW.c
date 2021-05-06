/* This program calculates Mean Opinion Score (MOS) of an RTP audio stream
How does it Work
----------------
This MOS calculator program is based on the libtrace frame capturing library. Each time a frame is received on a given interface (e.g. pcapint:wlan0) or from a file (e.g. pcapfile:trace.pcap), it is verified for an RTP or RTCP packet. These are the only supported packets for this program.

RTP: The Real-time Transport Protocol (RTP) defines a standardized packet format for delivering audio and video over IP networks. RTP is used extensively in communication and entertainment systems that involve streaming media, such as telephony, video teleconference applications, television services and web-based push-to-talk features. source:- http://en.wikipedia.org/wiki/Real-time_Transport_Protocol

RTCP: stands for Real Time Transport Control Protocol and is defined in RFC 3550. RTCP works hand in hand with RTP. RTP does the delivery of the actual data, where as RTCP is used to send control packets to participants in a call. The primary function is to provide feedback on the quality of service being provided by RTP. source:- http://www.3cx.com/pbx/rtcp/

An RTP audio/video stream is organized into unique sequences identified by SSRU field of the RTP header. The program creates a data structure for each sequence to hold the following data
- MOS calculation state (IDLE and START)
- Previous sequence, jitter and relative timestamp (tx and rx)
- First RTP stream packet timestamp in UNIX epoch format
- Counters for received packets, lost packets and marker packets
- Cumulative sum of jitter and latency

It is possible that an audio/video application can send more than one RTP stream and if this happens the linked list structure grows accordingly.

The end of a stream sequence is signaled by the RTCP "end of participation" packet. When the program receives such a packet, it removes one structure sequence that has a matching SSRU field.

The RTCP protocol is also used for time synchronization which is an important step during jitter calculation. Every time a new RTCP packet of the type "sender report" arrives, the time information contained in it is updated on the structure sequence that has a matching SSRU field.

If all information about a stream sequence is in place then a MOS score is calculated.

The MOS score calculation algorithm implemented in this program follows the ITU-T Perceptual Evaluation of Speech Quality (PESQ) P.862 standard. It calculates the PESQ score out of packet loss, jitter and latency parameters and latter map it onto a MOS scale.

The MOS score is calculated on aggregate basis. It collects cumulative packet loss, jitter and latency values for the last N RTP packets and it generates one MOS score value. This program uses N = 100 but the implementer can play on this parameter that best suits his/her demand.

The cumulative packet loss, jitter and latency values are converted first to the well know R score and later mapped to a MOS scale.

This application also saves RTP packets into audio files. It has an advantage when you are not sure of the MOS score values generated. Enable the RTP->audio file saving mode, listen the audio file and compare the quality with the MOS score values generated. Currently MPA/MP3 and PCMA/AU audio streams are supported.

Another funcitonality implemented in this application is the ability to select/use a specific audio stream from an aggregated stream. Some network problems require multi-language audio streaming and it is possible that an RTP packet can hold more than one language stream. Hence this application can retrieve a single audio stream depending on the language preference.

Best practice
------------- 
- Don't forget to synchronize audio/video transmitter and receiver nodes before starting the MOS program. This program uses transmitter and receiver clocks for latency calculation and unless they are synchronized MOS scores will drop sharply.

Author
------
Michael Tetemke Mehari
michael.mehari@intec.ugent.be
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <math.h>
#include <opus.h>
#include <errno.h>

#include "libtrace.h"
#include "mysql.h"
#include "uthash.h"		// hashing function header file
#include "PL_JT_LAT_MOS.h"	// custom defined header file
#include "audio_enc.h"		// AU file structure header file

#define BOLD		"\033[1m\033[30m"
#define RESET		"\033[0m"
#define MACSTR		"%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a)	(a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

int			finished = 0;
struct RECORD_t		*RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;
double 			ts_rx_real;

OpusDecoder		*decoder;
uint32_t		WAVE_subchunk2_size = 0;

FILE			*fp = NULL;		// File discriptor
libtrace_t 		*trace = NULL;		// trace discriptor
libtrace_packet_t 	*packet = NULL;		// packet discriptor
libtrace_filter_t 	*filter = NULL;		// filter discriptor

// opus frame duration (msec) conversion array from TOC byte
double opus_frame_duration[32]={ 0.01, 0.02, 0.04, 0.06, 0.01, 0.02, 0.04, 0.06, 0.01, 0.02, 0.04, 0.06, 0.01, 0.02, 0.01, 0.02, 0.0025, 0.05, 0.01, 0.02, 0.0025, 0.05, 0.01, 0.02, 0.0025, 0.05, 0.01, 0.02, 0.0025, 0.05, 0.01, 0.02};

// calculates absolute value of a real number
double Abs(double value)
{
	if(value < 0.0)
		return (-1.0 * value);
	else
		return value;
}

// Get the local mac address of the specifed interface
void get_mac_addr(char *if_name, char *mac_str)
{
	int fd;
	struct ifreq ifr;
	unsigned char* mac;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	//Copy the interface name in the ifreq structure
	strcpy(ifr.ifr_name, if_name);

	//get the mac address
	ioctl(fd, SIOCGIFHWADDR, &ifr);

	// Close the socket
	close(fd);

	mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
	sprintf(mac_str, MACSTR, MAC2STR(mac));
}


// Assign and initialize empty temporary data structure
#define	ASSIGN_STREAM_PTR(data_ptr)										\
	data_ptr->ts_tx_first_real = 0;										\
	data_ptr->ts_tx_first = 0;										\
	data_ptr->previous_ts_rx_real = ts_rx_real;								\
	data_ptr->previous_ts_tx = ts_tx;									\
	data_ptr->previous_jitter = 0;										\
	data_ptr->buffered_time = 0;										\
	data_ptr->frame_duration_sum = 0;									\
	data_ptr->frame_PL_sum = 0;										\
	data_ptr->packetLoss = 0;										\
	data_ptr->RTPcount = -1;											\
	data_ptr->jitterSum = 0;										\
	data_ptr->latencySum = 0;

// Gets a pointer to the payload following an RTP header and updated with the number of remaining bytes.
void *trace_get_payload_from_rtp(rtp_hdr_t *rtp, uint32_t *remaining)
{
	if (remaining)
	{
		if (*remaining < sizeof(rtp_hdr_t))
		{
			*remaining = 0;
			return NULL;
		}
		*remaining-=sizeof(rtp_hdr_t)+(rtp->cc+rtp->x)*sizeof(char);
	}
	return (void*)((char*)rtp+sizeof(rtp_hdr_t)+(rtp->cc+rtp->x)*sizeof(char));
}

// Retrieve frame duration
double get_frame_duration(void *audio_frame, uint8_t pt, uint32_t remaining)
{
	double frame_duration;

	// OPUS payload
	if(pt == PT_OPUS)
	{
		opus_TOC_t opus_TOC;
		opus_code3_frameCount_t opus_code3_frameCount;
		memcpy(&opus_TOC, audio_frame, sizeof(opus_TOC_t));

		if(opus_TOC.c == 0)				// One Frame in OPUS Packet
			frame_duration = opus_frame_duration[opus_TOC.config];
		else if(opus_TOC.c == 1)			// Two Frames in OPUS Packet, with Equal Compressed Sizes
			frame_duration = 2*opus_frame_duration[opus_TOC.config];
		else if(opus_TOC.c == 2)			// Two Frames in OPUS Packet, with Different Compressed Sizes
			frame_duration = 2*opus_frame_duration[opus_TOC.config];
		else if(opus_TOC.c == 3)			// A Signaled Number of Frames in OPUS Packet
		{
			memcpy(&opus_code3_frameCount, audio_frame + sizeof(opus_TOC_t), sizeof(opus_code3_frameCount_t));
			frame_duration = (opus_code3_frameCount.M)*opus_frame_duration[opus_TOC.config];
		}
	}
	// SPEEX payload
	else if(pt == PT_SPEEX)
	{
		uint8_t bytes;
		int nbBytes = 0;
		frame_duration = 0;
		do
		{
			bytes = *(uint8_t *)(audio_frame + nbBytes);
			nbBytes += bytes + 1;
		
			frame_duration += 0.02;
		}while(remaining > nbBytes);
	}
	
	return frame_duration;
}

// Retrieve the number of audio channels used
uint8_t get_frame_channel(void *audio_frame, uint8_t pt)
{
	uint8_t channels;
	if(pt == PT_OPUS)
	{
		opus_TOC_t opus_TOC;
		memcpy(&opus_TOC, audio_frame, sizeof(opus_TOC_t));
		channels = (opus_TOC.s == 0) ? 1 : 2; 
	}
	return channels;
}

// Return the RTP sampling rate based on the payload type
uint32_t get_sample_rate(uint16_t pt)
{
	// OPUS payload
	if(pt == PT_OPUS)
		return 48000;
	// SPEEX payload
	else if(pt == PT_SPEEX)
		return 16000;
	else
		return 8000;
}

// Insert a possible silence into the PCM bytes
void insert_silence()
{
	double		previous_ts_rx_real, prev_frame_duration, buffered_time;
	uint32_t	silence_len;

	previous_ts_rx_real = RECORD_ptr->previous_ts_rx_real;
	prev_frame_duration = RECORD_ptr->prev_frame_duration;
	buffered_time = RECORD_ptr->buffered_time;

	// Estimate silence length from buffered time and modify it for next round
	buffered_time += prev_frame_duration - (ts_rx_real - previous_ts_rx_real);
	if(buffered_time < 0)
	{
		silence_len = (uint32_t)(-1.0*buffered_time*RECORD_ptr->sample_rate);
		RECORD_ptr->buffered_time = 0;
	}
	else
	{
		silence_len = 0;
		RECORD_ptr->buffered_time = buffered_time;
	}

	// If silence needs to be added
	if(silence_len > 0)
	{
		// Allocate empty buffer for silence.
		void *silence_payload = calloc(silence_len, sizeof(uint16_t));

		// Insert silence
		fwrite(silence_payload, sizeof(uint16_t), silence_len, fp);

		// Free the memory once silence insertion is finished
		free(silence_payload);

		// Update WAVE chunck size
		WAVE_subchunk2_size += silence_len*sizeof(uint16_t);
	}
}

unsigned char *decode_OPUS_to_pcm(void *audio_frame, uint32_t remaining, uint32_t frame_size, char *saveStream)
{
	int		err;
	int16_t		*out = NULL;

	// If this is the first RTP packet, create a new decoder state and initialize the WAVE file header
	if(RECORD_ptr->RTPcount == -1)
	{
		WAVE_filehdr_t	filehdr;
		uint32_t 	fmt = htonl(0x666d7420);

		// Open file in writing mode to save the decoded RTP stream
		fp = (strcmp(saveStream, "STDOUT") != 0) ? fopen(saveStream, "wb") : stdout;
		if(fp == NULL)
		{
			fprintf(stderr, "Can't open the file %s for writing!\n",saveStream);
			return NULL;
		}

		/* Create a new opus decoder state. */
		decoder = opus_decoder_create(RECORD_ptr->sample_rate, RECORD_ptr->channels, &err);
		if (err<0)
		{
			fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
			fclose(fp);
			return NULL;
		}

		strcpy(filehdr.chunk_id, "RIFF");					// RIFF
		filehdr.chunk_size = 0xFFFFFFFF;					// Entire audio file size after this point
		strcpy(filehdr.format, "WAVE");						// WAVE
		memcpy(filehdr.subchunk1_id, &fmt, 4);					// fmt
		filehdr.subchunk1_size = 16;						// 16 bytes of sub chunck1 size
		filehdr.audio_format = 1;						// PCM (i.e. Linear quantization)
		filehdr.channels = RECORD_ptr->channels;				// Mono channel
		filehdr.sampleRate = RECORD_ptr->sample_rate;				// Fullband sampling rate
		filehdr.bits_per_sample = 8*sizeof(uint16_t);				// 16 bits PCM
		filehdr.block_align = RECORD_ptr->channels*sizeof(uint16_t);		// NumChannels * BitsPerSample/8
		filehdr.byte_rate = RECORD_ptr->sample_rate*filehdr.block_align;	// sampleRate * NumChannels * BitsPerSample/8
		strcpy(filehdr.subchunk2_id, "data");					// data
		filehdr.subchunk2_size = 0xFFFFFFFF;					// Audio data size

		fwrite(&filehdr , sizeof(WAVE_filehdr_t), 1, fp);
	}
	else if(strcmp(saveStream, "STDOUT") != 0)
	{
		// Insert a possible silence into the WAVE file
		insert_silence();
	}

	// Reallocate the decoding output buffer
	out = (int16_t *)realloc(out, RECORD_ptr->channels*frame_size*sizeof(uint16_t));
	if(out == NULL)
	{
		fprintf(stderr, "decoder failure: %s\n", opus_strerror(err));
		fclose(fp);
		return NULL;
	}

	// Decode the OPUS audio data
	if(opus_decode(decoder, (unsigned char *)audio_frame, remaining, out, frame_size, 0) != frame_size)
	{
		fprintf(stderr, "decoder failure: %s\n", opus_strerror(err));
		free(out);
		fclose(fp);
		return NULL;
	}

	return (unsigned char *) out;
}

unsigned char *decode_SPEEX_to_pcm(void *audio_frame, uint32_t remaining, uint32_t frame_size, char *saveStream)
{
	int		err;
	int16_t		*out = NULL;

	// If this is the first RTP packet, create a new decoder state and initialize the WAVE file header
	if(RECORD_ptr->RTPcount == -1)
	{
		WAVE_filehdr_t	filehdr;
		uint32_t 	fmt = htonl(0x666d7420);

		// Open file in writing mode to save the decoded RTP stream
		fp = fopen(saveStream, "wb");
		if(fp == NULL)
		{
			fprintf(stderr, "Can't open the file %s for writing!\n",saveStream);
			return NULL;
		}

		/* Create a new opus decoder state. */
		decoder = opus_decoder_create(RECORD_ptr->sample_rate, RECORD_ptr->channels, &err);
		if (err<0)
		{
			fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
			fclose(fp);
			return NULL;
		}

		strcpy(filehdr.chunk_id, "RIFF");					// RIFF
		filehdr.chunk_size = 0xFFFFFFFF;					// Entire audio file size after this point
		strcpy(filehdr.format, "WAVE");						// WAVE
		memcpy(filehdr.subchunk1_id, &fmt, 4);					// fmt
		filehdr.subchunk1_size = 16;						// 16 bytes of sub chunck1 size
		filehdr.audio_format = 1;						// PCM (i.e. Linear quantization)
		filehdr.channels = RECORD_ptr->channels;				// Mono channel
		filehdr.sampleRate = RECORD_ptr->sample_rate;				// Fullband sampling rate
		filehdr.bits_per_sample = 8*sizeof(short);				// 16 bits PCM
		filehdr.block_align = RECORD_ptr->channels*sizeof(short);		// NumChannels * BitsPerSample/8
		filehdr.byte_rate = RECORD_ptr->sample_rate*filehdr.block_align;	// sampleRate * NumChannels * BitsPerSample/8
		strcpy(filehdr.subchunk2_id, "data");					// data
		filehdr.subchunk2_size = 0xFFFFFFFF;					// Audio data size

		fwrite(&filehdr , sizeof(WAVE_filehdr_t), 1, fp);
	}
	else
	{
		// Insert a possible silence into the WAVE file
		insert_silence();
	}

	// Reallocate the decoding output buffer
	out = (int16_t *)realloc(out, RECORD_ptr->channels*frame_size*sizeof(short));
	if(out == NULL)
	{
		fprintf(stderr, "decoder failure: %s\n", opus_strerror(err));
		fclose(fp);
		return NULL;
	}

	// Decode the OPUS audio data
	if(opus_decode(decoder, (unsigned char *)audio_frame, remaining, out, frame_size, 0) != frame_size)
	{
		fprintf(stderr, "decoder failure: %s\n", opus_strerror(err));
		free(out);
		fclose(fp);
		return NULL;
	}

	return (unsigned char *) out;
}

// jitter calculation using RFC3550 (RTP)
double calculateJitter(double previous_jitter, double previous_ts_rx_real, uint32_t previous_ts_tx, uint32_t ts_tx, uint32_t sample_rate)
{
	double jitter = previous_jitter + (Abs(ts_rx_real - previous_ts_rx_real - (double)((ts_tx - previous_ts_tx)/sample_rate)) - previous_jitter) / 16.0;
	return jitter;
}

// One way latency measurement
double calculateLatency(double ts_tx_first_real, uint32_t ts_tx_first, uint32_t ts_tx, uint32_t sample_rate)
{
	double latency = Abs(ts_rx_real - (ts_tx_first_real + (double)(ts_tx-ts_tx_first)/sample_rate));
	return latency;
}

// To convert from NTP timestamp to UNIX timeval
void convert_ntp_time_into_unix_time(struct ntp_time_t *ntp_ts, struct timeval *unix_ts)
{
	unix_ts->tv_sec = ntp_ts->second - 2208988800; // the seconds from Jan 1, 1900 to Jan 1, 1970
	unix_ts->tv_usec = (uint32_t)(ntp_ts->fraction / 4294.967296);
}

// PESQ score calculation out of packet loss, jitter and latency network parameters and mapping into a MOS scale.
// http://www.nessoft.com/kb/50
#define	normMOS_RX												\
	averagePacketLoss = RECORD_ptr->packetLoss/(RECORD_ptr->packetLoss + RECORD_ptr->RTPcount);		\
	averageJitter = Abs(RECORD_ptr->jitterSum/RECORD_ptr->RTPcount);					\
	averageLatency = RECORD_ptr->latencySum/RECORD_ptr->RTPcount;						\
	effectiveLatency = averageLatency + 2*averageJitter + 0.01;						\
														\
	if(effectiveLatency < 0.16)										\
		R = maxR - 25*effectiveLatency;									\
	else													\
		R = maxR - (100*effectiveLatency - 12);								\
														\
	R = R - 250*averagePacketLoss;										\
														\
	MOS_norm = (R < 0) ? 0 : ((0.01) * R + (1.75e-6)*R*(R-60)*(100-R));

// Insert Packet Loss, Jitter, Latency and MOS score into database
void insert_PL_JT_LAT_MOS_score(MYSQL *conn, uint64_t current_time, char *src_mac, char *sniffer_mac, double averagePacketLoss, double averageJitter, double averageLatency, double MOS_norm)
{
	char query_statement[256];

	// factual_packet DB query statement and execution
	snprintf(query_statement, 256, "INSERT INTO PL_JT_LAT_MOS VALUES(%lu,'%s','%s',%f,%f,%f,%f)", current_time, src_mac, sniffer_mac, averagePacketLoss, averageJitter, averageLatency, MOS_norm);
	if(mysql_query(conn, query_statement) != 0)
	{
		fprintf(stderr, "PL_JT_LAT_MOS DB query error!\n");
		return;
	}
}

// Convert normalized MOS score to R values using modified bairstow's polynomial evaluation method
double normMOS_to_R(double MOS_norm)
{
	// f(R) = R^3 - 160R^2 + 2000/7R + 4*MOS_norm/0.000007
	double fn[4] = {571428.571428571*MOS_norm, 285.714285, -160, 1};
	double epsilon = 1e-10;				// stopping criterion
	double roots[3];				// root holding colomn vector
	double b[4];					// remainder holding variable
	double c[4];					// correction holding variable
	double r=1, s=2, dr, ds;			// coorection parameter for r and s
	int8_t i;

	while(1)
	{
		// iterative procedure to calculate the b coefficients
		b[3] = fn[3];
		b[2] = fn[2] + b[3]*r;
		for(i=1; i>=0; i--)
			b[i]=fn[i] + r*b[i+1] + s*b[i+2];
	   
		if (b[0]<epsilon && b[1]<epsilon)		// if remainder is sufficiently small
		{
			roots[0] = 0.5*(r + sqrt(r*r+4*s));	// root 1
			roots[1] = 0.5*(r - sqrt(r*r+4*s));	// root 2
			roots[2] = -b[1]/b[2];			// root 3
			break;
		}
		else
		{
			// iterative procedure to calculate the c coefficients
			c[3] = b[3];
			c[2] = b[2] + c[3]*r;
			for(i=1; i>=0; i--)
			   c[i] = b[i] + r*c[i+1] + s*c[i+2];

			dr = (b[1]*c[2]-b[0]*c[3])/(c[1]*c[3]-c[2]*c[2]);
			ds = (b[0]*c[2]-b[1]*c[1])/(c[1]*c[3]-c[2]*c[2]);

			r += dr;
			s += ds;
		}
	}

	// Return the maximum R value
	if(roots[0] < 100 && roots[0] > 0)
		return roots[0];
	else if(roots[1] < 100 && roots[1] > 0)
		return roots[1];
	else if(roots[2] < 100 && roots[2] > 0)
		return roots[2];
	else
		return 93.2;
}

/* This function is experimental. It tries to evaluate the normalized MOS score for different encoder bitrates (kbps) as a funciton of encoder type, audio signal bandwidth and encoder bitrate */
double MOS_enc(uint16_t pt, char *audio_BW, double bitrate)
{
	double x = bitrate;

	// OPUS encoding
	if(pt == PT_OPUS)
	{
		// Narrow band signal
		if(strcmp(audio_BW, "NARROW-BAND") == 0)
		{
			if(bitrate <= 20.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 3.47755328189003e-06, b = -2.67166900312929e-04, c = 8.20925600818548e-03, d = -1.27020852209707e-01, e = 1.00001497249875, f = -2.30990939984253;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 20.0)
				return 0.9376;
			else
				return 0.5714;
		}
		// Medium band signal
		else if(strcmp(audio_BW, "MEDIUM-BAND") == 0)
		{
			if(bitrate <= 25.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 4.91874147468462e-07, b = -4.94878327186501e-05, c =	1.98537222975071e-03, d = -4.02337679860307e-02, e = 4.20643199817638e-01, f = -9.36075470817430e-01;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 25.0)
				return 0.9276;
			else
				return 0.5079;
		}
		// Wideband signal
		else if(strcmp(audio_BW, "WIDE-BAND") == 0)
		{
			if(bitrate <= 35.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 2.18805762755069e-07, b = -2.67036238054913e-05, c = 1.27135673182267e-03, d = -2.97438959527979e-02, e = 3.48718494477557e-01, f = -7.83557213137123e-01;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 35.0)
				return 0.9147;
			else
				return 0.4797;
		}
		// Supper wideband signal
		else if(strcmp(audio_BW, "SUPPER-WIDE-BAND") == 0)
		{
			if(bitrate <= 41.0 && bitrate >= 8.7)
			{
				// 7th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^7 + bx^6 + cx^5 + dx^4 + ex^3 + fx^2 + gx + h
				double a = 1.89491748228137e-09, b = -3.64617249621041e-07, c = 2.92819261703315e-05, d = -1.26921705706618e-03, e = 3.20080289687522e-02, f = -4.69600829260597e-01, g = 3.73035932685573, h = -11.7287903519960;
				return ((((((a*x + b)*x + c)*x + d)*x + e)*x + f)*x + g)*x + h;
			}
			else if(bitrate > 41.0)
				return 0.9043;
			else
				return 0.2958;
		}
		// Fullband signal
		else if(strcmp(audio_BW, "FULL-BAND") == 0)
		{
			if(bitrate <= 50.0 && bitrate >= 8.5)
			{
				// 9th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^9 + bx^8 + cx^7 + dx^6 + ex^5 + fx^4 + gx^3 + hx^2 + ix + j
				double a = 1.09702479278367e-12, b = -3.11159330808731e-10, c = 3.83347910521901e-08, d = -2.68760622413805e-06, e = 1.17966458339847e-04, f = -3.35730316669723e-03, g = 6.19291665885357e-02, h = -7.15228416905110e-01, i = 4.72561014094321, j = -12.9908486208936;
				return ((((((((a*x + b)*x + c)*x + d)*x + e)*x + f)*x + g)*x + h)*x + i)*x + j;
			}
			else if(bitrate > 50.0)
				return 0.9113;
			else
				return 0.3438;
		}
		else
			return -1;
	}
	// SPEEX encoding
	else if(pt == PT_SPEEX)
	{
		// Narrow band signal
		if(strcmp(audio_BW, "NARROW-BAND") == 0)
		{
			return -1;
		}
		// Medium band signal
		else if(strcmp(audio_BW, "MEDIUM-BAND") == 0)
		{
			return -1;
		}
		// Wideband signal
		else if(strcmp(audio_BW, "WIDE-BAND") == 0)
		{
			if(bitrate <= 25.0 && bitrate >= 2.4)
			{
				// 5th degree polynomial approximation of speex encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 2.27024851581720e-07, b = -2.31212177346099e-05, c = 9.27816218582455e-04, d = -1.86631060213156e-02, e = 1.96110851677524e-01, f = -7.70503160267407e-02;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 25.0)
				return 0.8437;
			else
				return 0.2982;
		}
		else
			return -1;
	}
	else
		return -1;
}

/* Due to the amount of error checking required in our main function, it
 * is a lot simpler and tidier to place all the calls to various libtrace
 * destroy functions into a separate function.
 */
void libtrace_cleanup(libtrace_t *trace, libtrace_packet_t *packet, libtrace_filter_t *filter)
{
	/* It's very important to ensure that we aren't trying to destroy
	 * a NULL structure, so each of the destroy calls will only occur
	 * if the structure exists */
	if (trace)
		trace_destroy(trace);

	if (packet)
		trace_destroy_packet(packet);

	if (filter)
		trace_destroy_filter(filter);
}

// cleanup resources upon SIGINT and SIGTERM
static void cleanup(int signo)
{
	finished = 1;
}

// main function
int main(int argc, char *argv[])
{
	char *host = NULL;
	char *user = NULL;
	char *passwd = NULL;
	char *db = NULL;
	char *traceURI = NULL;
	char *traceURI_type;
	char *traceURI_if;
	uint16_t RTPPort = 5004;
	uint16_t RTCPPort = 5005;
	char *bpfStr = "";
	char *audio_BW = NULL;
	uint32_t interval = 400000;
	char *saveStream = NULL;
	uint16_t lang_ID = 1;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:f:p:b:a:i:s:L:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr,"Description\n");
				fprintf(stderr,"-----------\n");
				fprintf(stderr,BOLD "MOS estimator" RESET " is an application built on top of the libtrace packet capturing library which\n");
				fprintf(stderr,"captures RTP packets from an interface, generate a MOS score estimator and stores it into database.\n");
				fprintf(stderr,"MOS is calculated from packet loss, latency and jitter network parameters over 1msec time gap.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-f traceURI\t\ttrace format type [eg. -f int:wlan0]\n");
				fprintf(stderr,"-p RTPPort\t\tRTP port of the incoming audio [eg. -p 5004]\n");
				fprintf(stderr,"-b filter\t\taccept only packets that match the bpf filter expression [eg. -b 'not ether src host 00:0e:8e:30:9e:68']\n");
				fprintf(stderr,"-a audio_BW\t\tbandwidth of the audio stream (NARROW-BAND, MEDIUM-BAND, WIDE-BAND, SUPPER-WIDE-BAND, FULL-BAND) [eg. -b WIDE-BAND]\n");
				fprintf(stderr,"-i interval\t\tinterval (usec) between periodic reports [eg. -i 400000]\n");
				fprintf(stderr,"-s saveStream\t\tsave RTP stream into a WAVE audio file or STDOUT [eg. -s /tmp/sample.wav or -s STDOUT]\n");
				fprintf(stderr,"-L lang_ID\t\tLanguage Id where audio is translated and streamed to (only for simulation) [e.g. -L 1]\n\n");
				return 1;
			case 'n':
				host = strdup(optarg);
				break;
			case 'u':
				user = strdup(optarg);
				break;
			case 'k':
				passwd = strdup(optarg);
				break;
			case 'd':
				db = strdup(optarg);
				break;
			case 'f':
				traceURI = strdup(optarg);
				traceURI_type = strdup(traceURI);
				traceURI_type = strtok(traceURI_type,":");
				traceURI_if = strtok(NULL,"\0");
				break;
			case 'p':
				RTPPort = (uint16_t)atoi(optarg);
				RTCPPort = RTPPort+1;
				break;
			case 'b':
				bpfStr = strdup(optarg);
				break;
			case 'a':
				audio_BW = strdup(optarg);
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			case 's':
				saveStream = strdup(optarg);
				break;
			case 'L':
				lang_ID = (uint16_t)atoi(optarg);
				break;
			default:
				fprintf(stderr,"MOS: missing operand. Type './MOS -h' for more information.\n");
				return 1;
	    }
	}

	// House keeping the command line input
	if(host == NULL || user == NULL || passwd == NULL || db == NULL || traceURI == NULL || audio_BW == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database/Trace format/Audio stream BW] is not specified. Type './MOS -h' for more information\n");
		return 1;
	}

	// Make sure audio stream bandwidth is with in (NARROW-BAND, MEDIUM-BAND, WIDE-BAND, SUPPER-WIDE-BAND, FULL-BAND)
	if(strcmp(audio_BW, "NARROW-BAND") != 0 && strcmp(audio_BW, "MEDIUM-BAND") != 0 && strcmp(audio_BW, "WIDE-BAND") != 0 && strcmp(audio_BW, "SUPPER-WIDE-BAND") != 0 && strcmp(audio_BW, "FULL-BAND") != 0)
	{
		fprintf(stderr,"Wrong audio stream bandwidth selected. Supported bandwidths are [NARROW-BAND, MEDIUM-BAND, WIDE-BAND, SUPPER-WIDE-BAND and FULL-BAND]\n");
		return 1;
	}

	MYSQL 			*conn = NULL;

	uint32_t 		remaining, L2_pl_len;
	uint8_t 		transProt;
	libtrace_udp_t 		*udp;
	libtrace_linktype_t	linktype;
	libtrace_80211_t	*L2_header;

	char			sniffer_mac[18];

	void			*payload;
	rtp_hdr_t 		*rtp_hdr;
	rtcp_senderRpt_hdr_t 	*rtcp_senderRpt_hdr;
	rtcp_endPrtp_hdr_t 	*rtcp_endPrtp_hdr;
	uint8_t 		RTCP_pkt_type, RTCP_pkt_length;
	uint32_t		stream_id;

	struct timeval		systime;
	uint64_t		previous_time, current_time;

	langhdr_t		*langhdr;
	int			i;
	void			*audio_frame;

	struct ntp_time_t 	ts_tx_NTP;
	struct timeval 		ts_tx_UNIX;

	uint32_t		ts_tx;
	double			averagePacketLoss, averageJitter, averageLatency, effectiveLatency;
	double 			latency, jitter, R, maxR, MOS_norm;

	unsigned char		*pcm_bytes;
	uint32_t		frame_size;
	double			frame_duration;

	// handle SIGTERM and SIGINT
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	// initialize mysql database
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		fprintf(stderr, "Initializing MYSQL: %s\n", strerror(errno));
		return 1;
	}

	// connect to qocon database
	if (mysql_real_connect(conn, host, user, passwd, db, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "connecting to MYSQL: %s\n", strerror(errno));
		return 1;
	}

	/* Creating and initialising a packet structure to store the packets
	 * that we're going to read from the trace */
	packet = trace_create_packet();

	if (packet == NULL) {
		/* Unfortunately, trace_create_packet doesn't use the libtrace
		 * error system. This is because libtrace errors are associated
		 * with the trace structure, not the packet. In our case, we
		 * haven't even created a trace at this point so we can't
		 * really expect libtrace to set an error on it for us, can
		 * we?
		 */
		perror("Creating libtrace packet");
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}
	trace = trace_create(traceURI);

	if (trace_is_err(trace)) {
		trace_perror(trace,"Opening trace file");
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}

	if (trace_start(trace) == -1) {
		trace_perror(trace,"Starting trace");
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}

	// Create the bpf filter
	filter = trace_create_filter(bpfStr);
	if (filter == NULL) {
		trace_perror(trace,"Failed to create filter(%s)\n", bpfStr);
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}

	// Retrieve sniffer MAC address
	get_mac_addr(traceURI_if, sniffer_mac);

	// Populate next round parameters
	gettimeofday(&systime, NULL);
	previous_time = 1e6*systime.tv_sec + systime.tv_usec;

	/* This loop will read packets from the trace until either EOF is
	 * reached or an error occurs (hopefully the former!)
	 *
	 * Remember, EOF will return 0 so we only want to continue looping
	 * as long as the return value is greater than zero
	 */
	while (trace_read_packet(trace,packet)>0 && finished == 0)
	{
		/* Apply the filter to the packet */
		if(trace_apply_filter(filter, packet) <= 0)
			continue;

		// packet timestamp at receiver
		ts_rx_real = trace_get_seconds(packet);

		// Get udp pointer
		udp = (libtrace_udp_t *)trace_get_transport(packet, &transProt, &remaining);

		// Check if destination port is RTP
		if(udp != NULL && ntohs(udp->dest) == RTPPort)
		{
			// Update the remaining udp packet size 
			remaining = ntohs(udp->len);

			// Gets a pointer to the UDP payload 
			if(!(payload = trace_get_payload_from_udp(udp, &remaining)))
				continue;

			// retrieve the RTP header
			rtp_hdr = (rtp_hdr_t *)payload;

			// relative timestamp from transmitter
			ts_tx = ntohl(rtp_hdr->ts_tx);

			// Gets a pointer to the language header
			if(!(langhdr = (langhdr_t *)trace_get_payload_from_rtp(rtp_hdr, &remaining)))
				continue;

			// Does the RTP payload contains a language header
			if(ntohl(langhdr->magic_no) == 0x6564656e)
			{
				// Check if our language exists
				i = remaining/ntohs(langhdr->data_len);
				for(; i>0; i--)
				{
					// If we found a language ID match
					if(ntohs(langhdr->lang_ID) == lang_ID)
					{
						audio_frame = (void *)((uint8_t *)langhdr + sizeof(langhdr_t));
						remaining = ntohs(langhdr->data_len) - sizeof(langhdr_t);
						break;
					}

					langhdr = (langhdr_t *)((uint8_t *)langhdr + ntohs(langhdr->data_len));
				}

				// If our language does not exist, do not further process the RTP packet
				if(i == 0)
					continue;
			}
			// Else, the audio frame is the RTP payload itself
			else
				audio_frame = (void *)langhdr;

			stream_id = ntohl(rtp_hdr->ssrc);

			// If there is no rtp stream instance
			// Hash table implementation for c : https://github.com/troydhanson/uthash
			HASH_FIND_INT(hash_ptr, &stream_id, RECORD_ptr);
			if(RECORD_ptr == NULL)
			{
				RECORD_ptr= (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
				if(RECORD_ptr == NULL)
				{
					fprintf(stderr, "Unable to create record!\n");
					return 1;
				}

				// Assign and initialize empty data structure
				RECORD_ptr->stream_id = stream_id;
				ASSIGN_STREAM_PTR(RECORD_ptr);

				// Add the new record to the hash table
				HASH_ADD_INT(hash_ptr, stream_id, RECORD_ptr);
			}

			// Retrieve frame duration
			frame_duration = get_frame_duration(audio_frame, rtp_hdr->pt, remaining);

			// Calculate aggregate frame duration for estimating the average encoding bitrate
			RECORD_ptr->frame_duration_sum += frame_duration;

			// Calculate aggregate payload size (bytes) for estimating the average encoding bitrate
			if(rtp_hdr->pt == PT_OPUS)
				RECORD_ptr->frame_PL_sum += remaining;
			else if(rtp_hdr->pt == PT_SPEEX)
				RECORD_ptr->frame_PL_sum += (remaining - frame_duration/0.02);

			// First RTP packet
			if(RECORD_ptr->RTPcount == -1)
			{
				// Retrieve Layer 2 source mac address
				L2_header = trace_get_layer2(packet, &linktype, &L2_pl_len);
				trace_ether_ntoa(L2_header->mac2, RECORD_ptr->src_mac);

				// Store RTP (previous) sequence number for next round
				RECORD_ptr->previous_seq = ntohs(rtp_hdr->seq);

				// Calculate stream sample rate
				RECORD_ptr->sample_rate = get_sample_rate(rtp_hdr->pt);

				// Retrieve the number of audio channels used
				RECORD_ptr->channels = get_frame_channel(audio_frame, rtp_hdr->pt);

				// Sender Report RTCP is expected before any RTP packet. Incase it doesn't appear, approximate first real and relative transmit timestamps
				if(RECORD_ptr->ts_tx_first_real == 0 && RECORD_ptr->ts_tx_first == 0)
				{
					RECORD_ptr->ts_tx_first_real = ts_rx_real;
					RECORD_ptr->ts_tx_first = ts_tx;
				}
			}
			else
			{
				// Calculate packet loss
				if(ntohs(rtp_hdr->seq) - RECORD_ptr->previous_seq > 1)
					RECORD_ptr->packetLoss += ntohs(rtp_hdr->seq) - RECORD_ptr->previous_seq - 1;
				else if(RECORD_ptr->previous_seq - ntohs(rtp_hdr->seq) > 32768)
					RECORD_ptr->packetLoss += ntohs(rtp_hdr->seq) - RECORD_ptr->previous_seq + 65535;

				// calculate jitter
				jitter = calculateJitter(RECORD_ptr->previous_jitter, RECORD_ptr->previous_ts_rx_real, RECORD_ptr->previous_ts_tx, ts_tx, RECORD_ptr->sample_rate);
				RECORD_ptr->jitterSum += jitter;

				// calculate latency
				latency = calculateLatency(RECORD_ptr->ts_tx_first_real, RECORD_ptr->ts_tx_first, ts_tx, RECORD_ptr->sample_rate);
				RECORD_ptr->latencySum += latency;

				// Get the current time
				gettimeofday(&systime, NULL);
				current_time = 1e6*systime.tv_sec + systime.tv_usec;

				// Reporting interval has expired. Time to calculate MOS and store it into DB
				if((current_time - previous_time) > interval)
				{
					// Calculate average bitrate in kbps
					double bitrate_avg = 1e-3*8*RECORD_ptr->frame_PL_sum/RECORD_ptr->frame_duration_sum;

					// Calculate maximum weighted R value
					maxR = normMOS_to_R(MOS_enc(rtp_hdr->pt, audio_BW, bitrate_avg));

					// MACRO to calculate normalized recieved MOS score
					normMOS_RX;

					// insert packetLoss, jitter, latency and MOS score into database
					insert_PL_JT_LAT_MOS_score(conn, current_time, RECORD_ptr->src_mac, sniffer_mac, averagePacketLoss, averageJitter, averageLatency, MOS_norm);

//					printf("%f\t%f\t%f\t%f\t%f\n", averagePacketLoss, averageJitter, averageLatency, bitrate_avg, MOS_norm);
//					fflush(stdout);

					// reset variables for next round
					RECORD_ptr->frame_duration_sum = 0;
					RECORD_ptr->frame_PL_sum = 0;
					RECORD_ptr->RTPcount = 0;
					RECORD_ptr->packetLoss = 0;
					RECORD_ptr->jitterSum = 0;
					RECORD_ptr->latencySum = 0;

					// update parameters for next round
					previous_time = current_time;
				}

				// Store previous parameters for next round
				RECORD_ptr->previous_seq = ntohs(rtp_hdr->seq);
				RECORD_ptr->previous_jitter = jitter;
				RECORD_ptr->previous_ts_rx_real = ts_rx_real;
			}

			// If stream saving is enabled
			if(saveStream != NULL)
			{
				// Calculate audio frame size
				frame_size = frame_duration * RECORD_ptr->sample_rate;

				// Decode OPUS payload
				if(rtp_hdr->pt == PT_OPUS)
					pcm_bytes = decode_OPUS_to_pcm(audio_frame, remaining, frame_size, saveStream);
				// Decode SPEEX payload
				else if(rtp_hdr->pt == PT_SPEEX)
					pcm_bytes = decode_SPEEX_to_pcm(audio_frame, remaining, frame_size, saveStream);

				// Store decoded pcm into WAVE container file
				if(pcm_bytes != NULL)
				{
					// Save PCM audio frame into file
					fwrite(pcm_bytes, sizeof(char), RECORD_ptr->channels*frame_size*sizeof(uint16_t), fp);

					if(strcmp(saveStream, "STDOUT") != 0)
					{
						// Store previous WAVE play duration for next round
						RECORD_ptr->prev_frame_duration = frame_duration;

						// Update WAVE chunck size
						WAVE_subchunk2_size += RECORD_ptr->channels*frame_size*sizeof(uint16_t);
					}
				}
				else
					saveStream = NULL;
			}

			// Count total and interval RTP packets
			RECORD_ptr->RTPcount++;

			// Update previous parameter for next round
			RECORD_ptr->previous_ts_tx = ts_tx;
		}
		// Check if destination port is RTCP */
		else if(udp != NULL && ntohs(udp->dest) == RTCPPort)
		{
			// Get udp pointer
			udp = (libtrace_udp_t *)trace_get_transport(packet, &transProt, &remaining);

			// Update the remaining udp packet size 
			remaining = ntohs(udp->len);
			
			// Gets a pointer to the UDP payload 
			if(!(payload = trace_get_payload_from_udp(udp,&remaining)))
				continue;

			while(remaining > 0)
			{
				RTCP_pkt_type = *(uint8_t *)(payload+1);
				// Sender report messages
				if(RTCP_pkt_type == RTCP_SR)
				{
					// retrieve the RTCP sender report message
					rtcp_senderRpt_hdr = (rtcp_senderRpt_hdr_t *)payload;

					// relative timestamp from transmitter
					ts_tx = ntohl(rtcp_senderRpt_hdr->ts_tx_RTP);

					stream_id = ntohl(rtcp_senderRpt_hdr->ssrc);
						
					// If there is no rtp stream instance
					// Hash table implementation for c : https://github.com/troydhanson/uthash
					HASH_FIND_INT(hash_ptr, &stream_id, RECORD_ptr);
					if(RECORD_ptr == NULL)
					{
						RECORD_ptr= (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
						if(RECORD_ptr == NULL)
						{
							fprintf(stderr, "Unable to create record!\n");
							return 1;
						}

						// Assign and initialize empty data structure
						RECORD_ptr->stream_id = stream_id;
						ASSIGN_STREAM_PTR(RECORD_ptr);

						// Add the new record to the hash table
						HASH_ADD_INT(hash_ptr, stream_id, RECORD_ptr);
					}

					// Fill in the first RTP timestamp
					RECORD_ptr->ts_tx_first = ts_tx;

					// retrieve the NTP timestamp
					ts_tx_NTP.second = ntohl((uint32_t)rtcp_senderRpt_hdr->ts_tx_NTP.second);
					ts_tx_NTP.fraction = ntohl((uint32_t)rtcp_senderRpt_hdr->ts_tx_NTP.fraction);

					// convert the NTP timestamp to UNIX timestamp for the coming latency measurment
					convert_ntp_time_into_unix_time(&ts_tx_NTP, &ts_tx_UNIX);
					RECORD_ptr->ts_tx_first_real = ts_tx_UNIX.tv_sec + 1e-6*ts_tx_UNIX.tv_usec;
				}
				// End of participation message
				else if(RTCP_pkt_type == RTCP_BYE)
				{
					// retrieve the RTCP end of participation message
					rtcp_endPrtp_hdr = (rtcp_endPrtp_hdr_t *)payload;

					stream_id = ntohl(rtcp_endPrtp_hdr->ssrc);

					// retrieve rtp data structure referred by the end of participation message 
					HASH_FIND_INT(hash_ptr, &stream_id, RECORD_ptr);
					if(RECORD_ptr != NULL)
					{
						// Before removing the stream instance, calculate MOS score and store it into database
						if(RECORD_ptr->RTPcount > 0)
						{
							// Get the current time
							gettimeofday(&systime, NULL);
							current_time = 1e6*systime.tv_sec + systime.tv_usec;

							// MACRO to calculate Unfinished MOS score
							normMOS_RX;

							// insert packetLoss, jitter, latency and MOS score into database
							insert_PL_JT_LAT_MOS_score(conn, current_time, RECORD_ptr->src_mac, sniffer_mac, averagePacketLoss, averageJitter, averageLatency, MOS_norm);
						}

						// free the memory reserved for the rtp RECORD structure
						HASH_DEL(hash_ptr, RECORD_ptr);
					}
				}

				// Check remaining RTCP packets by adjusting the payload pointer
				RTCP_pkt_length = 4*(htons(*(uint16_t *)(payload+2)) + 1);
				remaining -= RTCP_pkt_length;
				payload += RTCP_pkt_length;
			}
		}
	}

	// Iterate through hash table and calculate the average MOS score
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		// Before removing the stream instance, calculate MOS score
		if(RECORD_ptr->RTPcount > 0)
		{
			// Get the current time
			gettimeofday(&systime, NULL);
			current_time = 1e6*systime.tv_sec + systime.tv_usec;

			// MACRO to calculate Unfinished MOS score
			normMOS_RX;

			// insert packetLoss, jitter, latency and MOS score into database
			insert_PL_JT_LAT_MOS_score(conn, current_time, RECORD_ptr->src_mac, sniffer_mac, averagePacketLoss, averageJitter, averageLatency, MOS_norm);
		}

		// free the memory reserved for the rtp RECORD structure
		HASH_DEL(hash_ptr, RECORD_ptr);
	}

	// close file in which RTP stream is saved
	if(saveStream != NULL)
	{
		// Before closing the file, update the chucksize on the WAVE file header
		uint32_t WAVE_chunk_size = WAVE_subchunk2_size+sizeof(WAVE_filehdr_t)-8;
		// seek to the 4th byte of the file and write the entire chunk size
		fseek(fp, 4, SEEK_SET);
		fwrite(&WAVE_chunk_size , sizeof(char), 4, fp);

		// seek to the 4th byte before the end of the file and write the data chunk size
		fseek(fp, sizeof(WAVE_filehdr_t)-4, SEEK_SET);
		fwrite(&WAVE_subchunk2_size , sizeof(char), 4, fp);

		fclose(fp);
	}

	/* If the trace is in an error state, then we know that we fell out of
	 * the above loop because an error occurred rather than EOF being
	 * reached. Therefore, we should probably tell the user that something
	 * went wrong
	 */
	if (trace_is_err(trace)) {
		trace_perror(trace,"Reading packets");
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}

	// clean libtrace resources
	libtrace_cleanup(trace, packet, filter);

	// clean mysql database resources
	mysql_close(conn);

	return 0;
}


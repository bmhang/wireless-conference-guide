#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <opus.h>

#include "rtp_streamer.h"
#include "audio_enc.h"

#define BOLD			"\033[1m\033[30m"
#define RESET			"\033[0m"

#define MAX_OPUS_PL_SIZE	(MAX_RTP_PL_SIZE - sizeof(langPkg_t))
#define MIN_bitRate		6000
#define MAX_bitRate		256000

static int	finished = 0;
uint32_t	sent_packets = 0;

uint8_t		frameLen = 20;			// 20msec frame length;
uint16_t	frameSize;
uint32_t	bitRate = 64000;		// 64kpbs output bitRate

FILE		*fp;
char		enc_par_ts[12], temp_ts[12];

// MACRO definition for Sending packet over a UDP socket
#define SEND_PACKET(sock, buf, len, flags)				\
if(send(sock, buf, len, flags) < 0)					\
{									\
	if (errno == ENOBUFS)						\
		fprintf(stderr, "Output buffers full, waiting...\n");	\
	else								\
		perror("Error while sending report packet");		\
}

// Convert an audio frame length into OPUS TOC config field
uint8_t frameLen_to_TOC_config(uint8_t frameLen)
{
	switch(frameLen)
	{
	    case 10:			// 10msec frame length
	   	return 0;
	    case 20:			// 20msec frame length
	    	return 1;
	    case 40:			// 40msec frame length
	    	return 2;
	    case 60:			// 60msec frame length
	    	return 3;
	    default:
	    	return 1;
	}
}

// Read WAVE audio header and decode the header information
int read_WAVE_hdr(frame_t *frame, char *audio_file)
{
	// Open audio file for reading
	frame->fp = fopen(audio_file, "rb");
	if (frame->fp == NULL)
		return 0;

	// Determine type of audio file based on magic number
	// http://en.wikipedia.org/wiki/List_of_file_signatures
	char file_magic_no[4];
	char WAVE_magic_no1[] = {0x52,0x49,0x46,0x46};	// RIFF
	char WAVE_magic_no2[] = {0x57,0x41,0x56,0x45};	// WAVE

	// Copy the first 4 octects
	
	fread(file_magic_no, sizeof(uint8_t), 4, frame->fp);

	// WAVE file
	if(memcmp(file_magic_no, WAVE_magic_no1, 4) == 0)
	{
		// Copy the second (not used) and third 4 octects
		fread(file_magic_no, sizeof(uint8_t), 4, frame->fp);
		fread(file_magic_no, sizeof(uint8_t), 4, frame->fp);
		if(memcmp(file_magic_no, WAVE_magic_no2, 4) == 0)
		{
			WAVE_filehdr_t WAVE_hdr;

			// First seek the pointer to the begining
			rewind(frame->fp);

			// Copy the WAVE file header to a structure
			fread(&WAVE_hdr, sizeof(uint8_t), sizeof(WAVE_filehdr_t), frame->fp);

			// Copy the WAVE file header into frame content
			frame->sampleRate = WAVE_hdr.sampleRate;
			frame->frameSize = 1e-3*frameLen*frame->sampleRate;
			frame->channels = WAVE_hdr.channels;
			frame->bits_per_sample = WAVE_hdr.bits_per_sample;
			frame->bitRate = bitRate;
			frame->in = (short *)malloc(frame->frameSize*frame->channels*sizeof(short));
			frame->cbits = (unsigned char *)calloc(MAX_OPUS_PL_SIZE, sizeof(char));
		}
	}
	else
	{
		fprintf(stderr, "Unknown audio stream\n");
		return 0;
	}

	return 1;
}

/*M
  \emph{Initialize a RTCP sender report packet by filling common header fields.}
**/
void rtcp_senderRpt_pkt_init(rtcp_senderRpt_t *senderRpt, uint32_t ssrc)
{
	assert(senderRpt != NULL);
	senderRpt->b.v = 2;			// version 2
	senderRpt->b.p = 0;			// We use no padding (at first)
	senderRpt->b.RC = 0;			// We set reception report count to zero
	senderRpt->type = RTCP_SR;		// sender report
	senderRpt->length = ntohs(6);		// 28 byte report
	senderRpt->ssrc  = ntohl(ssrc);		// synchronization source
}

/*M
  \emph{Initialize a RTP packet by filling common header fields.}
**/
void rtp_rfc2250_pkt_init(rtp_pkt_t *pkt, uint32_t ssrc)
{
	assert(pkt != NULL);

	pkt->b.v = 2;		// version 2
	pkt->b.p = 0;		// We use no padding (at first)
	pkt->b.x = 0;		// No extensions (at first)
	pkt->b.cc = 0;		// No csrc identifiers at first
	pkt->b.m = 1;		// Mark the first packet
	pkt->b.pt = PT_OPUS;	// OPUS payload type
	pkt->seq = 0;		// Initialize sequence number to zero
	pkt->timestamp = 0;	// Initialize timestamp to zero
	pkt->ssrc  = ssrc;	// synchronization source
	pkt->plen = 0;		// Zero padding length
	pkt->hlen = RTP_HDR_SIZE;
}

/*M
  \emph{Fill the packet data by packing the header information.}
**/
void rtp_pkt_pack(rtp_pkt_t *pkt)
{
	assert(pkt != NULL);

	/* v: 2 bits, p: 1 bit, x: 1 bit, cc: 4 bits */
	pkt->data[0] =  (pkt->b.v  & 0x3) << 6;
	pkt->data[0] |= (pkt->b.p  & 0x1) << 5;
	pkt->data[0] |= (pkt->b.x  & 0x1) << 4;
	pkt->data[0] |= (pkt->b.cc & 0xf);
	/* m: 1 bit, pt: 7 bits */
	pkt->data[1] =  (pkt->b.m & 0x1) << 7;
	pkt->data[1] |= (pkt->b.pt & 0x7f);

	uint8_t *ptr = pkt->data + 2;
	/* seq: 16 bits */
	UINT16_PACK(ptr, pkt->seq);
	/* timestamp: 32 bits */
	UINT32_PACK(ptr, pkt->timestamp);
	/* ssrc: 32 bits */
	UINT32_PACK(ptr, pkt->ssrc);
}

/*M
  \emph{Initialize a RTCP sender report packet by filling common header fields.}
**/
void rtcp_endPrtp_pkt_init(rtcp_endPrtp_t *endPrtp, uint32_t ssrc)
{
	assert(endPrtp != NULL);
	endPrtp->b.v = 2;			// version 2
	endPrtp->b.p = 0;			// We use no padding (at first)
	endPrtp->b.RC = 1;			// We set source report count to one
	endPrtp->type = RTCP_BYE;		// end of participation report
	endPrtp->length = ntohs(1);		// 8 byte report
	endPrtp->ssrc  = ntohl(ssrc);		// synchronization source
}

/*M
  \emph{Create an UDP socket.}

  If the given destination address is a multicast adress, the socket will be set
  to use the multicast TTL ttl and sets the datagrams to loop back.
  Returns the filedescriptor of the socket, or -1 on error.
**/
int net_udp4_socket(struct sockaddr_in *saddr, uint16_t port, uint8_t ttl)
{
	assert(saddr != NULL);

	/*M
	Create UDP socket.
	**/
	int fd;
	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

	/*M
	Set socket to reuse addresses.
	**/
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		goto error;
	}

	static int allow = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&allow, sizeof(allow)) == -1)
	{
		perror("setsockopt");
		goto error;
	}

	saddr->sin_family = AF_INET;
	saddr->sin_port   = htons(port);

	/*M
	If the address is a multicast address, set the TTL and turn on
	multicast loop so the local host can receive the UDP packets.
	**/
	if (IN_MULTICAST(htonl(saddr->sin_addr.s_addr)))
	{
		uint8_t loop = 1;
		if ((setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) || (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0))
		{
			perror("setsockopt");
			goto error;
		}
	}

	return fd;

    error:
	if (close(fd) < 0)
		perror("close");
	return -1;
}

/*
 * Resolve an IP4 hostname.
 */
int net_ip4_resolve_hostname(const char *hostname, uint16_t port, uint8_t ip[4], struct sockaddr_in *saddr)
{
	assert(hostname != NULL);

	struct hostent *host;
	if ((host = gethostbyname(hostname)) == NULL)
	{
		perror("gethostbyname");
		return 0;
	}
	
	if (host->h_length != 4)
		return 0;
	
	if (ip != NULL)
		memcpy(ip, host->h_addr_list[0], host->h_length);
		
	if (saddr != NULL)
	{
		memcpy(&saddr->sin_addr, host->h_addr_list[0], host->h_length);
		saddr->sin_port = port;
	}

	return 1;
}

/*M
  \emph{Create a sending UDP socket.}

  Connects the created UDP socket to hostname. Returns the
  filedescriptor of the socket, or -1 on error.
**/
int net_udp4_send_socket(char *hostname, uint16_t port, uint8_t ttl)
{
	struct sockaddr_in saddr;
	if (!net_ip4_resolve_hostname(hostname, port, NULL, &saddr))
		return -1;

	/*M
	Create udp socket.
	**/
	int fd;
	if ((fd = net_udp4_socket(&saddr, port, ttl)) < 0)
		return -1;

	if (saddr.sin_addr.s_addr == INADDR_BROADCAST)
	{
		static int allow = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&allow, sizeof(allow)) == -1)
		{
			perror("setsockopt");
			goto error;
		}
	}

	/*M
	Connect to hostname.
	**/
	if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
	{
		perror("connect");
		goto error;
	}

	return fd;

	error:
	if (close(fd) < 0)
		perror("close");
	return -1;
}

/* Function to explode string to array */
char** explode(char delimiter, char* str)
{
	char *p = strtok (str, &delimiter);
	char ** res  = NULL;
	int n_spaces = 0;

	/* split string and append tokens to 'res' */
	while (p)
	{
		res = realloc (res, sizeof (char*) * ++n_spaces);
		if (res == NULL)
			exit (-1); /* memory allocation failed */

		res[n_spaces-1] = p;
		p = strtok (NULL, &delimiter);
	}

	/* realloc one extra element for the last NULL */
	res = realloc (res, sizeof (char*) * (n_spaces+1));
	res[n_spaces] = 0;

	return res;
}

// cleanup resources upon SIGINT and SIGTERM
static void sig_int(int signo)
{
	finished = 1;
}

/* Main loop */
int main(int argc, char *argv[])
{
	char 		*dstn_addr = NULL;
	uint16_t 	port     = 5004;
	uint8_t   	ttl      = 1;
	char 		*audio_file = NULL;
	char		*enc_par_path = "/tmp/enc_par";
	uint16_t 	RTCP_interval = 100;
	char		*lang_str, **lang_array, **tmp;
	uint32_t	lang_count = 1, i, j;

	//  Process the command line arguments
	int c;
	while ((c = getopt(argc, argv, "ha:p:t:f:l:r:P:i:L:")) >= 0)
	{
		switch (c)
		{
			case 'h':
				fprintf(stderr,"Description\n");
				fprintf(stderr,"-----------\n");
				fprintf(stderr,BOLD "rtp_opus_streamer" RESET " is an application used to stream an OPUS encoded raw WAVE file over a network using RTP encapsulation.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-a dstn_addr\t\tDestination IP address where RTP is sent to [eg. -a 235.5.5.5]\n");
				fprintf(stderr,"-p port\t\t\tRTP/RTCP port of the outgoing audio [eg. -p 5004]\n");
				fprintf(stderr,"-t ttl\t\t\tTime to Live value of RTP packets  [eg. -t 1]\n");
				fprintf(stderr,"-f audio_file\t\tPath to audio file or STDIN [eg. -f /path/to/audio/file or -f -]\n");
				fprintf(stderr,"-l frameLen\t\tFrame length in msec (i.e. 10, 20, 40, 60) [eg. -l 20]\n");
				fprintf(stderr,"-r bitRate\t\tEncoder output bitRate in bps. It ranges from 6000bps to 256000bps in multiples of 400bps [eg. -r 64000]\n");
				fprintf(stderr,"-P enc_par_path\t\tPath for the shared encoding parameter file [eg. -P /tmp/enc_par]\n");
				fprintf(stderr,"-i RTCP_interval\tInterval of RTP packet to send one RTCP sender report packet [eg. -i 100]\n");
				fprintf(stderr,"-L languages\t\tComma separated language Ids where audio is translated and streamed to (only for simulation) [e.g. -L 1,5]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./rtp_opus_streamer -a 235.5.5.5 -p 5004 -f sample.wav -P /tmp/enc_par -i 100 -L 1,5\n" RESET);
				fprintf(stderr,"Streams an RTP encapsulated audio file (sample.wav) using two languages (i.e. ID 1 & 5) to a multicast address 235.5.5.5 on port 5004.\n");
				fprintf(stderr,"The shared encoding parameter file, to change frameLen and bitRate dynamically, is located at /tmp/enc_par and RTCP sender reports are generated once every 100 RTP packets\n");
				goto exit;
			case 'a':
				dstn_addr = strdup(optarg);
				break;
			case 'p':
				port = (uint16_t)atoi(optarg);
				break;
			case 't':
				ttl = (uint16_t)atoi(optarg);
				break;
			case 'f':
				audio_file = strdup(optarg);
				break;
			case 'l':
				frameLen = (uint16_t)atoi(optarg);
				break;
			case 'r':
				bitRate = atoi(optarg);
				break;
			case 'P':
				enc_par_path = strdup(optarg);
				break;
			case 'i':
				RTCP_interval = (uint16_t)atoi(optarg);
				break;
			case 'L':
				lang_str = strdup(optarg);
				// Explode the language string into an array
				lang_array = explode(',', lang_str);
				// Count the number of languages
				tmp = lang_array+1;
				while (*tmp != NULL)
				{
					tmp++;
					lang_count++;
				}
				break;
			default:
				fprintf(stderr, "rtp_opus_streamer: missing operand. Type './rtp_opus_streamer -h' for more information.\n");
				goto exit;
		}
	}

	// House keeping input parameters
	if(dstn_addr == NULL || audio_file == NULL)
	{
		fprintf(stderr, "Either [destination address or audio file] is not specified. Type './rtp_opus_streamer -h' for more information\n");
		goto exit;
	}
	// Check frame length
	if(frameLen != 10 && frameLen != 20 && frameLen != 40 && frameLen != 60)
	{
		fprintf(stderr, "Frame length of %umsec is incorrect. Accepted values are [10, 20, 40 and 60] msec. Frame length is set to 20msec\n", frameLen);
		frameLen = 20;
	}
	// Check if bitRate is out of bound
	if(bitRate < MIN_bitRate || bitRate > MAX_bitRate)
	{
		fprintf(stderr, "Encoder bitrate %ubps is out of range. Accepted values range between %ubps and %ubps. Bitrate is set to 64000bps\n", bitRate, MIN_bitRate, MAX_bitRate);
		bitRate = 64000;
	}
	// Check if bitrate is on the correct multiples of 400bps starting from 6000bps
	else if(ceil((bitRate-6000)/400) != (double)(bitRate-6000)/400)
	{
		fprintf(stderr, "Encoder bitrate %ubps is not a multiple of 400bps starting from 6000bps. Bitrate is rounded to %ubps\n", bitRate, (uint32_t)(400*round((double)(bitRate-6000)/400)+6000));
		bitRate = (uint32_t)(400*round((double)(bitRate-6000)/400)+6000);
	}

	uint32_t		ssrc;
	langPkg_t		langPkg;
	uint16_t		lang_frame_len;
	frame_t			audio_frame;
	
	long 			wait_time;
	uint32_t 		sent_octets;

	char			command[64];
	
	int			rtp_sock, rtcp_sock;

	rtp_pkt_t 		rtp_pkt;
	rtcp_senderRpt_t	rtcp_senderRpt;
	rtcp_endPrtp_t		rtcp_endPrtp;

	struct timeval		tv;
	uint32_t		start_sec, start_usec;

	int			nbBytes, err;
	OpusEncoder		*encoder;

	// handle SIGTERM and SIGINT
	signal(SIGINT, sig_int);
	signal(SIGTERM, sig_int);

	// Initialize parameters
	wait_time = 0;
	sent_octets = 0;

	gettimeofday(&tv, NULL);
	srand (tv.tv_sec);
	ssrc = ((uint32_t)rand()) & 0xFFFFFFFF;

	// Get start time
	start_sec = tv.tv_sec;
	start_usec = tv.tv_usec;

	// Read audio stream from STDIN
	if(strcmp(audio_file, "-") == 0)
	{
		audio_frame.fp = stdin;
		audio_frame.sampleRate = 48000;
		audio_frame.frameSize = 1e-3*frameLen*audio_frame.sampleRate;
		audio_frame.channels = 1;
		audio_frame.bits_per_sample = 16;
		audio_frame.bitRate = bitRate;
		audio_frame.in = (short *)malloc(audio_frame.frameSize*audio_frame.channels*sizeof(short));
		audio_frame.cbits = (unsigned char *)calloc(MAX_OPUS_PL_SIZE, sizeof(char));
	}
	// Read audio stream from WAVE file
	else if (!read_WAVE_hdr(&audio_frame, audio_file))
	{
		fprintf(stderr, "Could not read WAVE file: %s\n", audio_file);
		goto exit;
	}

	// Initialize a shared encoding parameter (bitRate and frameSize)
	fp = fopen(enc_par_path, "w");
	fprintf(fp,"%u\n%u\n", bitRate, frameLen);
	fclose(fp);

	// Retrieve the encoding paramter input file timestamp
	sprintf(command, "stat -c %%Y %s", enc_par_path);
	fp = popen(command, "r");
	fgets(enc_par_ts, sizeof(enc_par_ts), fp);
	pclose(fp);

	/* Create a new encoder state */
	encoder = opus_encoder_create(audio_frame.sampleRate, audio_frame.channels, OPUS_APPLICATION_VOIP, &err);
	if (err<0)
	{
		fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
		goto exit;
	}

	// Set encoder related controls (i.e. bitRate, cbr, bit_per_sample)
	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(audio_frame.bitRate));		/* Configures the bitRate in the encoder */
	opus_encoder_ctl(encoder, OPUS_SET_VBR(0));					/* Disables variable bitRate (VBR) in the encoder. */
	opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(audio_frame.bits_per_sample));	/* Depth of signal being encoded */
	opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));				/* Highest encoder complexity */
	
	// Initialize RTCP and RTP packet structures
	rtcp_senderRpt_pkt_init(&rtcp_senderRpt, ssrc);
	rtp_rfc2250_pkt_init(&rtp_pkt, ssrc);
	rtcp_endPrtp_pkt_init(&rtcp_endPrtp, ssrc);

	// Open RTP and RTCP sockets
	rtp_sock  = net_udp4_send_socket(dstn_addr, port, ttl);
	rtcp_sock  = net_udp4_send_socket(dstn_addr, port+1, ttl);
	if (rtp_sock < 0 || rtcp_sock < 0)
	{
		fprintf(stderr, "Could not open socket\n");
		goto exit;
	}


	/*M
	  \emph{Simple RTP audio streaming server main loop.}

	  The main loop opens the Audio file \verb|audio_file|, reads each frame
	  into an rtp packet and sends it out using the UDP socket
	  \verb|sock|. After sending a packet, the mainloop sleeps for the duration
	  of the packet, synchronizing  itself when the sleep is not accurate
	  enough. If the sleep desynchronizes itself from the stream more
	  than \verb|MAX_WAIT_TIME|, the synchronization is reset.
	**/
	while (!finished)
	{
		// Retrieve the timestamp of the encoding parameters file
		sprintf(command, "stat -c %%Y %s", enc_par_path);
		fp = popen(command, "r");
		fgets(temp_ts, sizeof(temp_ts), fp);
		pclose(fp);
		// Retrieve the new values if the encoding parameters file is modified
		if(strcmp(enc_par_ts, temp_ts) != 0)
		{
			fp = fopen(enc_par_path, "r");
			fscanf(fp,"%u\n%u\n", (uint32_t *)(&bitRate), (uint32_t *)(&frameLen));
			fclose(fp);

			// Does the encoding parameters contain acceptable values?
			// Check frame length
			if(frameLen != 10 && frameLen != 20 && frameLen != 40 && frameLen != 60)
			{
				fprintf(stderr, "Frame length of %umsec is incorrect. Accepted values are [10, 20, 40 and 60] msec. Frame length is set to 20msec\n", frameLen);
				frameLen = 20;
			}
			// Check if bitRate is out of bound
			if(bitRate < MIN_bitRate || bitRate > MAX_bitRate)
			{
				fprintf(stderr, "Encoder bitrate %ubps is out of range. Accepted values range between %ubps and %ubps. Bitrate is set to 64000bps\n", bitRate, MIN_bitRate, MAX_bitRate);
				bitRate = 64000;
			}
			// Check if bitrate is on the correct multiples of 400bps starting from 6000bps
			else if(ceil((bitRate-6000)/400) != (double)(bitRate-6000)/400)
			{
				fprintf(stderr, "Encoder bitrate %ubps is not a multiple of 400bps starting from 6000bps. Bitrate is rounded to %ubps\n", bitRate, (uint32_t)(400*round((double)(bitRate-6000)/400)+6000));
				bitRate = (uint32_t)(400*round((double)(bitRate-6000)/400)+6000);
			}

			// If the encoding parameters result in an audio packet within limit
			if(1e-3*frameLen*bitRate < 8*MAX_OPUS_PL_SIZE )
			{
				// Modify encoder frameSize
				audio_frame.frameSize = 1e-3*frameLen*audio_frame.sampleRate;
				// Reallocate the encoder's input audio buffer size
				audio_frame.in = (short*)realloc(audio_frame.in, audio_frame.frameSize*audio_frame.channels*sizeof(short));

				// Modify encoder bitRate
				opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitRate));

				fprintf(stderr, "Audio frame length = %umsec\tEncoder bitrate = %ubps\n", frameLen, bitRate);
			}
			// If it exceeds the limit, we either have to modify the bitrate or the frameLen. Here we modify the bitrate
			else
			{
				// Modify encoder frameSize
				audio_frame.frameSize = 1e-3*frameLen*audio_frame.sampleRate;
				// Reallocate the encoder's input audio buffer size
				audio_frame.in = (short*)realloc(audio_frame.in, audio_frame.frameSize*audio_frame.channels*sizeof(short));

				// Modify the bitRate and keep it on the correct 400bps multiples
				bitRate = 1e3*8*MAX_OPUS_PL_SIZE/frameLen;
				bitRate = (uint32_t)(400*round((double)(bitRate-6000)/400)+6000);
				// Modify encoder bitRate
				opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitRate));

				fprintf(stderr, "Audio frame length = %umsec\tEncoder bitrate = %ubps\n", frameLen, bitRate);

			}

			// Update encoding parameters timestamp for future use
			strcpy(enc_par_ts, temp_ts);
		}

		// Retrieve the audio data
		fread(audio_frame.in, sizeof(short)*audio_frame.channels, audio_frame.frameSize, audio_frame.fp);
		// When the audio file reaches the end, reset it back to the beginning
		if (feof(audio_frame.fp))
		{
			// seek the pointer to the begining of the data
			fseek(audio_frame.fp, sizeof(WAVE_filehdr_t), SEEK_SET);

			// Copy the WAVE file header to a structure
			fread(audio_frame.in, sizeof(short)*audio_frame.channels, audio_frame.frameSize, audio_frame.fp);
		}

		// Encode the frame
		nbBytes = opus_encode(encoder, audio_frame.in, audio_frame.frameSize, audio_frame.cbits, MAX_OPUS_PL_SIZE);
		if (nbBytes<0)
		{
			fprintf(stderr, "encode failed: %s\n", opus_strerror(nbBytes));
			return EXIT_FAILURE;
		}

		// Send RTCP packets once every #{RTCP_interval}
		if(sent_packets % RTCP_interval == 0)
		{
			/* Fill rtcp sender per report parameters */
			gettimeofday(&tv, NULL);
			rtcp_senderRpt.ts_tx_NTP.second = ntohl(tv.tv_sec + 2208988800LL);	// NTP time second part
			rtcp_senderRpt.ts_tx_NTP.fraction = ntohl(tv.tv_usec * 4294.967296);	// NTP time fraction part

			rtcp_senderRpt.ts_tx_RTP = ntohl(rtp_pkt.timestamp);			// RTP timestamp
			rtcp_senderRpt.sdr_pkt_count = ntohl(sent_packets);			// sender packet count
			rtcp_senderRpt.sdr_oct_count  = ntohl(sent_octets);			// sender octet count

			/* send rtcp sender report */
			SEND_PACKET(rtcp_sock, &rtcp_senderRpt, sizeof(rtcp_senderRpt_t), 0);
		}		
		// Update parameters to be used in RTCP packet
		sent_octets += nbBytes;
		sent_packets++;

		// If more than one language is to be streamed, include a language header to each language frame
		if(lang_count > 1)
			lang_frame_len = nbBytes + sizeof(langPkg_t);
		// Else, no need to include a language header
		else
			lang_frame_len = nbBytes;

		// Streaming multiple RTP languages (only for simulation purpose).
		// Inner loop tries to fit all requested languages in one RTP packet. If this is not the case,
		// outer loop adds the remaining language streams into consequetive RTP packets.
		// All language streams are preceeded with a header where it helps the recievers to select their prefered language.

		i=0;	// i counts the number of RTP packets send
		while(1)
		{	// j counts the number of languages packed in one RTP packet
			for(j=0; j<(MAX_RTP_PL_SIZE/lang_frame_len) && (i*(MAX_RTP_PL_SIZE/lang_frame_len)+j)<lang_count; j++)
			{
				if(lang_count > 1)
				{
					// Populate language header structure
					langPkg.magic_no = htonl(0x6564656e);
					langPkg.lang_ID = htons(atoi(lang_array[i*(MAX_RTP_PL_SIZE/lang_frame_len) + j]));
					langPkg.PL_len = htons(lang_frame_len);
					// Copy language header to RTP packet
					memcpy(rtp_pkt.data + rtp_pkt.hlen + j*lang_frame_len, &langPkg, sizeof(langPkg_t));
					// Copy audio to RTP packet
					memcpy(rtp_pkt.data + rtp_pkt.hlen + j*lang_frame_len + sizeof(langPkg_t), audio_frame.cbits, lang_frame_len - sizeof(langPkg_t));
				}
				else
				{
					// Copy audio to RTP packet
					memcpy(rtp_pkt.data + rtp_pkt.hlen, audio_frame.cbits, lang_frame_len);
				}
			}

			// pack the rtp header
			rtp_pkt_pack(&rtp_pkt);

			// send rtp packet
			SEND_PACKET(rtp_sock, rtp_pkt.data, (rtp_pkt.hlen + j*lang_frame_len), 0);

			if (i*(MAX_RTP_PL_SIZE/lang_frame_len) + j < lang_count)
				i++;
			else
				break;
		}
		// Update RTP sampling instance and sequence number
		rtp_pkt.timestamp += audio_frame.frameSize * (48000.0/audio_frame.sampleRate);
		rtp_pkt.seq++;

		// Adjust frame timing
		// Increment the audio Timestamp
		wait_time += 1e3*frameLen;
		// Sender synchronisation \verb|sleep| until the next frame has to be sent
		if (wait_time > 0)
			usleep(wait_time);
		// Get length of iteration
		gettimeofday(&tv, NULL);
		uint32_t len = 1e6*(tv.tv_sec - start_sec) + (tv.tv_usec - start_usec);
		// Adjust the waiting time
		wait_time -= len;
		if (abs(wait_time) > MAX_WAIT_TIME)
			wait_time = 0;
		// Reorganize starting time
		start_sec = tv.tv_sec;
		start_usec = tv.tv_usec;
	}

	/* send rtcp end of participation report */
	SEND_PACKET(rtcp_sock, &rtcp_endPrtp, sizeof(rtcp_endPrtp_t), 0);

	/*M
	  Close the audio file.
	**/
	if (fclose(audio_frame.fp) != 0)
		fprintf(stderr, "Could not close audio file\n");
	
    exit:
	if (dstn_addr != NULL)
		free(dstn_addr);
	if (audio_file != NULL)
		free(audio_file);

	return 0;
}


/*
 * RTP Payload types
 * Table B.2 / H.225.0
 * Also RFC 3551, and
 *
 *	http://www.iana.org/assignments/rtp-parameters
 *	https://github.com/boundary/wireshark/blob/master/epan/rtp_pt.h	
 */

#define PT_OPUS		120	/* RFC6716 */
#define PT_SPEEX	110

#define PACKED		__attribute__((packed))

/* RTCP Payload types */
#define RTCP_SR     200   /*  sender report        */
#define RTCP_RR     201   /*  receiver report      */
#define RTCP_SDES   202   /*  source description   */
#define RTCP_BYE    203   /*  good bye             */
#define RTCP_APP    204   /*  application defined  */

/* NTP time stamp structure */
struct ntp_time_t
{
	uint32_t second;
	uint32_t fraction;
};

/** Generic UDP header structure */
typedef struct udp_hdr {
	uint16_t source;		/**< Source port */
	uint16_t dest;			/**< Destination port */
	uint16_t len;			/**< Length */
	uint16_t check;			/**< Checksum */
} PACKED udp_hdr_t;

/* RTP data structure */
struct RECORD_t
{
	uint32_t stream_id;					// stream identifier

	char	 src_mac[18];					// source MAC address
	char	 src_IP[16];					// source IP address

	uint8_t	 channels;					// mono, sterio, ... data stored in the stream

	uint16_t previous_seq;					// previous sequence
	double	 previous_ts_rx_real;				// previous recieve timestamp in real time (seconds)
	uint32_t previous_ts_tx;				// previous transmit timestamp in relative time (samples)
	double	 previous_jitter;				// previous jitter
	double	 prev_frame_duration;				// Previous frame duration
	
	uint32_t sample_rate;
	double	 buffered_time;					// Audio sampling rate
	double	 frame_duration_sum;				// summation of all frame durations
	double	 frame_PL_sum;					// summation of all frame payloads

	double	 ts_tx_first_real;				// Timestamp in real time (i.e. seconds) for first RTP packet
	uint32_t ts_tx_first;					// Timestamp in relative time (i.e. samples) for first RTP packet 

	uint32_t totalRTPcount;					// total RTP packet counter
	uint32_t RTPcount;					// RTP packet counter

	double	 packetLoss;					// lost packet counter
	double	 jitterSum;					// sum of jitter values
	double	 latencySum;					// sum of latency values

	UT_hash_handle	hh;					// hash function handler
};

/* RTP language header structure */
typedef struct langhdr_s
{
	uint32_t magic_no;					// Magic number for start of a language header
	uint16_t lang_ID;					// Language type
	uint16_t data_len;					// Audio data length
} langhdr_t;

/*C
  \emph{RTP header structure.}

  \verbatim{
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |V=2|P|X|  CC   |M|     PT      |       sequence number         | 
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           timestamp                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           synchronization source (SSRC) identifier            |
  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  |            contributing source (CSRC) identifiers             |
  |                             ....                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 }
**/

/* RTP header structure */
typedef struct rtp_hdr
{
	uint8_t cc:4;						// CSRC count
	uint8_t x:1;						// header extension flag
	uint8_t p:1;						// padding flag
	uint8_t v:2;						// protocol version
	uint8_t pt:7;						// payload type
	uint8_t m:1;						// marker bit

	uint16_t seq;						// sequence number
	uint32_t ts_tx;						// timestamp
	uint32_t ssrc;						// synchronization source identifier
//	uint32_t csrc[1];					// optional CSRC list
} PACKED rtp_hdr_t;


/*C
  \emph{RTCP sender report header structure.}

  \verbatim{  
  
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |   PT=SR=200   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         SSRC of sender                        |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |              NTP timestamp, most significant word             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             NTP timestamp, least significant word             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         RTP timestamp                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     sender's packet count                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      sender's octet count                     |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 }
**/

/* RTCP sender report header structure */
typedef struct rtcp_senderRpt_hdr
{
	uint8_t RC:5;						// reception report count
	uint8_t p:1;						// padding flag
	uint8_t v:2;						// protocol version

	uint8_t type;						// packet type
	uint16_t length;					// length in byte
	uint32_t ssrc;						// synchronization source identifier
	struct ntp_time_t ts_tx_NTP;				// current NTP timestamp
	uint32_t ts_tx_RTP;					// First RTP timestamp
	uint32_t sdr_pkt_count;					// Sender packet count
	uint32_t sdr_oct_count;					// Sender octet count
} PACKED rtcp_senderRpt_hdr_t;

/*C
  \emph{RTCP end of participation header structure.}

  \verbatim{  
  
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |   PT=SR=200   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         SSRC of sender                        |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 }
**/

/* RTCP end of participation header structure */
typedef struct rtcp_endPrtp_hdr
{
	uint8_t RC:5;						// reception report count
	uint8_t p:1;						// padding flag
	uint8_t v:2;						// protocol version

	uint8_t type;						// packet type
	uint16_t length;					// length in byte
	uint32_t ssrc;						// synchronization source identifier
} PACKED rtcp_endPrtp_hdr_t;



/*C
  \emph{MP3 tag structure.}

  \verbatim{
   0                   1                    2                    3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5  6 7 8 9 0 1 2  3  4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+--+--+-+-+-+-+-+-+-+-+
  |	     Sync   	| V | L |Pt|   BR  | SR|Pa|Pr| CM| ME|C|O|Emp|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+--+--+-+-+-+-+-+-+-+-+
 }
**/

/* MP3 tag structure */
typedef struct MP3_tag
{
	uint32_t Emp:2;						// Emphasis
	uint32_t O:1;						// Original
	uint32_t C:1;						// Copyright
	uint32_t ME:2;						// Mode extention
	uint32_t CM:2;						// Channel mode
	uint32_t Pr:1;						// Private bit
	uint32_t Pa:1;						// Padding bit
	uint32_t SR:2;						// Sampling rate frequency index
	uint32_t BR:4;						// Bitrate index
	uint32_t Pt:1;						// Protection bit
	uint32_t L:2;						// MPEG layer description
	uint32_t V:2;						// MPEG audio version
	uint32_t Sync:11;					// Frame sync
} PACKED MP3_tag_t;


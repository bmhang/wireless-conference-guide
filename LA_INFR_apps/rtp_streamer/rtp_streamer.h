

#define MAX_FILENAME 256

/*M
  \emph{Maximal synchronization latency for sending packets.}

  In usecs.
**/
#define MAX_WAIT_TIME (1 * 1000 * 1000)

/*M
 \emph{Get lower bits of byte.}

 Get the b lower bits of byte x.
**/
#define LOWERBITS(x, b) ((b) ? (x) & ((2 << ((b) - 1)) - 1) : 0)

/*M
 \emph{Get higher bits of byte.}

 Get the b higher bits of byte x.
**/
#define HIGHERBITS(x, b) (((x) & 0xff) >> (8 - (b)))

/*M
  \emph{16 bits value packing macro.}

  This macro advances the buffer pointer it is given as first
  argument, and fills the buffer with the big endian packed value
  given as second argument.
**/
#define UINT16_PACK(ptr, i)					\
{								\
	*(ptr++) = (uint8_t)(((i) >> 8) & 0xFF);		\
	*(ptr++) = (uint8_t)((i)        & 0xFF);		\
}

/*M
  \emph{32 bits value packing macro.}

  This macro advances the buffer pointer it is given as first
  argument, and fills the buffer with the big endian packed value
  given as second argument.
**/
#define UINT32_PACK(ptr, i)					\
{								\
	*(ptr++) = (uint8_t)(((i) >> 24) & 0xFF);		\
	*(ptr++) = (uint8_t)(((i) >> 16) & 0xFF);		\
	*(ptr++) = (uint8_t)(((i) >> 8)  & 0xFF);		\
	*(ptr++) = (uint8_t)((i)         & 0xFF);		\
}

#define EEOF (-1)
#define ESYNC (-2)

/*M
  \emph{Maximum RTP packet size.}
**/
#define MAX_RTP_SIZE 1472

/*M
  \emph{RTP header size.}
**/
#define RTP_HDR_SIZE 12

/*M
  \emph{Maximum RTP payload size.}
**/
#define MAX_RTP_PL_SIZE (MAX_RTP_SIZE - RTP_HDR_SIZE)

/*
 * RTP Payload types
 * Table B.2 / H.225.0
 * Also RFC 3551, and
 *
 *	http://www.iana.org/assignments/rtp-parameters
 *	https://github.com/boundary/wireshark/blob/master/epan/rtp_pt.h	
 */
#define PT_SPEEX	110
#define PT_OPUS		120	/* RFC6716 */

/* RTCP Payload types */
#define RTCP_SR     200   /*  sender report        */
#define RTCP_RR     201   /*  receiver report      */
#define RTCP_SDES   202   /*  source description   */
#define RTCP_BYE    203   /*  good bye             */
#define RTCP_APP    204   /*  application defined  */


/* General audio frame structure */
typedef struct frame_s
{
	FILE		*fp;				/* Frame pointer */
	uint16_t	sampleRate;			/* Sampling rate in sps */
	uint16_t	frameSize;			/* Frame size in samples */
	uint8_t		channels;			/* Audio channels */
	uint16_t	bits_per_sample;		/* bits per sample */
	uint32_t	bitRate;			/* Audio output bitRate */
	short	 	*in;				/* Encoder input audio buffer */
	unsigned char	*cbits;				/* Encoder output audio buffer */
} frame_t;


/* RTP language header structure */
typedef struct langPkg_s
{
	uint32_t magic_no;			/* Magic number for start of a language header */
	uint16_t lang_ID;			/* Language type */
	uint16_t PL_len;			/* Audio data length */
} langPkg_t;

/*
  RTP header structure

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

*/

// Structure representing the first 32 bits of an RTP header packet
typedef struct rtp_bits_s
{
	uint8_t v   :2;		/* version */
	uint8_t p   :1;		/* padding */
	uint8_t x   :1;		/* number of extension headers */
	uint8_t cc  :4;		/* number of CSRC identifiers */
	uint8_t m   :1;		/* marker */
	uint8_t pt  :7;		/* payload type */
} rtp_bits_t;

/*M
  \emph{Structure representing a RTP packet.}

  The \verb|data| buffer is not the payload buffer, it contains the
  complete data for the packet. Be careful to update the payload
  pointer correctly when adding additional headers.
**/
typedef struct rtp_pkt_s
{
	rtp_bits_t b;					/* first 32 bits */
	uint16_t   seq;					/* sequence number */
	uint32_t   timestamp;				/* timestamp */
	uint32_t   ssrc;				/* synchronization source */
	uint8_t    plen;				/* padding length */
	uint16_t   hlen;				/* header length */
	uint8_t    data[MAX_RTP_SIZE];			/* packet data */
} rtp_pkt_t;

/* NTP time stamp structure */
struct ntp_time_t
{
	uint32_t second;
	uint32_t fraction;
};

/*
  RTCP sender report header structure
  
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
*/

// Structure representing the first 24 bits of an RTCP header packet
typedef struct rtcp_bits_s {
	uint8_t RC:5;				// reception report count
	uint8_t p:1;				// padding flag
	uint8_t v:2;				// protocol version
} rtcp_bits_t;

/* RTCP sender report header structure */
typedef struct rtcp_senderRpt
{
	rtcp_bits_t b;				// first 8 bits
	uint8_t  type;				// packet type
	uint16_t length;			// length in byte
	uint32_t ssrc;				// synchronization source identifier
	struct   ntp_time_t ts_tx_NTP;		// current NTP timestamp
	uint32_t ts_tx_RTP;			// First RTP timestamp
	uint32_t sdr_pkt_count;			// Sender packet count
	uint32_t sdr_oct_count;			// Sender octet count
} rtcp_senderRpt_t;

/*
  RTCP end of participation header structure

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    RC   |   PT=SR=203   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         SSRC of sender                        |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*/

// RTCP end of participation header structure
typedef struct rtcp_endPrtp
{
	rtcp_bits_t b;				// first 8 bits
	uint8_t     type;			// packet type
	uint16_t    length;			// length in byte
	uint32_t    ssrc;			// synchronization source identifier
} rtcp_endPrtp_t;



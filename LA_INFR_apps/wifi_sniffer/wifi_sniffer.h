/* enable/disable flags */
enum flag_t
{
	DISABLED = 0,
	ENABLED = 1
};

/* Wi-Fi frame type and subtype enumeration */
// Frame type emumeration
enum frame_type_t
{
	MNGT = 0,
	CTRL,
	DATA
};

union frame_subtype_t
{
	// Management frame sub-type emumeration
	enum
	{	ASSN_REQ = 0,
		ASSN_RESP,
		RE_ASSN_REQ,
		RE_ASSN_RESP,
		PRB_REQ,
		PRB_RESP,
		BEACON = 8,
		ATIM,
		DE_ASSN,
		AUTHN,
		DE_AUTHN,
		ACTION
	}mngt;	
	// Control frame sub-type emumeration
	enum
	{
		BLK_ACK_REQ = 8,
		BLK_ACK,
		PS_POLL,
		RTS,
		CTS,
		ACK,
		CF_END,
		CF_END_CF_ACK
	}ctrl;
	// Data frame sub-type emumeration
	enum
	{
		DATA_RAW = 0,
		DATA_CF_ACK,
		DATA_CF_POLL,
		DATA_CF_ACK_CF_POLL,
		NUL,
		CF_ACK,
		CF_POLL,
		CF_ACK_CF_POLL,
		QOS_DATA,
		QOS_DATA_CF_ACK,
		QOS_DATA_CF_POLL,
		QOS_DATA_CF_ACK_CF_POLL,
		QOS_NUL,
		QOS_CF_POLL_NO_DATA = 14,
		QOS_CF_ACK_NO_DATA
	}data;
	uint8_t val;
};


/*C
  \emph{RadioTap header structure}

  \verbatim{
   0                   1                   2                   3                   4                   5                   6
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  it_version	  |     it_pad	  |		it_len		  |                           it_present                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           					TSFT								  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |	 FLAGS	  |	 RATE	  |                           CHANNEL                             |		FHSS		  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | DBM_ANTSIGNAL | DBM_ANTNOISE  |	     LOCK_QUALITY	  |	   TX_ATTENUATION	  |	  DB_TX_ATTENUATION	  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | DBM_TX_POWER  |	ANTENNA	  |  DB_ANTSIGNAL |  DB_ANTNOISE  |		RX_FLAGS	  |		TX_FLAGS	  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  RTS_RETRIES  |  DATA_RETRIES |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 }
**/

/* ++++++++ libtrace.h extenstion for radio tap fields +++++++ */
#define TRACE_RADIOTAP_MCS 19
#define TRACE_RADIOTAP_AMPDU_STATUS 20

/* Radiotap channel type structure */
struct channel_type_t
{
	uint16_t reserved:4;		// reserved
	uint16_t turbo:1;		// Turbo
	uint16_t cck:1;			// Complementary Code Keying
	uint16_t ofdm:1;		// Orthogonal Frequency Division Multiplexing
	uint16_t spec_2Ghz:1;		// 2.4 GHz spectrum
	uint16_t spec_5Ghz:1;		// 5 GHz spectrum
	uint16_t passive:1;		// passive
	uint16_t dym_cck_ofdm:1;	// dynamic CCK-OFDM
	uint16_t gfsk:1;		// Gaussian Frequency Shift Keying
	uint16_t gsm:1;			// Global System of Mobile communication
	uint16_t static_turbo:1;	// static Turbo
	uint16_t half_rate:1;		// Half Rate Channel
	uint16_t quarter_rate:1;	// Quarter Rate Channel
};

/* Radiotap MCS information structure */
struct MCS_INFO_t
{
	
	uint8_t known;			// Known MCS Information
	uint8_t bandwidth:2;		// Bandwidth
	uint8_t guard:1;		// Guard Interval
	uint8_t reserved:5;		// Reserved
	uint8_t index;			// MCS index
};

/*C
  \emph{HR-DSSS PHY frame structure}
  
   In the IEEE Std 802.11b-1999 amendment, an additional new physical layer radio technology for the ISM band (2.4 GHz) was defined, based upon the existing DSSS PHY: the HR-DSSS PHY (High Rate Direct sequence spread spectrum). Its main new feature is the addition of two higher data rates, 5.5 Mbit/s and 11 Mbit/s, by using the CCK (Complementary Code Keying) coding to obtain this.
   
   A complementary code contains a pair of finite bit sequences of equal length, such that the number of pairs of identical elements (1 or 0) with any given separation in one sequence is equal to the number of pairs of unlike elements having the same separation in the other sequence. A network using CCK can transfer more data per unit time for a given signal bandwidth than a network using the Barker code, because CCK makes more efficient use of the bit sequences.
   
   The CCK modulation used by 802.11b transmits data in symbols of eight chips (instead of 11 chips for the Barker code for 1 Mbit/s and 2 Mbit/s), where each chip is a complex QPSK bit-pair at a chip rate of 11Mchip/s. In 5.5 Mbit/s and 11 Mbit/s modes respectively 4 and 8 bits are modulated onto the eight chips of the symbol. This indeed leads to 11 Mchip/s * (4bit / 8 chips) = 5.5 Mbit/s and 11 Mchip/s * (8 bit / 8 chips) = 11 Mbit/s

  \verbatim{
  |<------------ 56/128 bits ---------->|<------ 16 bits ---->|<- 8 bits ->|<- 8 bits ->|<------ 16 bits ---->|<---- 16 bits ---->|<------------- variable ------------>|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |	      Long/short sync   	|	   SFD        |   signal   |   service  |	length        |	      HEC         |		    PSDU		|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   ╀							         ╀								   ╀					╀
  ┌───────────────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────┬─────────────────────────────────────┐
  │ 			PLCP preamble 			      │	       				PLCP header			  |	 	   payload		|
  └───────────────────────────────────────────────────────────┴───────────────────────────────────────────────────────────────────┴─────────────────────────────────────┘
		72/144 us at 1 Mbits/s DBPSK				[24 us at 2 Mbits/s DBPSK]/[48 us at 1 Mbits/s DBPSK]	     	      1,2,5.5,11 Mbits/s
 }
 
  The rate is encoded in the signal field in multiples of 100 kbit/s. Thus, 0x0A represents 1 Mbit/s and 0x14 is used for 2 Mbit/s (same as for DSSS PHY). Now 0x37 is additionally used for 5.5 Mbit/s and 0x6E for 11 Mbit/s.
  
  IEEE Std 802.11b-1999 adds an optional short PLCP PDU format to obtain higher throughput at the higher layers, due to less PHY overhead. The short synchronization field consists of 56 scrambled zeros instead of scrambled ones. The short start frame delimiter SFD consists of a mirrored bit pattern compared to the SFD of the long format: 0000 0101 1100 1111 is used for the short PLCP PDU instead of 1111 0011 1010 0000 for the long PLCP PPDU. Receivers that are unable to receive the short format (i.e. when they did not incorporate the 802.11b amendment) will not detect the start of a frame (but will sense the
medium is busy). Only the preamble is transmitted at 1 Mbit/s, DBPSK. The following header is already transmitted at 2 Mbit/s, DQPSK, which is also the lowest available data rate for the payload.

**/


/*C
  \emph{OFDM PHY frame structure}
  
  In the IEEE Std 802.11a-1999 and IEEE Std 802.11g-2003 amendment, a new physical layer radio technology for the 2.4/5 GHz band was defined: the OFDM PHY (Orthogonal Frequency Division Multiplexing). The OFDM system provides a wireless LAN with data payload communication capabilities of 6, 9, 12, 18, 24, 36, 48, and 54 Mbit/s. The system uses 52 subcarriers that are modulated using binary or quadrature phase shift keying (BPSK/QPSK), 16-quadrature amplitude modulation (QAM), or 64-QAM. Forward error correction coding (convolutional coding) is used with a coding rate of 1/2, 2/3, or 3/4.

  Per channel, the system uses 52 subcarriers and adds 12 virtual subcarriers which are used as guard spacing between different channels. 26 subcarriers are to the left of the center frequency and 26 are to the right. This makes a ‘virtual’ total of 64 subcarriers, which allows for an implementation of OFDM with the help of FFT (Fast Fourier Transform). Out of the 52 actual subcarriers, 4 are used for pilot signals to make the signal detection robust against frequency offsets. The remaining 48 subcarriers are used to transmit data.
  
  The basic idea of OFDM was the reduction of the symbol rate by distributing bits over numerous subcarriers. IEEE 802.11a/g uses a fixed symbol rate of 250 000 symbols per second independent of the data rate (0.8 μs guard interval for Inter Symbol Interference (ISI) mitigation plus 3.2 μs used for data results in a symbol duration of 4 μs).
  
  Example BPSK 1/2: BPSK for modulation delivers 1 coded bit per carrier. For all 48 data subcarriers this totals to 48 coded bits per OFDM symbol, which gives 24 data bits with 1/2 coding. These 24 data bits per OFDM symbol then result in: 24 bits/OFDM-symbol x 250 000 OFDM-symbols/s = 600 x 104 bits/s = 6 x 106 bits/s = 6 Mbit/s data rate.

  \verbatim{
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9|<-------- variable ------->|<--- 6 --->|<-- variable ->|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ . . . +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | rate  |r|	     length	  |p|	tail	|		service		  |	       PSDU	      |	  tail	  |	pad	  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ . . . +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   ┆						┆							         				   ┆
  └╶╶╶╶╶╶╶╶╶╶╶╶╶ ┐				┆							         				   ┆
  	16 us	  ╀	          6 Mbits/s	╀			     6,9,12,18,24,36,48,54 Mbits/s			   	  ╀
  ┌───────────────┬─────────────────────────────┬─────────────────────────────────────────────────────────────────────────────────────────┐
  │ PLCP preamble │	       signal		|					payload		      				  |
  └───────────────┴─────────────────────────────┴─────────────────────────────────────────────────────────────────────────────────────────┘
     12 symbols		      1 symbol						   variable symbols
 }
 
  Due to the nature of OFDM, the PDU on the physical layer of IEEE 802.11a/g (OFDM PHY) looks quite different from 802.11b (HR-DSSS PHY) or the original 802.11 (FHSS PHY and DSSS PHY) physical layers.
  
  The PLCP preamble consists of 12 symbols and is used for frequency acquisition, channel estimation, and synchronization. The duration of the preamble is 16 μs. (While a normal symbol lasts 4 us, the preamble contains 10 short symbols + 2 normal symbols, explaining the duration of only 16 μs).
  
  The following OFDM symbol, called signal, contains the following fields and is BPSK-modulated. The 4 bit rate field determines the data rate and the modulation of the rest of the packet (examples are 0x3 for 54 Mbit/s, 0x9 for 24 Mbit/s, or 0xF for 9 Mbit/s). The length field indicates the number of bytes in the payload field. The parity bit shall be an even parity for the first 17 bits of the signal field (rate, length and the reserved bit). Finally, the six tail bits are set to zero.
  
  The data field is sent with the rate determined in the rate field and contains a service field which is used to synchronize the descrambler of the receiver (the data stream is scrambled using the polynomial x7 + x4 + 1) and which contains bits for future use. The the PSDU, PLCP Service Data Unit, contains the MAC frame (1-4095 byte). The tail bits are used to reset the encoder. Finally, the pad field ensures that the number of bits in the PDU maps to an integer number of OFDM symbols.

**/

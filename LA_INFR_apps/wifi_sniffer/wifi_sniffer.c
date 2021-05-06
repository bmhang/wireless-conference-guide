/* This program retrieves 802.11 PHY and MAC layer parameters and store in into database
How does it Work
----------------
Researchers in the field of wireless networking are often interested to collect and analyze Physical and MAC layer parameters of Wi-Fi packets. Physical layer parameters include but not limited to preamble types, tranmission rate, frequency, 802.11 a/b/g/n protocol types, recieved signal strength and modulation types. And MAC layer parameters also include but not limited to MAC address used, sequence number, Frame Check Sequence and framelength.

MAC layer parameters are included in the transmitted packet whereas Physical layer parameters are generated at the recieve end and it depends on the dirver support which is made available to the end user. If it is supported, these Physical layer parameters are injected into the recieved frame and brought as one whole packet to the application.

The other support we get from wireless drivers is the ability to work in monitor mode. In this mode, an interface can intercept packet coming from all nodes with in a single Wireless Domain System (WDS). More to that, in order to capture everything on the air, both with in WDS and external to WDS, we need to make the interface in promiscous mode. This mode by default encompases the functions of a monitor mode interface and therefore it suffice to create a promiscous mode interface in order to sniff all packets on the air.

Once the wireless interface is made in promiscous and monitor mode and RadioTap header injection is supported, we start capturing packets one by one, extract the Physical and MAC layer parameters from the packet and store it into database.

For packet capturing, we use the libtrace frame capturing library. Libtrace 

The main goal of making this program is for interference estimation. In a wireless experiment, interference is a major bottleneck that corrupts experiment outputs. One approach to mitigate this problem is by estimating the interference and telling whether the experiment is valid or not. This can be achieved by calculating the Channel Occupation Ratio (COR) of an interference source. During an experiment, we capture all interfering packets in the surrounding, decode Physical and MAC layer parameters, calculate aggregate COR metric.

Libtrace packet library supports the Berkeley Packet Filter (BPF) and appropraite parameters can be passed to filter the incoming packets. One good application of BPF filters is to separate packets coming from the System Under Test and other interferences.

At the data collection or database end, each entrie has a unique Frame Check Sequence. There is a limited chance that two consequetive packets on the air will have identical Frame Check Sequences. However the case, there are special cases where this happens. Sometimes, senders send multiple copy of identical packets on the air, or during retransmission of packets where the exact replica has to be transmitted. Certain control packets like ACK, CTS and RTS can be identical at different moments in time. In all these cases we need to check replicated packets and handle it appropraitely. The handler used in this program generate a deterministic random Frame Check Sequence number based on original Frame Check Sequence and store it into database. Hash tables and functions are used to speed up the searching process.


The other short comming of Wi-Fi cards is the limitation of decoding packets which went through a reciever end with out a problem. A sniffer sitting in between the transmitter and the reciever is supposed to capture all packets that went through the reciever. However, this is not the case and typical reasons for such limitation are monitor overloading, poor sniffer reception, packet drops at kernel, antenna pollarization difference, and others. In order to tackle this problem . . .

Moreover, we have increased performance by using multiple SQL INSERT statements. This drastically reduces the physical drive flushing time needed per single INSERT statements and communication overhead introduced when sending it to the database.
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>


#include "uthash.h"
#include "libtrace.h"
#include "mysql.h"
#include "wifi_sniffer.h"	// custom defined header file

// MACRO DEFINITION

#define BOLD   		"\033[1m\033[30m"
#define RESET   	"\033[0m"
#define CLEAR_LINE	"%c[2K"
#define MACSTR		"%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a)	(a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

// Global variables
libtrace_t *trace = NULL;		// trace descriptor
libtrace_packet_t *packet = NULL;	// packet descriptor
libtrace_filter_t *filter = NULL;	// filter descriptor
MYSQL *conn = NULL;			// mysql connection descriptor

uint8_t size_8 = sizeof (uint8_t);
uint8_t size_16 = sizeof (uint16_t);
uint8_t size_32 = sizeof (uint32_t);
uint8_t size_64 = sizeof (uint64_t);

char query_buffer[1024*1024] = "INSERT INTO wifi_sniffer VALUES";

/* MCS data rate array [bandwidth][guard_interval][MCS_index]
* ┌───────────────────────────────────────────────────────────────────────────────────────────────────┐
* │ data_rate [Mbits/sec] = bits/symbol * FEC_rate * subcarrier_count * symbol_rate * spatial streams |
* └───────────────────────────────────────────────────────────────────────────────────────────────────┘
*  subcarrier_count @ 20Mhz = 52, @ 40Mhz = 108
*  symbol_rate @ long GI = 0.25 Msym/sec, @ short GI = 0.2778 Msym/sec
* ┌─────────────┬─────────────┬──────────┬─────────────────┐
* │  MCS_index	│ bits/symbol │ FEC_rate │ spatial streams |
* ├─────────────┼─────────────┼──────────┼─────────────────┤
* │ 0,8,16,24	│	1     │    1/2   │	1,2,3,4	   │
* │ 1,9,17,25	│	2     │    1/2   │	1,2,3,4	   │
* │ 2,10,18,26	│	2     │    3/4   │	1,2,3,4	   │
* │ 3,11,19,27	│	4     │    1/2   │	1,2,3,4	   │
* │ 4,12,20,28	│	4     │    3/4   │	1,2,3,4	   │
* │ 5,13,21,29	│	6     │    2/3   │	1,2,3,4	   │
* │ 6,14,22,30	│	6     │    3/4   │	1,2,3,4	   │
* │ 7,15,23,31	│	6     │    5/6   │	1,2,3,4	   │
* └─────────────┴─────────────┴──────────┴─────────────────┘
*/
float MCS_data_rate[2][2][32]= {{{6.5,	13.0,	19.5,	26.0,	39.0,	52.0,	58.5,	65.0,	13.0,	26.0,	39.0,	52.0,	78.0,	104.0,	117.0,	130.0,	19.5,	39.0,	58.5,	78.0,	117.0,	156.0,	175.5,	195.0,	26.0,	52.0,	78.0,	104.0,	156.0,	208.0,	234.0,	260.0},
				 {7.2,	14.4,	21.7,	28.9,	43.3,	57.8,	65.0,	72.2,	14.4,	28.9,	43.3,	57.8,	86.7,	115.6,	130.0,	144.4,	21.7,	43.3,	65.0,	86.7,	130.0,	173.3,	195.0,	216.7,	28.8,	57.6,	86.8,	115.6,	173.2,	231.2,	260.0,	288.8}},
				{{13.5,	27.0,	40.5,	54.0,	81.0,	108.0,	121.5,	135.0,	27.0,	54.0,	81.0,	108.0,	162.0,	216.0,	243.0,	270.0,	40.5,	81.0,	121.5,	162.0,	243.0,	324.0,	364.5,	405.0,	54.0,	108.0,	162.0,	216.0,	324.0,	432.0,	486.0,	540.0},
				 {15.0,	30.0,	45.0,	60.0,	90.0,	120.0,	135.0,	150.0,	30.0,	60.0,	90.0,	120.0,	180.0,	240.0,	270.0,	300.0,	45.0,	90.0,	135.0,	180.0,	270.0,	360.0,	405.0,	450.0,	60.0,	120.0,	180.0,	240.0,	360.0,	480.0,	540.0,	600.0}}};

/* OFDM_HT bits per channel [bandwidth][MCS_index]
* ┌────────────────────────────────────────────────────────────────────────────────┐
* │ bits per channel = bits/symbol * FEC_rate * subcarrier_count * spatial streams |
* └────────────────────────────────────────────────────────────────────────────────┘
*  subcarrier_count @ 20Mhz = 52, @ 40Mhz = 108
*/
uint16_t MCS_bits_per_channel[2][32]={{26, 52, 78, 104, 156, 208, 234, 260, 52, 104, 156, 208, 312, 416, 468, 520, 78, 156, 234, 312, 468, 624, 702, 780, 104, 208, 312, 416, 624, 832, 936, 1040},
				  {54, 108, 162, 216, 324, 432, 486, 540, 108, 216, 324, 432, 648, 864, 972, 1080, 162, 324, 486, 648, 972, 1296, 1458, 1620, 216, 432, 648, 864, 1296, 1728, 1944, 2160}};

uint32_t recvd_pkt_count = 0, fltrd_pkt_count = 0, start_pkt_count, total_pkt_count;

/* Structure definition of Frame Check Sequence list for 802.11 control frames */
struct RECORD_ptr_t
{
	uint32_t FCS;
	UT_hash_handle hh;		// hash function handler
};
struct RECORD_ptr_t *RECORD_ptr, *hash_ptr = NULL;

/* ++++++++ libtrace.h extenstion for radio tap fields +++++++ */
/*
  The NetBSD ieee80211_radiotap man page
  (http://netbsd.gw.com/cgi-bin/man-cgi?ieee80211_radiotap+9+NetBSD-current)
  Radiotap capture fields must be naturally aligned.  That is, 16-, 32-,
  and 64-bit fields must begin on 16-, 32-, and 64-bit boundaries, respectively.
  In this way, drivers can avoid unaligned accesses to radiotap
  capture fields.  radiotap-compliant drivers must insert padding before a
  capture field to ensure its natural alignment.  radiotap-compliant packet
  dissectors, such as tcpdump(8), expect the padding.
*/
/* return the radiotap header size until the give field */
uint8_t radioTap_header_size_until(libtrace_radiotap_t *rtap, uint8_t field)
{
	uint8_t len = 0;

	if (rtap->it_present & (1 << TRACE_RADIOTAP_TSFT))
	{
		if(field > TRACE_RADIOTAP_TSFT)
			len += size_64;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_FLAGS))
	{
		if(field > TRACE_RADIOTAP_FLAGS)
			len += size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_RATE))
	{
		if(field > TRACE_RADIOTAP_RATE)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_CHANNEL))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_CHANNEL)
			len+= size_32;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_FHSS))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_FHSS)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DBM_ANTSIGNAL))
	{
		if(field > TRACE_RADIOTAP_DBM_ANTSIGNAL)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DBM_ANTNOISE))
	{
		if(field > TRACE_RADIOTAP_DBM_ANTNOISE)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_LOCK_QUALITY))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_LOCK_QUALITY)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_TX_ATTENUATION))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_TX_ATTENUATION)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DB_TX_ATTENUATION))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_DB_TX_ATTENUATION)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DBM_TX_POWER))
	{
		if(field > TRACE_RADIOTAP_DBM_TX_POWER)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_ANTENNA))
	{
		if(field > TRACE_RADIOTAP_ANTENNA)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DB_ANTSIGNAL))
	{
		if(field > TRACE_RADIOTAP_DB_ANTSIGNAL)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DB_ANTNOISE))
	{
		if(field > TRACE_RADIOTAP_DB_ANTNOISE)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_RX_FLAGS))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_RX_FLAGS)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_TX_FLAGS))
	{
		/* 16 bit natural alignment */
		while ( len % size_16) len+= size_8;
		if(field > TRACE_RADIOTAP_TX_FLAGS)
			len+= size_16;
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_RTS_RETRIES))
	{
		if(field > TRACE_RADIOTAP_RTS_RETRIES)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_DATA_RETRIES))
	{
		if(field > TRACE_RADIOTAP_DATA_RETRIES)
			len+= size_8;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_MCS))
	{
		if(field > TRACE_RADIOTAP_MCS)
			len+= size_8 + size_16;	/* Always aligned */
		else
			return len;
	}
	if (rtap->it_present & (1 << TRACE_RADIOTAP_AMPDU_STATUS))
	{
		/* 32 bit natural alignment */
		while ( len % size_32) len+= size_8;
		if(field > TRACE_RADIOTAP_AMPDU_STATUS)
			len+= size_64;
		else
			return len;
	}

	return len;
}
/* Function definition similar to trace_get_wireless_signal_strength_dbm libtrace function for 802.11n High Throghput */
bool trace_get_wireless_HT_signal_strength_dbm (void *link, int8_t *SSI)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;

	if (rtap->it_present & (1 << TRACE_RADIOTAP_DBM_ANTSIGNAL))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the SSI field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_DBM_ANTSIGNAL);
		SSI = (int8_t *) p;
		return true;
	}
	else
	{
		return false;
	}
}

/* Function definition similar to trace_get_wireless_rate libtrace function for 802.11n High Throughput */
bool trace_get_wireless_HT_rate(void *link, float *rate)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;
	struct MCS_INFO_t *MCS_INFO;

	if (rtap->it_present & (1 << TRACE_RADIOTAP_MCS))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the HT_rate field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_MCS);
		MCS_INFO = (struct MCS_INFO_t *)p;

		*rate = MCS_data_rate[MCS_INFO->bandwidth][MCS_INFO->guard][MCS_INFO->index];
		return true;
	}
	else
	{
		return false;
	}
}

/* Function definition to retrieve the preamble type stored in a radiotap header */
bool trace_get_wireless_preamble_type(void *link, char preamble)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;

	if (rtap->it_present & (1 << TRACE_RADIOTAP_FLAGS))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the flags field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_FLAGS);

		if(*p & (1 << 1))
			preamble = 'S';
		else
			preamble = 'L';
		return true;
	}
	else
	{
		return false;
	}
}

/* Function definition to retrieve the 802.11 channel type stored in a radiotap header */
struct channel_type_t * trace_get_wireless_channel_type(void *link)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;
	struct channel_type_t *channel_type;
	if (rtap->it_present & (1 << TRACE_RADIOTAP_CHANNEL))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the channel type field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_CHANNEL) + sizeof(uint16_t);
		channel_type = (struct channel_type_t *)p;
		return channel_type;
	}
	else
	{
		return NULL;
	}
}

/* Function definition to calculate the number of bits used in an OFDM symbol */
bool trace_get_wireless_HT_bits_per_channel(void *link, uint16_t *bits)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;
	struct MCS_INFO_t *MCS_INFO;

	if (rtap->it_present & (1 << TRACE_RADIOTAP_MCS))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the HT_rate field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_MCS);
		MCS_INFO = (struct MCS_INFO_t *)p;

		*bits = MCS_bits_per_channel[MCS_INFO->bandwidth][MCS_INFO->index];
		return true;
	}
	else
	{
		return false;
	}
}

/* Function to return the PLCP duration for High Throughput  */
uint16_t trace_get_wireless_HT_mixed_PLCP_duration(void *link)
{
	struct libtrace_radiotap_t *rtap = (struct libtrace_radiotap_t *)link;
	uint8_t * p;
	struct MCS_INFO_t *MCS_INFO;
	uint8_t streams;
	
	if (rtap->it_present & (1 << TRACE_RADIOTAP_MCS))
	{
		/* Skip over any extended bitmasks */
		p = (uint8_t *) &(rtap->it_present);
		while ( *((uint32_t*)p) & (1U << TRACE_RADIOTAP_EXT) )
		{
			p += sizeof (uint32_t);
		}

		/* Point at the HT_rate field of radiotap header*/
		p += sizeof(uint32_t) + radioTap_header_size_until(rtap,TRACE_RADIOTAP_MCS);
		MCS_INFO = (struct MCS_INFO_t *)p;

		// number of spacial streams used
		streams = ((((MCS_INFO->index) & 0x78) >> 3) + 1);
		
		return (32 + 4*streams);
	}
	else
	{
		return 20;
	}
}

// Get the mac address of the specifed interface
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

// Get wireless parameter of the specified interface
int32_t get_iwpar(char *if_name, int request)
{
	int fd;
	struct iwreq wrq;

	// Open socket
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	// Clear our request and set the interface name
	memset (&wrq, 0, sizeof (struct iwreq));
	strncpy ((char *)&wrq.ifr_name, if_name, IFNAMSIZ);

	// Do the request
	ioctl(fd, request, &wrq);

	// Close socket
	close(fd);

	switch(request)
	{
		// transmit power
		case SIOCGIWTXPOW:
			return wrq.u.txpower.value;
		// transmit frequency
		case SIOCGIWFREQ:
			return wrq.u.freq.m;
		default:
			return 0;
	}
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
void cleanup()
{
	// Insert the remaining data into DB
	query_buffer[strlen(query_buffer)-1] = ';';
	if(mysql_query(conn, query_buffer) != 0)
		fprintf(stderr, "wifi_sniffer DB query error!\n");

	// clean libtrace resources
	libtrace_cleanup(trace, packet, filter);

	// clean mysql database resources
	mysql_close(conn);

	exit(0);
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
	char *bpf_str = "";
	uint32_t interval = 100000;
	enum flag_t pkt_stat = ENABLED;
	enum flag_t unique_FCS = DISABLED;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:f:b:i:s:U:")) != -1)
	{
		switch (option)
		{
			case 'h':
				fprintf(stderr,"Description\n");
				fprintf(stderr,"-----------\n");
				fprintf(stderr,BOLD "wifi_sniffer" RESET " is an application built on top of the libtrace packet capturing library which\n");
				fprintf(stderr,"captures physical and MAC layer Wi-Fi parameters and stores it into database.\n");
				fprintf(stderr,"Parameters include but not limited to Frame Check Sequence, Data rate, Frequency, SSI,\n");
				fprintf(stderr,"MAC address and Frame Length.\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-f traceURI\t\ttrace format type [eg. -f int:wlan0]\n");
				fprintf(stderr,"-b filter\t\taccept only packets that match the bpf filter expression [eg. -b 'not ether src host 00:0e:8e:30:9e:68'] \n");
				fprintf(stderr,"-i interval\t\tinterval (usec) between periodic reports [eg. -i 100000]\n");
				fprintf(stderr,"-s pkt_stat\t\tenable/disable packet statistics [eg. -s 1]\n");
				fprintf(stderr,"-U unique_FCS\t\tenable/disable unique Frame Check Sequence [eg. -U 0]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./wifi_sniffer -n 10.11.31.5 -u qocon -k qoconpass -d qocon -f int:wlan0 -b 'not ether src host 00:0e:8e:30:9e:68'\n" RESET);
				fprintf(stderr,"calculate COT metric on " BOLD "wlan0" RESET " interface from all Wi-FI sources except the MAC 00:0e:8e:30:9e:68.\n");
				fprintf(stderr,"Finally send the data into a database server with IP 10.11.31.5, user qocon, passwd qoconpass and database qocon.\n\n");
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
			case 'b':
				bpf_str = strdup(optarg);
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			case 's':
				pkt_stat = atoi(optarg);
				break;
			case 'U':
				unique_FCS = atoi(optarg);
				break;
			default:
				fprintf(stderr,"wifi_sniffer: missing operand. Type './wifi_sniffer -h' for more information\n");
				return 1;
		}
	}

	// House keeping the command line input
	if(host == NULL || user == NULL || passwd == NULL || db == NULL || traceURI == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database/Trace format] is not specified. Type './wifi_sniffer -h' for more information\n");
		return 1;
	}

	// Packet statistics check-up
	if(pkt_stat == ENABLED  && strcmp(traceURI_type, "ring") != 0 && strcmp(traceURI_type, "int") != 0 && strcmp(traceURI_type, "pcapint") != 0)
	{
		pkt_stat = DISABLED;
		fprintf(stderr,"Packet statistics will be disabled. It is allowed only for [ring|int|pcapint] libtrace interfaces.\n");
	}

	uint32_t remaining;
	void* linkptr;
	libtrace_linktype_t linktype;
	libtrace_80211_t *L2_header;
	struct libtrace_radiotap_t *rtap;

	char preamble;
	char sniffer_mac[18];
	uint8_t rate_int;
	float rate_float;
	uint16_t freq;
	uint16_t bits_per_channel;
	int8_t SSI;
	struct channel_type_t *channel_type;
	uint16_t PLCP_duration;
	
	enum frame_type_t frame_type;
	union frame_subtype_t frame_subtype;
	uint8_t flag;

	char dst_mac[18], src_mac[18];
	uint8_t frag_no;
	uint16_t seq_no;
	uint32_t FCS;
	
	float COT;

	struct timeval systime;
	uint64_t previous_time, current_time;

	char query_statement[164];
	char command[64];


	// handle SIGTERM and SIGINT
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	// initialize mysql database
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		fprintf(stderr,"Description\n");
		return 1;
	}

	// connect to qocon database
	if (mysql_real_connect(conn, host, user, passwd, db, 0, NULL, 0) == NULL)
	{
		fprintf(stderr,"Database connection error!\n");
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

	// Start capturing the trace
	if (trace_start(trace) == -1) {
		trace_perror(trace,"Starting trace");
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}

	// Create the bpf filter
	filter = trace_create_filter(bpf_str);
	if (filter == NULL) {
		trace_perror(trace,"Failed to create filter(%s)\n", bpf_str);
		libtrace_cleanup(trace, packet, filter);
		return 1;
	}
	
	// Retrieve sniffer MAC address
	get_mac_addr(traceURI_if, sniffer_mac);

	// Log the start time
	gettimeofday(&systime, NULL);
	previous_time = 1e3*systime.tv_sec + (int)(systime.tv_usec/1e3);

	// If packet statistics is enabled
	if(pkt_stat == ENABLED)
	{
		// Calculate the start packet count on the given interface
		sprintf(command,"cat /proc/net/dev | grep %s | awk -F' ' '{print $3}'", traceURI_if);
		FILE *fp = popen(command, "r");
		if(fp != NULL)
		{
			fgets(command, sizeof(command), fp);
			start_pkt_count = atoi(command);
		}
	}

	/* This loop will read packets from the trace until either EOF is
	 * reached or an error occurs (hopefully the former!)
	 *
	 * Remember, EOF will return 0 so we only want to continue looping
	 * as long as the return value is greater than zero
	 */

	while (trace_read_packet(trace, packet) > 0)
	{
		// Count the number of recieved packets
		recvd_pkt_count++;

		// Apply the filter to the packet
		if(trace_apply_filter(filter, packet) <= 0)
			continue;

		// Get a pointer to the first packet buffer
		linkptr = trace_get_packet_buffer( packet, &linktype, &remaining);

		/* Physical layer parameters */
		rtap = (struct libtrace_radiotap_t *)linkptr;
		// 802.11n packets
		if(rtap->it_present & (1 << TRACE_RADIOTAP_MCS))
		{
			preamble = '\0';
			// Retrieve the data rate from the RadioTap header
			if(!trace_get_wireless_HT_rate (linkptr, &rate_float))
				continue;
			// Retrieve the center frequency from the RadioTap header
			if(!trace_get_wireless_freq (linkptr, linktype, &freq))
				freq = 0;
			// Retrieve the number of bits used for an OFDM channel
			if(!trace_get_wireless_HT_bits_per_channel(linkptr,&bits_per_channel))
				bits_per_channel = 26;
			// Retrieve the SSI from the RadioTap header
			if(!trace_get_wireless_HT_signal_strength_dbm (linkptr, &SSI))
				SSI = 0;
			// Retrieve wireless channel type. If it does not exist, assume PLCP duration of 20 usec
			if((channel_type = trace_get_wireless_channel_type(linkptr)) == NULL)
				PLCP_duration = 20;
			// It it does, retrieve the PLCP preamble + header duration. We assume HT-mixed PHY frame format
			else
				PLCP_duration=trace_get_wireless_HT_mixed_PLCP_duration(linkptr);
		}
		// 802.11abg packets
		else
		{
			// Retrieve wireless preamble type (i.e. short/long)
			if(!trace_get_wireless_preamble_type(linkptr, preamble))
				preamble = 'L';
			// Retrieve the data rate from the RadioTap header
			if(!trace_get_wireless_rate (linkptr, linktype, &rate_int))
				continue;
			else
				rate_float = (double)(rate_int/2);
			// Retrieve the center frequency from the RadioTap header
			if(!trace_get_wireless_freq (linkptr, linktype, &freq))
				freq = 0;
			// number of bits used for an OFDM channel
			bits_per_channel = 2*rate_int;
			// Retrieve the SSI from the RadioTap header
			if(!trace_get_wireless_signal_strength_dbm (linkptr, linktype, &SSI))
				SSI = 0;
			// Retrieve wireless channel type. If it does not exist, assume PLCP duration of 20 usec
			if((channel_type = trace_get_wireless_channel_type(linkptr)) == NULL)
				PLCP_duration = 20;
			// If it does exist
			else
			{
				// 2.4/5 Ghz legacy OFDM transmission
				if(channel_type->ofdm == 1)
					PLCP_duration = 20;
				// 2.4 Ghz CCK transmission with short preamble
				else if((channel_type->cck == 1) && (preamble == 'S'))
					PLCP_duration = 96;
				// 2.4 Ghz CCK transmission with long preamble
				else if((channel_type->cck == 1) && (preamble  == 'L'))
					PLCP_duration = 192;
			}

		}

		/* MAC layer parameters
		   802.11 frame types https://supportforums.cisco.com/docs/DOC-13664 */

		// Retrieve Layer 2 header
		L2_header = trace_get_layer2(packet,&linktype,&remaining);

		// Get the frame type and subtype from the 802.11 layer 2 header
		if(L2_header->type <= 2 && L2_header->subtype <= 15)
		{
			frame_type = L2_header->type;
			if(frame_type == MNGT)
				frame_subtype.mngt = L2_header->subtype;
			else if(frame_type == CTRL)
				frame_subtype.ctrl = L2_header->subtype;
			else if(frame_type == DATA)
				frame_subtype.data = L2_header->subtype;
		}
		else
			continue;

		// Copy the L2 flag field
		flag = (L2_header->to_ds) | (L2_header->from_ds << 1) | (L2_header->more_frag << 2) | (L2_header->retry << 3) | (L2_header->power << 4) | (L2_header->more_data << 5) | (L2_header->wep << 6) | (L2_header->order << 7);

		// Retrieve physical receiver MAC address.
		trace_ether_ntoa(L2_header->mac1,dst_mac);
		
		// Retrieve physical source MAC address if it is not an ACK or CTS packet.
		if (frame_type == CTRL && (frame_subtype.ctrl == ACK || frame_subtype.ctrl == CTS))
			strcpy(src_mac,"00:00:00:00:00:00");
		else
			trace_ether_ntoa(L2_header->mac2,src_mac);

		// Check if the packet was in transmit path. If so, retrieve txpower and freq since the Radiotap header does not contain this information
		if(strcmp(sniffer_mac, src_mac) == 0)
		{
			freq = (uint16_t) get_iwpar(traceURI_if, SIOCGIWFREQ);
			SSI  = (int8_t)   get_iwpar(traceURI_if, SIOCGIWTXPOW);
		}

		// Retrieve fragment number and sequence number for data and managment frames
		if (frame_type == DATA || frame_type == MNGT)
		{
			frag_no = (L2_header->SeqCtl) & 0x000F;
			seq_no  = (L2_header->SeqCtl) >> 4;
		}
		else
		{
			frag_no = 0;
			seq_no  = 0;
		}

		/* Per packet COT calculation */
		// 2.4 Ghz CCK transmission
		if(rate_float == 1.0 || rate_float == 2.0 || rate_float == 5.5 || rate_float == 11.0)
			COT = PLCP_duration + 8.0*remaining/rate_float;
		// 2.4/5 Ghz OFDM transmission
		else
			COT = PLCP_duration + bits_per_channel*ceil((22+8.0*remaining)/bits_per_channel)/rate_float;

		// Retrieve Frame Check Sequence. It is the last four byte in a frame
		memcpy(&FCS,(uint8_t *)L2_header+remaining-4,sizeof(FCS));

		// Get the time of the day
		gettimeofday(&systime, NULL);
		if(unique_FCS == ENABLED)
		{
			// Unique FCS fields are enforced by randomly generating new values for those FCS who has reoccured.
			// Hash tables are used to store the original and the next FCS fields (incase of reoccurance).
			// Hash table implementation for c : https://github.com/troydhanson/uthash
			HASH_FIND_INT(hash_ptr,&FCS,RECORD_ptr);
			if(RECORD_ptr == NULL)
			{
				RECORD_ptr = (struct RECORD_ptr_t*)malloc(sizeof(struct RECORD_ptr_t));
				RECORD_ptr->FCS = FCS;
				HASH_ADD_INT(hash_ptr, FCS, RECORD_ptr);
			}
			else
			{
				// Assigna a random number by seeding [modified] current time to FCS
				// systime = a[bcd|efgh|ijkl]|mnop|qrst|uvwx|yz![@]|#$%^ => FCS_time = bcde|fghi|jkl@
				// While systime has 1usec precison, FCS_time has 62.5msec precision
				// It is assumed that identical FCS packets (i.e. ACK, RTS, CTS, PS-POLL) do not appear more than once within a single FCS_time duration
				uint32_t FCS_time = ((systime.tv_sec << 4) & 0xFFFFFFF0) + ((systime.tv_usec >> 16) & 0x0000000F);
				srand (FCS_time);
				FCS = rand();
			}
		}

		// Count the number of filtered packets
		fltrd_pkt_count++;

		// Get the current time in msec precision
		current_time = 1e6*systime.tv_sec + systime.tv_usec;

		// wifi_sniffer DB query statement and execution
		sprintf(query_statement, "(%lu,'%s',%2.1f,%d,%d,%d,%d,%u,'%s','%s',%u,%u,%f,'0x%x',%u),", current_time, sniffer_mac, rate_float, freq, SSI, frame_type, frame_subtype.val, flag, dst_mac, src_mac, frag_no, seq_no, COT, FCS, remaining);
		strcat(query_buffer,query_statement);

		// Reporting interval has expired. Time to store data into DB and print statistics
		if((current_time - previous_time) > interval)
		{
			query_buffer[strlen(query_buffer)-1] = ';';
			if(mysql_query(conn, query_buffer) != 0)
				fprintf(stderr, "wifi_sniffer DB query error!\n");
			sprintf(query_buffer,"INSERT INTO wifi_sniffer VALUES");

			// If packet statistics is enabled
			if(pkt_stat == ENABLED)
			{
				// Calculate the current packet count on the given interface
				sprintf(command,"cat /proc/net/dev | grep %s | awk -F' ' '{print $3}'", traceURI_if);
				FILE *fp = popen(command, "r");
				if(fp != NULL)
				{
					fgets(command, sizeof(command), fp);
					total_pkt_count = atoi(command)-start_pkt_count;
				}

				// Print packet statistics
				printf(CLEAR_LINE "TOTAL=%d\tRECIEVED=%d\tFILTERED=%d\tLOST=%d\r", 27, total_pkt_count, recvd_pkt_count, fltrd_pkt_count, (total_pkt_count-recvd_pkt_count)*(fltrd_pkt_count/recvd_pkt_count));
				fflush(stdout);
			}

			// update next round parameters
			previous_time = current_time;
		}
	}

	// Insert the remaining data into DB
	query_buffer[strlen(query_buffer)-1] = ';';
	if(mysql_query(conn, query_buffer) != 0)
		fprintf(stderr, "wifi_sniffer DB query error!\n");

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

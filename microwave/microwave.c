
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <complex.h>

#include "warplab_defines.h"
#include "pw_defines.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

int  packetNum = 1, UDP_Ports, isFinished = 0;
char IPAddrs[16], datarec[1500];

void warplab_pktSend(int udpSock, int pktDataTx[], int pktDataLen)
{
	struct sockaddr_in addr_udp;
	socklen_t len = sizeof(struct sockaddr_in);

	// Send a message to the specified address
	addr_udp.sin_family = AF_INET;
	addr_udp.sin_port = htons(UDP_Ports);
	addr_udp.sin_addr.s_addr  = inet_addr(IPAddrs);
	if(sendto(udpSock, pktDataTx, pktDataLen, 0, (struct sockaddr *)&addr_udp, sizeof(struct sockaddr_in)) < 0)
	{
		perror("SENDTO");
		exit(1);
	}

	// Receive a reply from WARP
	if(recvfrom(udpSock, datarec, sizeof(datarec), 0, (struct sockaddr *)&addr_udp, (socklen_t *)&len) < 0)
	{
		perror("RECV_SOCK_TIMEOUT");
		exit(1);
	}
}

void warplab_writeSMWO(int udpSock, int SMWO_id, complex double TxData[], int TxLength)
{
	int pktNoTx = 1, n,k,l;

	if(TxLength > pow(2,14))
	{
		fprintf(stderr, "ERROR: TxData must contain 16384 (2^14) samples maximum!");
		return;
	}

	int maxPayloadBytesPerPkt = 1024;

	int16_t *TxData_I_fi = (int16_t *) malloc(sizeof(int16_t) * TxLength);
	int16_t *TxData_Q_fi = (int16_t *) malloc(sizeof(int16_t) * TxLength);;
	for(k=0; k<TxLength; k++)
	{
		TxData_I_fi[k] = (int16_t) (creal(TxData[k])*pow(2,15));
		TxData_Q_fi[k] = (int16_t) (cimag(TxData[k])*pow(2,15));
	}

	int *TxPktData = (int *) malloc(sizeof(int) * TxLength);
	for(k=0; k<TxLength; k++)
		TxPktData[k] = pow(2,16)*(int)(TxData_I_fi[k]) + (int)((uint16_t)(TxData_Q_fi[k]));

	// length(TxPktData) is the number of samples, each of which is a 32-bit value
	// We'll send UDP packets with payloads of 1024 bytes, or 256 samples
	// This results in a maximum of 64 UDP packets to download a full TxData

	int numPkts = ceil(TxLength*4.0/maxPayloadBytesPerPkt);

	for (n=0; n<=numPkts-1; n++)
	{
		int indexStart = ((n*maxPayloadBytesPerPkt/4)+1);
		int indexEnd = MIN(TxLength, ((n+1)*maxPayloadBytesPerPkt/4));

		int *dataToSendTx = (int *) malloc(sizeof(int)*(3 + indexEnd - indexStart + 1));
		dataToSendTx[0] = htonl(pktNoTx);
		dataToSendTx[1] = htonl(SMWO_id);
		dataToSendTx[2] = htonl(indexStart-1);
		for(l=3,k=indexStart; k<=indexEnd; l++,k++)
			dataToSendTx[l] = htonl(TxPktData[k-1]);
		warplab_pktSend(udpSock, dataToSendTx, sizeof(int)*(3 + indexEnd - indexStart + 1));

		pktNoTx = pktNoTx+1;

		free(dataToSendTx);
	}

	free(TxData_I_fi);
	free(TxData_Q_fi);
	free(TxPktData);
}

void warplab_setAGCParameter(int nodeHandle, int parameter_Id, int parameter_Value)
{
	// Set the packet to be sent to the WARP node
	int pktDataTx[3] = {htonl(packetNum), htonl(parameter_Id), htonl(parameter_Value)};
	warplab_pktSend(nodeHandle, pktDataTx, sizeof(pktDataTx));
}

void warplab_writeRegister(int nodeHandle, int register_Id, int parameter_Value)
{

	int ReadWrite = 1; // Set ReadWrite to 1 to write to register

	// Set the packet to be sent to the WARP node
	int pktDataTx[4] = {htonl(packetNum), htonl(register_Id), htonl(ReadWrite), htonl(parameter_Value)};
	warplab_pktSend(nodeHandle, pktDataTx, sizeof(pktDataTx));
}

void warplab_setRadioParameter(int nodeHandle, int parameter_Id, int parameter_Value)
{
	// Set the packet to be sent to the WARP node
	int pktDataTx[3] = {htonl(packetNum), htonl(parameter_Id), htonl(parameter_Value)};
	warplab_pktSend(nodeHandle, pktDataTx, sizeof(pktDataTx));
}

void warplab_sendCmd(int udpSock, int commands)
{
	int pktDataTx[2] = {htonl(packetNum), htonl(commands)};
	warplab_pktSend(udpSock, pktDataTx, sizeof(pktDataTx));
}

void warplab_sendSync(int udpsync)
{
	char SyncIPAddr[16];
	struct sockaddr_in addr_sync;
	int pktDataTx = 0;

	// Synchronization address
	sprintf(SyncIPAddr, "%s.%d", WARP_subnet, SyncAddr);

	// Send a message to the specified address
	addr_sync.sin_family = AF_INET;
	addr_sync.sin_port = htons(SyncPort);
	addr_sync.sin_addr.s_addr  = inet_addr(SyncIPAddr);
	if (sendto(udpsync, &pktDataTx, sizeof(pktDataTx), 0, (struct sockaddr *)&addr_sync, sizeof(struct sockaddr_in)) < 0)
	{
		perror("SENDTO");
		exit(1);
	}
}

void warplab_initNodes(int nodeHandles, int nodeID)
{
	// Define the nodes' IP addresses & UDP ports
	// The IP address and UDP port ranges must match those hardc-coded in the FPGA
	sprintf(IPAddrs, "%s.%d", WARP_subnet, nodeID);
	UDP_Ports = NodeStartingPort + nodeID - 1;

	// Send the SYNC packet to clear any stale triggers
	warplab_sendSync(nodeHandles);

	// Send the INITIALIZE packets
	warplab_sendCmd(nodeHandles, INITIALIZE);
}

int warplab_initNets(int portNo)
{
	int socketHandles;
	struct timeval timeout;
	struct sockaddr_in addr_ctrl;
	int flag = 1;

	// UDP Connections to individual nodes
	socketHandles = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketHandles < 0)
	{
		perror("SOCKET");
		exit(1);
	}

	// Enable SO_REUSEADDR to allow multiple instances of this application
	if(setsockopt(socketHandles, SOL_SOCKET, SO_REUSEADDR,(char *)&flag, sizeof(flag)) < 0)
	{
		perror("SETSOCKOPT");
		exit(1);
	}

	// Set 5 second recieve socket timeout
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if (setsockopt (socketHandles, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) < 0)
	{
		perror("SO_RCVTIMEO");
		exit(1);
	}

	// Bind to the proper port number and IP address.
	memset(&addr_ctrl, 0, sizeof(struct sockaddr_in));
	addr_ctrl.sin_family = AF_INET;
	addr_ctrl.sin_port = htons(portNo);
	addr_ctrl.sin_addr.s_addr  = INADDR_ANY;
	if(bind(socketHandles, (struct sockaddr*)&addr_ctrl, sizeof(struct sockaddr_in)))
	{
		perror("BIND");
		exit(1);
	}

	// Allow Broadcast packets to be sent using this socket handler
	if(setsockopt(socketHandles, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)))
	{
		perror("SETSOCKOPT");
		exit(1);
	}

	return socketHandles;
}

int warplab_initialize(int portNo, int nodeID)
{
	int socketHandles;

	socketHandles = warplab_initNets(portNo);
	warplab_initNodes(socketHandles, nodeID);

	return socketHandles;
}

void closeWARPSE(int udp_txrx)
{
	// %%%%%%% STOP %%%%%%%% //
	warplab_writeRegister(udp_txrx, PW_STOP_REC, 1);

	// --------------------------------------------------------------------------
	// 4. Reset and disable the boards
	// --------------------------------------------------------------------------
	warplab_sendCmd(udp_txrx, RADIO2TXBUFF_TXDIS);
	warplab_sendCmd(udp_txrx, RADIO2_TXDIS);
	warplab_sendCmd(udp_txrx, RADIO3RXBUFF_RXDIS);
	warplab_sendCmd(udp_txrx, RADIO3_RXDIS);

	// Close sockets
	close(udp_txrx);
}

// Set the finished experiment flag
void sigHandler()
{
	isFinished = 1;
}


int main (int argc, char *argv[])
{
	int nodeID = 1;
	int portNo = 9000;
	int Radio2_TxGain_BB = 3;
	int Radio2_TxGain_RF = 10;
	int CarrierChannel_r2 = 1;
	int period = 4000;
	int num_of_fft = 3000;

	int option = -1;
	while ((option = getopt (argc, argv, "hi:p:b:r:c:P:f:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr, "Microwave signal generator using WARP platform\n\n");
				fprintf(stderr, "Argument list\n");
				fprintf(stderr, "-------------\n");
				fprintf(stderr, "-h\t\t\t\thelp menu\n");
				fprintf(stderr, "-i nodeID\t\t\tWARP board identifier [eg. -n 1]\n");
				fprintf(stderr, "-p portNo\t\t\tWARP board port number [eg. -p 9000]\n");
				fprintf(stderr, "-b Radio2_TxGain_BB\t\tRadio 2 baseband transmit gain [eg. -b 3]\n");
				fprintf(stderr, "-r Radio2_TxGain_RF\t\tRadio 2 radio frequency transmit gain [eg. -r 10]\n");
				fprintf(stderr, "-c CarrierChannel_r2\t\tCarrier channel number [eg. -c 1]\n");
				fprintf(stderr, "-P period\t\t\tSignal period [eg. -P 4000]\n");
				fprintf(stderr, "-f num_of_fft\t\t\tnumber of fft per period [eg. -f 3000]\n");
				return 1;
			case 'i':
				nodeID = atoi(optarg);
				break;
			case 'p':
				portNo = atoi(optarg);
				break;
			case 'b':
				Radio2_TxGain_BB = atoi(optarg);
				break;
			case 'r':
				Radio2_TxGain_RF = atoi(optarg);
				break;
			case 'c':
				CarrierChannel_r2 = atoi(optarg);
				break;
			case 'P':
				period = atoi(optarg);
				break;
			case 'f':
				num_of_fft = atoi(optarg);
				break;
			default:
				fprintf(stderr, "microwave: missing operand. Type './microwave -h' for more information.\n");
				exit(1);
	    }
	}

	int udp_txrx;
	int num_of_idle_lines = period - num_of_fft;

	// handle SIGTERM and SIGINT
	signal(SIGINT,  sigHandler);
	signal(SIGTERM, sigHandler);

	// Initialize WARP board
	udp_txrx = warplab_initialize(portNo, nodeID);

	// %%%%%%%%%%%%%%%%%%%%%%%% channel definition  %%%%%%%%%%%%%%%%%%%%%%%%% //
	warplab_setRadioParameter(udp_txrx, CARRIER_CHANNEL_R2, CarrierChannel_r2);
//	warplab_setRadioParameter(udp_txrx, CARRIER_CHANNEL_R3, CarrierChannel_r3);
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% //

	int Node1_MGC_AGC_Select = 0;
	int TxDelay = 0; // Number of noise samples per Rx capture. In [0:2^14]

	int TxMode = 1; // Transmission mode. In [0:1]
		        // 0: Single Transmission
		        // 1: Continuous Transmission.

		        // Zigbee op channel 25 of 2475 MHZ zou op WIFI channel 11 moeten
		        // overeenkomen met bin 845 op een 1024 point FFT.
	warplab_writeRegister(udp_txrx, TX_DELAY, TxDelay);
	warplab_writeRegister(udp_txrx, TX_MODE, TxMode);

 
	// gain configurations 
	warplab_setRadioParameter(udp_txrx, RADIO2_TXGAINS, (Radio2_TxGain_RF + Radio2_TxGain_BB*pow(2,16)));
//	warplab_setRadioParameter(udp_txrx, RADIO3_RXGAINS, (Radio3_RxGain_BB + Radio3_RxGain_RF*pow(2,16)));
	warplab_setAGCParameter(udp_txrx, MGC_AGC_SEL, Node1_MGC_AGC_Select);

	// %%%%%%%%%%%%%%%%%%%%%%%% PW_PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% //

	// Select global receive mode
	warplab_writeRegister(udp_txrx, PW_REC_MODE_SEL, 1);
		        // [1] = Continuous / [0] = Standard WARPlab mode
		        
	// Select Transmission Medium
	warplab_writeRegister(udp_txrx, PW_WIRELESS_OR_LOCAL, 1);
		        // [1] = Wireless / [0] = Locally
		        
	// Select BYPASS when routing is local
	warplab_writeRegister(udp_txrx, PW_LOCAL_BYPASS, 0);
		        // [1] = Bypassed / [0] = Not bypassed
		        
	// Select BYPASS when routing is wireless
	warplab_writeRegister(udp_txrx, PW_WIRELESS_BYPASS, 0);
		        // [1] = Bypassed / [0] = Not bypassed
		        
	// Select Transmission Medium
	warplab_writeRegister(udp_txrx, PW_DEBUG_MODE_DATA, 0);
		        // [1] = Debug data / [0] = Real Data
		        
	// Make sure STOP_REC is zero
	warplab_writeRegister(udp_txrx, PW_STOP_REC, 0);

	// Channel Initiliazation
//	int f_sample = 40000000;
	int fft_size = 1024; 
	int data_limit = fft_size; 
	// combine 20 measurements in one packet
	// for some reason, when datalimit is small, the measurements can get messed
	// up, eg the DC is no longer at the center, probably somewhere in the
	// ethernet socket there is overflow 
	warplab_writeRegister(udp_txrx, PW_DATA_LIMIT, data_limit);

	// channel_cycles = 14 (defined in lw_defines; % internal record = 2^channel_cycles*1024/40000 ms 
	warplab_writeRegister(udp_txrx, PW_CHANNEL_CYCLES, channel_cycles);


	// 1 : maxhold, 0 : average 
	int se_mode = 0;        
	warplab_writeRegister(udp_txrx, LW_MODE, se_mode);

	warplab_sendCmd(udp_txrx, PW_SEND_CONFIGURATION);

	//
	//--------------------------------------------------------------------------
	// 2. load the modulated 1024 qam mod samples from the mat file, reshape and
	// scale
	//--------------------------------------------------------------------------
	// define the dimention of the spectrogram
	int row_length = 2;
//	int nrow = 2;
//	tx_signal1 = reshape(qam_signal',1,row_length*nrow);
	// scale the data 
//	scale1 = 16*max([abs(real(tx_signal1)) abs(imag(tx_signal1))]) ;

//	if (scale1 >= 1)
//	    tx_signal1 = tx_signal1 ./ scale1;
//	end
	complex double tx_signal[4] = {-0.0625 - 0.0625i, -0.0625 - 0.0625i, -0.0625 - 0.0625i, -0.0625 - 0.0625i};


	//--------------------------------------------------------------------------
	// 3. write the values into buffers and registers  
	//--------------------------------------------------------------------------
//	Node1_Radio2_TxData = tx_signal1 ; %.*(2^(scale_factor-1));

	warplab_writeRegister(udp_txrx, TX_LENGTH, 4) ;
	warplab_writeRegister(udp_txrx, LW_FIG_ROW_LENGTH, row_length);
	// Download the samples to be transmitted
	warplab_writeSMWO(udp_txrx, RADIO2_TXDATA, tx_signal, 4);
	// warplab_writeSMWO(udp_txrx, RADIO2_TXDATA_2, Node1_Radio2_TxData_2);
	warplab_writeRegister(udp_txrx, LW_NUM_OF_FFT, num_of_fft);
	warplab_writeRegister(udp_txrx, LW_NUM_OF_IDLE_LINES, num_of_idle_lines);
	// Note that the sum of num_of_fft and num_of_idle lines should not be bigger than 16384
	int pix_repeat = 256; // in total 1024 = 40 MHz, 2 columns per row, 2*256=512 => 20 MHz
	warplab_writeRegister(udp_txrx, LW_PIX_REPEAT, pix_repeat);
	warplab_writeRegister(udp_txrx, LW_TX_IFFT_EN, 1); 
	// ----------------------------------------------------------------
	// 4. prepare the filter 
	// --------------------------------------------------------------------------
	// 1) enable the filter option 
	warplab_writeRegister(udp_txrx, LW_TX_FILTER_EN, 0);


	// --------------------------------------------------------------------------
	// 2. Prepare WARP boards for transmission and reception and send trigger to
	// start transmission and reception (trigger is the SYNC packet)
	// --------------------------------------------------------------------------
	warplab_writeRegister(udp_txrx, PW_STOP_REC, 0);
	// put both radio into RX mode 
	warplab_sendCmd(udp_txrx, RADIO2_TXEN);
	warplab_sendCmd(udp_txrx, RADIO2TXBUFF_TXEN);
	// warplab_sendCmd(udp_txrx, RADIO3_RXEN);
	// warplab_sendCmd(udp_txrx, RADIO3RXBUFF_RXEN);
	// warplab_sendCmd(udp_txrx, TXRX_START);       // ATTENTION: Make sure you're using RX or TXRX !
	warplab_sendCmd(udp_txrx, TX_START); 
	warplab_sendSync(udp_txrx);

	// --------------------------------------------------------------------------
	// 4. initialize the global variable as flag, register close request
	// to the figure and start running
	// --------------------------------------------------------------------------
	while(isFinished == 0)
	{
		sleep(1);
	}

	closeWARPSE(udp_txrx);

	return 0;
}


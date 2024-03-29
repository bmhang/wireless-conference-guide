
// Define the magic SYNC address; this must match the address hard-coded in the FPGA
char WARP_subnet[] = "10.0.0";
int SyncAddr = 255;
int ComputerAddr = 200;

int SyncPort = 10000;
int NodeStartingPort = 9000;

int channel_cycles = 14; // internal record = 2^channel_cycles*1024/40000 ms 

int ACK = 1;
int NOACK = 2;

int INITIALIZE = 100;
int NETWORKCHECK = 101;
int BOARDINFO = 102;

int RADIO1_TXEN = 1001;
int RADIO2_TXEN = 1002;
int RADIO3_TXEN = 1003;
int RADIO4_TXEN = 1004;
int RADIO1_TXDIS = 1005;
int RADIO2_TXDIS = 1006;
int RADIO3_TXDIS = 1007;
int RADIO4_TXDIS = 1008;

int RADIO1_RXEN = 1009;
int RADIO2_RXEN = 1010;
int RADIO3_RXEN = 1011;
int RADIO4_RXEN = 1012;
int RADIO1_RXDIS = 1013;
int RADIO2_RXDIS = 1014;
int RADIO3_RXDIS = 1015;
int RADIO4_RXDIS = 1016;

int RADIO1_TXDATA = 1101;
int RADIO2_TXDATA = 1102;
int RADIO3_TXDATA = 1103;
int RADIO4_TXDATA = 1104;
int RADIO1_RXDATA = 1105;
int RADIO2_RXDATA = 1106;
int RADIO3_RXDATA = 1107;
int RADIO4_RXDATA = 1108;

int RADIO1_RSSIDATA = 1109;
int RADIO2_RSSIDATA = 1110;
int RADIO3_RSSIDATA = 1111;
int RADIO4_RSSIDATA = 1112;

int RADIO1TXBUFF_TXEN = 1113;
int RADIO2TXBUFF_TXEN = 1114;
int RADIO3TXBUFF_TXEN = 1115;
int RADIO4TXBUFF_TXEN = 1116;
int RADIO1TXBUFF_TXDIS = 1117;
int RADIO2TXBUFF_TXDIS = 1118;
int RADIO3TXBUFF_TXDIS = 1119;
int RADIO4TXBUFF_TXDIS = 1120;

int RADIO1RXBUFF_RXEN = 1121;
int RADIO2RXBUFF_RXEN = 1122;
int RADIO3RXBUFF_RXEN = 1123;
int RADIO4RXBUFF_RXEN = 1124;
int RADIO1RXBUFF_RXDIS = 1125;
int RADIO2RXBUFF_RXDIS = 1126;
int RADIO3RXBUFF_RXDIS = 1127;
int RADIO4RXBUFF_RXDIS = 1128;

int TX_START = 2000;
int RX_START = 2001;
int RX_DONEREADING = 2002;
int RX_DONECHECK = 2003;
int TX_STOP = 2004;
int TXRX_START = 2005;

int READ_AGC_DONE_ADDR = 3000;
int READ_AGC_GAINS = 3010;
int AGC_RESET = 3020;
int SET_AGC_TARGET_dBm = 3030;
int SET_AGC_NOISEEST_dBm = 3040;
int SET_AGC_THRESHOLDS = 3050;
int READ_AGC_THRESHOLDS = 3060;
int SET_AGC_TRIG_DELAY = 3070;
int SET_AGC_DCO_EN_DIS = 3080;
int READ_RADIO1AGCDONERSSI = 3090;
int READ_RADIO2AGCDONERSSI = 3091;
int READ_RADIO3AGCDONERSSI = 3092;
int READ_RADIO4AGCDONERSSI = 3093;

// int  CAPT_OFFSET = 4001;
int TX_DELAY = 4001;
int TX_LENGTH = 4002;
int TX_MODE = 4003;
int CARRIER_CHANNEL = 4004;
int RADIO1_TXGAINS = 4005;
int RADIO1_RXGAINS = 4006;
int RADIO2_TXGAINS = 4007;
int RADIO2_RXGAINS = 4008;
int RADIO3_TXGAINS = 4009;
int RADIO3_RXGAINS = 4010;
int RADIO4_TXGAINS = 4011;
int RADIO4_RXGAINS = 4012;
int MGC_AGC_SEL = 4013;
int TX_LPF_CORN_FREQ = 4014;
int RX_LPF_CORN_FREQ = 4015;


int TX_TEST = 6000;
int RX1BUFFERS_DEBUG = 6001;
int RX2BUFFERS_DEBUG = 6002;
int RX3BUFFERS_DEBUG = 6003;
int RX4BUFFERS_DEBUG = 6004;

int LW_MODE = 8070;
int LW_THRESHOLD = 8071;

int CARRIER_CHANNEL_R2 = 8072;
int CARRIER_CHANNEL_R3 = 8073;

int LW_FIG_ROW_LENGTH = 8074;
int LW_NUM_OF_FFT = 8075;
int LW_PIX_REPEAT = 8076; 
int LW_TX_IFFT_EN = 8077;
int LW_TX_FILTER_EN = 8078;
int LW_TXFILTER_BUFF = 8079;
int LW_NUM_OF_IDLE_LINES = 8080;
int RADIO2_TXDATA_2 = 8081;

int CLOSE = 99999;


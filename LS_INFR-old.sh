#!/bin/bash
# -------------------- System: Global Variables ----------------------- #
BEOF_DIR="/users/dkulenka/BEOF"

HOST_EC="10.11.16.16"						# EC host IP address
PORT_EC=8800							# EC port number
PORT_RC=8800							# RC port number
EC_RECV_SOCK_TIMEOUT=5						# Maximum time (in seconds) the EC will wait for RC results during exec_WR call
RC_START_RETRIES=3						# Maximum number of RC connection retries incase of failure
SYNC_CLOCKS=true						# true = turn ON, false = turn OFF synchronization
OFM_MAX=0.001							# Maximum Offset From Master (OFM) in seconds
WRL_DRV=false							# true = install, false = do not install wireless driver
VERBOSE=true							# true = turn ON, false = turn OFF verbose output
GRAPH_path="http://10.11.31.6/CREW_BM/GRAPH/GRAPH.html"		# Graphing tool path
OEMV_path="http://10.11.31.6/CREW_BM/OEMV/OEMV.html"		# Orchestration and Error Message Viewer (OEMV) path

source $BEOF_DIR/global_var.sh
# -------------------- User section: Global Variables ----------------- #
CTRL_IP_SUBNET="10.11.16"
WIFI_IP_SUBNET="192.168.0"

SPEAKER_node="38"
#LISTENER_nodes=( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 22 23 24 25 26 27 28 29 30 31 33 34 35 36 37 38 39 40 41 42 )
#SNIFFER_nodes=(  1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 22 23 24 25 26 27 28 29 30 31 33 34 35 36 37 38 39 40 41 42 55 )
LISTENER_nodes=( 7 17 27 28 37 )
SNIFFER_nodes=( 7 17 27 28 37 38 )
#LISTENER_nodes=( 2 3 4 5 6 7 8 10 11 14 15 16 17 18 19 20 22 23 24 25 26 27 28 29 30 33 35 36 37 38 40 42 )
#SNIFFER_nodes=(  2 3 4 5 6 7 8 10 11 14 15 16 17 18 19 20 22 23 24 25 26 27 28 29 30 33 35 36 37 38 40 42 55 )
#LISTENER_nodes=( 1 3 )
#SNIFFER_nodes=(  1 3 55 )
INTRF_nodes=( 2 )

LISTENER_CTRL_IPs=$(IFS=, eval echo '"${LISTENER_nodes[*]/#/${CTRL_IP_SUBNET}.}"')
LISTENER_WIFI_IPs=$(IFS=, eval echo '"${LISTENER_nodes[*]/#/${WIFI_IP_SUBNET}.}"')
SNIFFER_CTRL_IPs=$(IFS=, eval echo '"${SNIFFER_nodes[*]/#/${CTRL_IP_SUBNET}.}"')
INTRF_CTRL_IPs=$(IFS=, eval echo '"${INTRF_nodes[*]/#/10.11.17.}"')

declare -A GROUP_SPEAKER=(
	[IF]="wlan0"
	[MODE]="master"
	[CONFIG]="$BEOF_DIR/config/hostapd/OPEN.conf"
	[TX_POWER]="16"
	[IF_IP]="${WIFI_IP_SUBNET}.$SPEAKER_node"
	[CTRL_IP]="${CTRL_IP_SUBNET}.$SPEAKER_node"
	[MCT_ADDR]=$(eval echo $RAND_MCT_ADDR)
)

declare -A GROUP_LISTENERs=(
	[IF]="wlan0"
	[MODE]="managed"
	[CONFIG]="$BEOF_DIR/config/wpa_supplicant/OPEN.conf"
	[TX_POWER]="16"
	[IF_IP]=$LISTENER_WIFI_IPs
	[CTRL_IP]=$LISTENER_CTRL_IPs
	[MCT_ADDR]=$(eval echo $RAND_MCT_ADDR)
)

declare -A GROUP_SNIFFERs=(
	[IF]="mon0"
	[MODE]="monitor"
	[CTRL_IP]=$SNIFFER_CTRL_IPs
	[MCT_ADDR]=$(eval echo $RAND_MCT_ADDR)
)

declare -A GROUP_INTRFs=(
	[CTRL_IP]=$INTRF_CTRL_IPs
	[MCT_ADDR]=$(eval echo $RAND_MCT_ADDR)
)

SPEEX_STREAMER_EXEC="$BEOF_DIR/exec/rtp_speex_streamer"
OPUS_STREAMER_EXEC="$BEOF_DIR/exec/rtp_opus_streamer"
PL_JT_LAT_MOS_EXEC="$BEOF_DIR/exec/PL_JT_LAT_MOS_DGRAM"
PL_JT_LAT_MOS_PARSER="$BEOF_DIR/exec/PL_JT_LAT_MOS_parser"
WIFI_SNIFFER_EXEC="$BEOF_DIR/exec/wifi_sniffer"
MICROWAVE_EXEC="$BEOF_DIR/exec/microwave"
EI_PARSER="$BEOF_DIR/exec/EI_parser"

AUDIO_IN_PATH="$BEOF_DIR/exec/audio_sample/audio_29sec.wav"
MOS_orig=4.75

SPEAKER_DST_ADDR="235.5.5.5"
SPEAKER_DST_SN="235.5.5.0/24"

# database parameters
HOST="10.11.31.5"
USER="CREW_BM"
PASSWD="CREW_BM"
DB="benchmarking"

LA_path="LA.txt"						# Locating Array file path
#LA_path="LA_outlier.txt"					# Locating Array outlier file path
#LA_path="SSD34.txt"						# Super Saturated Design 34 file path
#LA_path="SSD51.txt"						# Super Saturated Design 51 file path
#LA_path="LA_EXH_A.txt"						# Locating Array Exhaustive Search 1st fraction file path
#LA_path="LA_EXH_B.txt"						# Locating Array Exhaustive Search 2nd fraction file path
#LA_path="LA_EXH_C.txt"						# Locating Array Exhaustive Search 3rd fraction file path

enc_par_path="/tmp/enc_par"					# Shared encoding parameter file path
PKT_STAT=0							# Do not show packet statistics
MRMT_WINDOW=30000000						# 30 sec measurment window
IDLE_WINDOW=10000000						# 10 sec idle window
CablelossRx=10							# 10dBm attunuator attached to Wi-Fi interface

# -------------------- System: Function Definition -------------------- #
source $BEOF_DIR/func_def.sh

# -------------------- User section: Function Definition -------------- #
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ #
# Code inside sig_handler() function is executed up on [SIGHUP, SIGINT, SIGQUIT, SIGABRT, SIGKILL] system signals #
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ #
sig_handler()
{
	exec_WR ${GROUP_LISTENERs[MCT_ADDR]} "killall -q PL_JT_LAT_MOS_DGRAM"
	exec_WR ${GROUP_SPEAKER[MCT_ADDR]}   "killall -q rtp_speex_streamer && killall -q rtp_opus_streamer"
	exec_WR ${GROUP_SNIFFERs[MCT_ADDR]}  "killall -q wifi_sniffer && killall -q dd"
	exec_WR ${GROUP_INTRFs[MCT_ADDR]}    "killall -q microwave"
}

# -------------------- System: Main Code ------------------------------ #
source $BEOF_DIR/wireless_conf.sh
source $BEOF_DIR/pre_main_code.sh
# -------------------- User section: Main Code ------------------------ #

# Retrieve SPEAKER MAC address
SPEAKER_MAC_str=$(exec_WR ${GROUP_SPEAKER[CTRL_IP]}	"cat /sys/class/net/${GROUP_SPEAKER[IF]}/address" | cut -d# -f 2-)

# Retrieve comma separated list of LISTENER MAC addresses for PL_JT_LAT_MOS calculation
LISTENER_MAC_str=$(exec_WR ${GROUP_LISTENERs[MCT_ADDR]}	"cat /sys/class/net/${GROUP_SNIFFERs[IF]}/address" | cut -d# -f 2- | tr '\n' ',' | sed -e 's/,$//')

# SNIFFER MAC addresses =  SPEAKER MAC address + LISTENER MAC addresses
SNIFFER_MAC_str=$(printf "${SPEAKER_MAC_str},${LISTENER_MAC_str}")

# Start sniffer application
exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "$WIFI_SNIFFER_EXEC  -n $HOST -u $USER -k $PASSWD -d $DB -f int:${GROUP_SNIFFERs[IF]} -s $PKT_STAT"

# Start Packet Loss (PL), Jitter (JT), Latency (LAT) and Mean Opinion Score (MOS) calculating program
exec_WOR ${GROUP_LISTENERs[MCT_ADDR]} "$PL_JT_LAT_MOS_EXEC -n $HOST -u $USER -k $PASSWD -d $DB -f ${GROUP_LISTENERs[IF]} -g $SPEAKER_DST_ADDR -a WIDE-BAND"

# Create result storing directory and populate header text inside all files
mkdir -p "$BEOF_DIR/tmp/$EXPR_ID"
IFS=',' read -ra SNIFFER_MACs <<< "$SNIFFER_MAC_str"
for (( i = 0 ; i < ${#SNIFFER_MACs[@]} ; i++ )) do
	if [[ "$SPEAKER_MAC_str" == "${SNIFFER_MACs[$i]}" ]]; then
		printf "band\tchannel\ttxRate\ttxPower\tMTU\ttxQueueLen\tqDisc\tipfrag_low_thresh\tipfrag_high_thresh\tudp_rmem_min\trmem_default\trmem_max\tudp_wmem_min\twmem_default\twmem_max\tudp_mem_min\tudp_mem_pressure\tudp_mem_max\tROHC\tcodecType\tcodecBitRate\tframeLen\tintCOR\tintPeriod\tbgSensing\tEI_UL\tEI_DL_SUT\tEI_DL_BG\n" > "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
	else
		printf "band\tchannel\ttxRate\ttxPower\tMTU\ttxQueueLen\tqDisc\tipfrag_low_thresh\tipfrag_high_thresh\tudp_rmem_min\trmem_default\trmem_max\tudp_wmem_min\twmem_default\twmem_max\tudp_mem_min\tudp_mem_pressure\tudp_mem_max\tROHC\tcodecType\tcodecBitRate\tframeLen\tintCOR\tintPeriod\tbgSensing\tEI_UL\tEI_DL_SUT\tEI_DL_BG\tPacketLoss\tJitter\tLatency\tMOS\n" > "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
	fi
done

# Retrieve Locating Array list and conduct experiment on each row of factors
LA_CNT=1
echo "1" > /tmp/LA_CNT.log
while IFS=$'\t' read -r -a parameters
do
	LA_CNT_prev=$(cat /tmp/LA_CNT.log)
	if [[ $LA_CNT -lt $LA_CNT_prev ]]; then
		LA_CNT=$((LA_CNT+1))
		continue
	fi

	# ---------------------------- Retrieve LA parameters ---------------------------- #
	band=${parameters[0]}
	channel=${parameters[1]}
	txRate=${parameters[2]}
	txPower=${parameters[3]}
	MTU=${parameters[4]}
	txQueueLen=${parameters[5]}
	qDisc=${parameters[6]}
	ipfrag_low_thresh=${parameters[7]}
	ipfrag_high_thresh=${parameters[8]}
	udp_rmem_min=${parameters[9]}
	rmem_default=${parameters[10]}
	rmem_max=${parameters[11]}
	udp_wmem_min=${parameters[12]}
	wmem_default=${parameters[13]}
	wmem_max=${parameters[14]}
	udp_mem_min=${parameters[15]}
	udp_mem_pressure=${parameters[16]}
	udp_mem_max=${parameters[17]}
	ROHC=${parameters[18]}
	codecType=${parameters[19]}
	codecBitRate=${parameters[20]}
	frameLen=${parameters[21]}
	intCOR=${parameters[22]}
#	intPeriod=${parameters[23]}
	intPeriod=4000
	bgSensing=${parameters[24]}

	# ------------------------------ Reset LA parameters ----------------------------- #
	# Kill running applications
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}	"killall -q rtp_opus_streamer"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}	"killall -q rtp_speex_streamer"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}	"killall -q hostapd"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}	"killall -q wpa_supplicant"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}	"killall -q dd"
	exec_WOR ${GROUP_LISTENERs[MCT_ADDR]}	"killall -q PL_JT_LAT_MOS"

	# Bring interface down, clear cache and bring the interface up
	exec_WR ${GROUP_SNIFFERs[MCT_ADDR]} "ifconfig ${GROUP_SNIFFERs[IF]} down"
	exec_WR ${GROUP_SNIFFERs[MCT_ADDR]} "sync; echo 3 > /proc/sys/vm/drop_caches"
	exec_WR ${GROUP_SNIFFERs[MCT_ADDR]} "ifconfig ${GROUP_SNIFFERs[IF]} up"

	# ------------------------------ Apply LA parameters ----------------------------- #
	# Update Sniffers' and AP's channel
	if [[ "$band" == "2.4" ]]; then
		exec_WR ${GROUP_SPEAKER[MCT_ADDR]} "sed -i -e 's/channel=.*/channel=$channel/g' -e 's/hw_mode=.*/hw_mode=g/g' /tmp/hostapd.${GROUP_SPEAKER[IF]}.conf"
	elif [[ "$band" == "5" ]]; then
		exec_WR ${GROUP_SPEAKER[MCT_ADDR]} "sed -i -e 's/channel=.*/channel=$channel/g' -e 's/hw_mode=.*/hw_mode=a/g' /tmp/hostapd.${GROUP_SPEAKER[IF]}.conf"
	fi
	exec_WR ${GROUP_SPEAKER[MCT_ADDR]} "sed -i -e 's/supported_rates=.*/supported_rates=$(($txRate*10))/g' -e 's/basic_rates=.*/basic_rates=$(($txRate*10))/g' /tmp/hostapd.${GROUP_SPEAKER[IF]}.conf"

	# Restart hostapd
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]} "iw dev ${GROUP_SPEAKER[IF]} set txpower fixed 1600"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]} "hostapd -B /tmp/hostapd.${GROUP_SPEAKER[IF]}.conf"

	# Connect LISTENERs to AP
	exec_WOR ${GROUP_LISTENERs[MCT_ADDR]} "iw dev ${GROUP_LISTENERs[IF]} set txpower fixed 1600"
	exec_WOR ${GROUP_LISTENERs[MCT_ADDR]} "wpa_supplicant -B -c /tmp/wpa_supplicant.${GROUP_LISTENERs[IF]}.conf -i ${GROUP_LISTENERs[IF]}"

	printf "\n"
	# Wait until all LISTENER nodes are connected to the AP
	for (( i = 0 ; i < ${#LISTENER_nodes[@]} ; i++ )) do

		printf "INFO: ${CTRL_IP_SUBNET}.${LISTENER_nodes[$i]} connecting to AP ... "
		status=$(exec_WR "${CTRL_IP_SUBNET}.${LISTENER_nodes[$i]}" "wpa_cli -i ${GROUP_LISTENERs[IF]} status | grep wpa_state" | cut -d= -f 2)
		until [[ $status == "COMPLETED" ]]; do
			sleep 1
			status=$(exec_WR "${CTRL_IP_SUBNET}.${LISTENER_nodes[$i]}" "wpa_cli -i ${GROUP_LISTENERs[IF]} status | grep wpa_state" | cut -d= -f 2)
		done
		printf "Finished\n"
	done

	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "iw dev ${GROUP_SPEAKER[IF]} set txpower fixed $(($txPower*100))"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "ifconfig ${GROUP_SPEAKER[IF]} mtu $MTU"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "ifconfig ${GROUP_SPEAKER[IF]} txqueuelen $txQueueLen"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "tc qdisc replace dev ${GROUP_SPEAKER[IF]}   root $qDisc"
	exec_WOR ${GROUP_LISTENERs[MCT_ADDR]} "tc qdisc replace dev ${GROUP_LISTENERs[IF]} root $qDisc"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "sysctl -w net.ipv4.ipfrag_low_thresh=$ipfrag_low_thresh"
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]}   "sysctl -w net.ipv4.ipfrag_high_thresh=$ipfrag_high_thresh"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.ipv4.udp_rmem_min=$udp_rmem_min"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.core.rmem_default=$rmem_default"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.core.rmem_max=$rmem_max"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.ipv4.udp_wmem_min=$udp_wmem_min"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.core.wmem_default=$wmem_default"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.core.wmem_max=$wmem_max"
	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]}  "sysctl -w net.ipv4.udp_mem=\"$udp_mem_min $udp_mem_pressure $udp_mem_max\""

	# ROHC
	# TODO

	# Speaker audio streaming
	# Add routing table for the speaker destination subnet
	exec_WOR ${GROUP_SPEAKER[MCT_ADDR]} "route add -net $SPEAKER_DST_SN dev ${GROUP_SPEAKER[IF]}"

	# Start speex RTP_streamer
	if [[ "$codecType" == "speex" ]]; then
		exec_WOR ${GROUP_SPEAKER[MCT_ADDR]} "$SPEEX_STREAMER_EXEC -a $SPEAKER_DST_ADDR -f $AUDIO_IN_PATH -r $codecBitRate -l $frameLen -P $enc_par_path"
	# Start opus RTP_streamer
	elif [[ "$codecType" == "opus" ]]; then
		exec_WOR ${GROUP_SPEAKER[MCT_ADDR]} "$OPUS_STREAMER_EXEC  -a $SPEAKER_DST_ADDR -f $AUDIO_IN_PATH -r $codecBitRate -l $frameLen -P $enc_par_path"
	fi

	# Interference COR
	if [[ "$band" == "2.4" ]]; then
		exec_WOR ${GROUP_INTRFs[MCT_ADDR]} "$MICROWAVE_EXEC -b 3 -r 35 -p $intPeriod -f $(echo "($intCOR*$intPeriod)/1"| bc) -c $channel"
	elif [[ "$band" == "5" ]]; then
		# Convert 5GHz channels to consequetive 2.4GHz style (i.e. CH36 => CH15, CH40 => CH16, CH44 => CH17, CH48 => CH18, ...)
		exec_WOR ${GROUP_INTRFs[MCT_ADDR]} "$MICROWAVE_EXEC -b 3 -r 35 -p $intPeriod -f $(echo "($intCOR*$intPeriod)/1"| bc) -c $(echo "$channel/4 + 6" | bc)"
	fi

	# Interference period
	# TODO

	# Background sensing
	if [[ "$bgSensing" == "1" ]]; then
		exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]} "dd if=/dev/zero of=/dev/null"
	fi

	# ------------------------- Display LA parameters ------------------------ #
	printf "\n"
	printf "Locating Array #$LA_CNT\n"
	printf "\n"
	printf "Factor			value\n"
	printf "======			=====\n"
	printf "band			$band Ghz\n"
	printf "channel			$channel\n"
	printf "txRate			$txRate Mbps\n"
	printf "txPower			$txPower dBm\n"
	printf "MTU			$MTU bytes\n"
	printf "txQueueLen		$txQueueLen bytes\n"
	printf "qDisc			$qDisc\n"
	printf "ipfrag_low_thresh	$ipfrag_low_thresh bytes\n"
	printf "ipfrag_high_thresh	$ipfrag_high_thresh bytes\n"
	printf "udp_rmem_min		$udp_rmem_min bytes\n"
	printf "rmem_default		$rmem_default bytes\n"
	printf "rmem_max		$rmem_max bytes\n"
	printf "udp_wmem_min		$udp_wmem_min bytes\n"
	printf "wmem_default		$wmem_default bytes\n"
	printf "wmem_max		$wmem_max bytes\n"
	printf "udp_mem_min		$udp_mem_min bytes\n"
	printf "udp_mem_pressure	$udp_mem_pressure bytes\n"
	printf "udp_mem_max		$udp_mem_max bytes\n"
	printf "codecType		$codecType\n"
	printf "codecBitRate		$codecBitRate bps\n"
	printf "frameLen		$frameLen msec\n"
	printf "intCOR			$intCOR\n"
	printf "bgSensing		$bgSensing\n"

	# Wait 10sec to avoid the effect of inter-iteration interference
	printf "\nWaiting 10sec ... "
	sleep $(echo "$IDLE_WINDOW/1000000" | bc -l)
	printf "FINISHED\n\n"

	# -------------------------------------------------------------------------------- #
	# ----------------------------- Measurment collection ---------------------------- #
	# -------------------------------------------------------------------------------- #

	# Start dumping trace
#	exec_WOR ${GROUP_SNIFFERs[MCT_ADDR]} "tcpdump -i ${GROUP_SNIFFERs[IF]} -w /root/trace${LA_CNT}.pcap"
	# Empty wifi_sniffer and PL_JT_LAT_MOS database tables

	mysql -h $HOST --user=$USER --password=$PASSWD -Bse 'TRUNCATE wifi_sniffer' $DB
	mysql -h $HOST --user=$USER --password=$PASSWD -Bse 'TRUNCATE PL_JT_LAT_MOS' $DB

	printf "Measurment collection started for 30sec ... "
	sleep $(echo "$MRMT_WINDOW/1000000" | bc -l)
	printf "FINISHED\n\n"

	# Stop dumping trace
#	exec_WR ${GROUP_SNIFFERs[MCT_ADDR]} "killall -q tcpdump"

	# Stop microwave interference
	exec_WR ${GROUP_INTRFs[MCT_ADDR]} "killall -q microwave"

	# -------------------------------------------------------------------------------- #
	# ---------------------------- Performance calculation --------------------------- #
	# -------------------------------------------------------------------------------- #
	MAC=()
	EI_UL=()
	EI_DL_SUT=()
	EI_DL_BG=()
	PL=()
	JT=()
	LAT=()
	MOS=()
	# Parse/store EI and PL_JT_LAT_MOS metrics
	EI_parser_str=$(eval $EI_PARSER -n $HOST -u $USER -k $PASSWD -d $DB -m $SNIFFER_MAC_str -t $MRMT_WINDOW -l $CablelossRx  $REDIR_STDERR_TO_LOGFILE)
	PL_JT_LAT_MOS_PARSER_str=$(eval $PL_JT_LAT_MOS_PARSER -n $HOST -u $USER -k $PASSWD -d $DB -m $LISTENER_MAC_str -t $MRMT_WINDOW  $REDIR_STDERR_TO_LOGFILE)

	readarray EI_parser_array < <(printf "$EI_parser_str")
	for (( i = 0 ; i < ${#EI_parser_array[@]} ; i++ )) do
		EI_array=( ${EI_parser_array[i]} )
		MAC[i]=${EI_array[0]}
		EI_UL[i]=${EI_array[1]}
		EI_DL_SUT[i]=${EI_array[2]}
		EI_DL_BG[i]=${EI_array[3]}
	done

	readarray PL_JT_LAT_MOS_PARSER_array < <(printf "$PL_JT_LAT_MOS_PARSER_str")
	for (( i = 0 ; i < ${#PL_JT_LAT_MOS_PARSER_array[@]} ; i++ )) do
		PL_JT_LAT_MOS_array=( ${PL_JT_LAT_MOS_PARSER_array[i]} )

		MAC_INDEX=$(arrayIndex ${PL_JT_LAT_MOS_array[0]} MAC)
		if [[ $MAC_INDEX -eq -1 ]]; then
			MAC_INDEX=${#MAC[@]}
			MAC[MAC_INDEX]=${PL_JT_LAT_MOS_array[0]}
		fi
		PL[MAC_INDEX]=${PL_JT_LAT_MOS_array[1]}
		JT[MAC_INDEX]=${PL_JT_LAT_MOS_array[2]}
		LAT[MAC_INDEX]=${PL_JT_LAT_MOS_array[3]}
		MOS[MAC_INDEX]=$(echo "${PL_JT_LAT_MOS_array[4]} * ($MOS_orig - 1) + 1" | bc -l)
	done

	# Performance metrics storage
	printf "MAC\t\t\tEI_UL\t\tEI_DL_SUT\tEI_DL_BG\tPL\tJitter\t\tLatency\t\tMOS\n"
	printf "===\t\t\t=====\t\t=========\t========\t==\t======\t\t=======\t\t===\n"
	for (( i = 0 ; i < ${#SNIFFER_MACs[@]} ; i++ )) do

		printf "$band\t$channel\t$txRate\t$txPower\t$MTU\t$txQueueLen\t$qDisc\t$ipfrag_low_thresh\t$ipfrag_high_thresh\t$udp_rmem_min\t$rmem_default\t$rmem_max\t$udp_wmem_min\t$wmem_default\t$wmem_max\t$udp_mem_min\t$udp_mem_pressure\t$udp_mem_max\t$ROHC\t$codecType\t$codecBitRate\t$frameLen\t$intCOR\t$intPeriod\t$bgSensing" >> "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"

		# If current sniffer MAC exists with in result MACs
		if $(elementExists ${SNIFFER_MACs[$i]} MAC) ; then

			# Retrieve result MAC address index
			MAC_INDEX=$(arrayIndex ${SNIFFER_MACs[$i]} MAC)

			if [ -v ${MOS[$MAC_INDEX]} ]; then
				printf "\t%e\t%e\t%e\n"						${EI_UL[$MAC_INDEX]} ${EI_DL_SUT[$MAC_INDEX]} ${EI_DL_BG[$MAC_INDEX]} >> "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
				printf "${MAC[$MAC_INDEX]}\t%e\t%e\t%e\n" 			${EI_UL[$MAC_INDEX]} ${EI_DL_SUT[$MAC_INDEX]} ${EI_DL_BG[$MAC_INDEX]}
			elif [ -v ${EI_UL[$MAC_INDEX]} ]; then
				printf "\t\t\t\t%.0f\t%e\t%e\t%e\n"				${PL[$MAC_INDEX]} ${JT[$MAC_INDEX]} ${LAT[$MAC_INDEX]} ${MOS[$MAC_INDEX]} >> "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
				printf "${MAC[$MAC_INDEX]}\t\t\t\t%.0f\t%e\t%e\t%e\n"		${PL[$MAC_INDEX]} ${JT[$MAC_INDEX]} ${LAT[$MAC_INDEX]} ${MOS[$MAC_INDEX]}
			else
				printf "\t%e\t%e\t%e\t%.0f\t%e\t%e\t%e\n"			${EI_UL[$MAC_INDEX]} ${EI_DL_SUT[$MAC_INDEX]} ${EI_DL_BG[$MAC_INDEX]} ${PL[$MAC_INDEX]} ${JT[$MAC_INDEX]} ${LAT[$MAC_INDEX]} ${MOS[$MAC_INDEX]} >> "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
				printf "${MAC[$MAC_INDEX]}\t%e\t%e\t%e\t%.0f\t%e\t%e\t%e\n"	${EI_UL[$MAC_INDEX]} ${EI_DL_SUT[$MAC_INDEX]} ${EI_DL_BG[$MAC_INDEX]} ${PL[$MAC_INDEX]} ${JT[$MAC_INDEX]} ${LAT[$MAC_INDEX]} ${MOS[$MAC_INDEX]}
			fi
		# Else only store the input parameters
		else
			printf "\n" >> "$BEOF_DIR/tmp/$EXPR_ID/${SNIFFER_MACs[$i]}.txt"
			printf "${SNIFFER_MACs[$i]}\n"
		fi
	done

	# Increment LA factor count
	LA_CNT=$((LA_CNT+1))

done < $LA_path

exec_WR ${GROUP_LISTENERs[MCT_ADDR]} "killall -q PL_JT_LAT_MOS_DGRAM"
exec_WR ${GROUP_SPEAKER[MCT_ADDR]}   "killall -q rtp_speex_streamer && killall -q rtp_opus_streamer"
exec_WR ${GROUP_SNIFFERs[MCT_ADDR]}  "killall -q wifi_sniffer && killall -q dd"
exec_WR ${GROUP_INTRFs[MCT_ADDR]}    "killall -q microwave"

echo "INFO: Experiment Finished"

# -------------------- System: Main Code ------------------------------ #
source $BEOF_DIR/post_main_code.sh


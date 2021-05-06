#!/usr/bin/env bash

APPS_PATH="/groups/wall2-ilabt-iminds-be/react/wirelessConfExp/LA_INFR_apps/"
BEOF_PATH="/groups/wall2-ilabt-iminds-be/react/wirelessConfExp/BEOF"

cd "$APPS_PATH/microwave"
make

cd "$APPS_PATH/rtp_streamer"
make

cd "$APPS_PATH/PL_JT_LAT_MOS"
make

cd "$APPS_PATH/wifi_sniffer"
make

cd "$BEOF_PATH/exec/EC"
make

cd "$BEOF_PATH/exec/RC"
make; 



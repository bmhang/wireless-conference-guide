#!/usr/bin/env bash

cd ~
git clone https://github.com/mmehari/BEOF.git
cd BEOF/exec/EC
sudo make clean
sudo make
cd ~/BEOF/exec/RC
sudo make clean
sudo make 

EXP_FILE_DIR="/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp"
EXP_DIR="/users/bhang/BEOF/expr_descr"
APPS_DIR="/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp/LA_INFR_apps"
EXEC_DIR="/users/bhang/BEOF/exec"

cp $APPS_DIR/microwave/microwave $EXEC_DIR
cp $APPS_DIR/PL_JT_LAT_MOS/PL_JT_LAT_MOS_DGRAM $EXEC_DIR
cp $APPS_DIR/PL_JT_LAT_MOS/PL_JT_LAT_MOS_parser $EXEC_DIR
cp $APPS_DIR/rtp_streamer/rtp_opus_streamer $EXEC_DIR
cp $APPS_DIR/rtp_streamer/rtp_speex_streamer $EXEC_DIR
cp $APPS_DIR/wifi_sniffer/wifi_sniffer $EXEC_DIR
cp $APPS_DIR/wifi_sniffer/EI_parser $EXEC_DIR

mkdir $EXEC_DIR/audio_sample
cp $APPS_DIR/audio_29sec.wav $EXEC_DIR/audio_sample

cp $EXP_FILE_DIR/LA_INFR.sh $EXP_DIR
cp $EXP_FILE_DIR/LA.txt $EXP_DIR

sudo chmod +x $EXP_FILE_DIR/LA_INFR.sh

#sudo apt-get -y install nfs-kernel-server --fix-missing

printf "/users/bhang/BEOF/exec *(rw,sync,no_root_squash,no_subtree_check)\n"  | sudo tee -a /etc/exports
printf "/users/bhang/BEOF/config *(rw,sync,no_root_squash,no_subtree_check)\n" | sudo tee -a /etc/exports
printf "/users/bhang/BEOF/tmp *(rw,sync,no_root_squash,no_subtree_check)\n" | sudo tee -a /etc/exports

sudo exportfs -rav
sudo service nfs-kernel-server restart

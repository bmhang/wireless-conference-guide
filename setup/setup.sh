#!/usr/bin/env bash

SETUP_PATH="/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp/setup"
BEOF_PATH="/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp/BEOF"

# source $SETUP_PATH/install-docker.sh
source $SETUP_PATH/install-dependencies.sh
#source $SETUP_PATH/make-libs.sh
cd $SETUP_PATH
# sudo docker build -t beof -f Dockerfile_BEOF .
cd $BEOF_PATH/exec/RC
sudo make install
sudo service ntp stop

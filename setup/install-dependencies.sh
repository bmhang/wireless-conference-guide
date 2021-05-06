#!/usr/bin/env bash

sudo apt-get update
sudo apt-get -y install libopus-dev libspeex-dev libmysqlclient-dev

sudo apt-get -y install nfs-kernel-server
#sudo echo "/groups/wall2-ilabt-iminds-be/react/BEOF/tmp *(rw,sync,no_root_squash,no_subtree_check)" >> /etc/exports
#sudo echo "/groups/wall2-ilabt-iminds-be/react/BEOF/config *(rw,sync,no_root_squash,no_subtree_check)" >> /etc/exports
#exportfs -rav
#service nfs-kernel-server restart






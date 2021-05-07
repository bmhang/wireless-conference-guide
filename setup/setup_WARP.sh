#!/usr/bin/env bash

IMPACT_PATH = "/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp/Xilinx3/14.7/ISE_DS/ISE/bin/lin64/impact"
IMAGE_PATH = "/groups/ilabt-imec-be/asu-conf-la/wirelessConfExp/WARP_images/WARP2_80M_tx.txt"

$IMPACT_PATH -batch $IMAGE_PATH

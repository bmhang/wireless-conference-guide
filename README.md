# Wireless Conferencing Experiments with BEOF

This repository contains the necessary code to perform a locating-array based wireless conferencing experiment on the w.iLab.t wireless testbed using the Bash Experiment Orchestration Framework (BEOF). The research behind the experiment described here first appeared in [1].

## Getting Started

### Introduction

Screening experiments are an important first step in determining which factors should be included in implementation, and which ones can be safely eliminated from consideration. Locating array based screening experiments use the properties of locating arrays to reduce the number of tests needed to discover influential factors and 2-way interactions with many less tests than other traditional methods.

This guide provides the steps necessary to reproduce experiments conducted in [1].

### Prerequisites

This user guide is designed to be used with the w.iLab.t-2 testbed. The Zotac, Warp, and Server type nodes were used.

Requirements:
- w.iLab.t-2 testbed
- jFed Experimenter software (not required, but highly recommended) can be found [here](https://jfed.ilabt.imec.be).
- one Server node
- one Warp node associated with a Zotac node (e.g. ZotacG3/Warp2, ZotacH3/Warp3)
- at least three Zotac nodes
- Ubuntu 14.04 installed on the nodes

The first thing to do is to swap in some nodes on wilab. For more detailed instructions on how to swap in nodes, please refer to the w.iLab.t documentation [here](https://doc.ilabt.imec.be/ilabt/wilab/getting_started.html "here"). In this repository is a file named small_exp41_3.rspec. This file can be opened with jFed Experimenter and has already configured a small topology of nodes to be used. Simply duplicate the Zotac nodes to increase the size of the experiment.

### Xilinx

You have to install Xilinx next, and I didn't do this part before. Daniel, you're welcome to add here as you can remember!

### Setting Up the Conferencing Experiment

Preparing this experiment comes with four distinct phases:
- Set up the Warp node
- Set up a MySQL database on the server node
- Set up the LA_INFR.sh file
- Begin the experiment from a Zotac node.

Each of these phases will now be described in detail.

## References
[1] R. Compton, M. T. Mehari, C. J. Colbourn, E. D. Poorer, I. Moerman, V. R. Syrotiuk, "Identifying Parameters and Interactions that impact Wireless Network Performance Efficiently," Unpublished?, October 2017.

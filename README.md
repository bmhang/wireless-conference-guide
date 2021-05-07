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

The first thing to do is to swap in some nodes on wilab. For more detailed instructions on how to swap in nodes, please refer to the w.iLab.t documentation [here](https://doc.ilabt.imec.be/ilabt/wilab/getting_started.html "here"). In this repository is a file named `small_exp41_3.rspec`. This file can be opened with jFed Experimenter and has already configured a small topology of nodes to be used. Simply duplicate the Zotac nodes to increase the size of the experiment.

### Xilinx iMPACT

The Xilinx iMPACT application is used to program the Warp type software defined radios (SDRs) on the testbed. This guide assumes that the user already has prior knowledge of SDRs and programming the Warp nodes using this application. iMPACT is included in the Xilinx ISE package, which can be downloaded [here](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive-ise.html). We suggest using version 14.7, although others will most likely work as well. If the entire package is not needed, it is possible that the Lab Tools: Standalone package should cover the basic needs of programming the Warp SDR. 

### Clone BEOF

This experiment runs on the Bash Experiment Orchestration Framework. The BEOF repository can be found [here](https://github.com/mmehari/BEOF). Clone the repository into this folder.

## Setting Up the Conferencing Experiment

Preparing this experiment comes with four distinct phases:
- Set up the Warp node
- Set up a MySQL database on the server node
- Set up the `LA_INFR.sh` file
- Begin the experiment from a Zotac node.

We now describe each of the phases in detail. 

### Setting up the Warp Node

To prepare the Warp node, open `setup_WARP.sh`, located inside the `setup` folder.
1. Change the `IMPACT_PATH` on line 3 to be the location of the `impact` application installed for your node.
2. Change the `IMAGE_PATH` on line 4 to be the location where your WARP images are stored. Ensure that you are using the correct Warp image for the Warp node you are using (i.e. for Warp2 you would use the `WARP2_80M_tx.txt` file).
3. Open an SSH terminal into the Zotac node associated with your Warp node. For example, you would SSH into ZotacG3 if you swapped in Warp2. From your Zotac node, execute `setup_WARP.sh` and allow the process to complete.

### Setting up a MySQL Database on the Server Node

This experiment requires a MySQL database to store experiment data before outputting it.

(NEEDS UPDATING) I took very general notes on this, but we're going to have to go through this to make it MUCH more specific.

These commands must be run on the server node you swapped in. In the example file provided, server3 is used.
1. To begin, run `sudo apt-get install mysql-server`. When prompted, set the root account password to something you will remember.
2. Create a new user with all privileges. Make note of these credentials as you will need them later.
3. Edit the `my.cnf` file in `~/etc/mysql` and change the bind address to `0.0.0.0`. This must be done so the database can be accessed remotely.
4. Restart the server with `sudo service mysql restart`.
5. Create a new database called `benchmarking`.
6. Create new tables in this database using the code provided [here](setup/my_sql_setup_code.md). There are 20 tables that need to be created.
7. You can now exit the MySQL interface. 

Setting up a MySQL database can be time consuming, so it is recommended that you create a disk image once you complete this step. This will ensure that the experiment can be swapped in and out without needing to repeat this process. This can be done by right-clicking a node in jFed and selecting "Create Image." Be sure to check the "Save System Users in Image" checkbox. Make sure to use this image the next time you swap in the experiment. 

When loading your MySQL database from the disk image, you may not be able to start the server (`sudo service mysql start`). If this is the case, the following command should be run:

```
sudo adduser --disabled-password --gecos "" mysql && sudo chown mysql:mysql -R /var/lib/mysql && sudo chmod -R 755 /var/lib/mysql/ && sudo service mysql start
```

### Setting Up the `LA_INFR.sh` File

Open an SSH terminal into the Zotac node you want to use as the Experiment Controller. Then open 'LA_INFR.sh'.
1. On line 3, change the directory to your location of the BEOF folder.
2. On line 5, change the IP address to that of the node you want to use as the Experiment Controller (EC). That should be the node you are currently logged into. The IP address can be found by entering `ifconfig -a`. Use the IP address of the first wireless interface.
3. On line 22, edit the `SPEAKER_node` value with the last octet of the IP address of the speaker node (i.e. if the IP address is 127.0.0.25, you would put 25). The speaker node must be one of the Zotac nodes.
4. Edit the `LISTENER_nodes` variable with the last octet (as in step 3) of the IP addresses of the listener nodes. The listener nodes are all Zotac nodes swapped in excluding the speaker node. Put a space between the values of each node.
5. Edit the `SNIFFER_nodes` variable with the last octet (as in steps 3 and 4) of the IP addresses of sniffer nodes. Sniffer nodes are all Zotac nodes (including the speaker node).
6. Edit the `INTRF_nodes` variable with the last octet of the IP address of the server node.
7. Scroll down until you see a comment labeled `database parameters`. Edit the `HOST`, `USER`, `PASSWD`, and `DB` values with the IP address of the server hosting the MySQL database, the username and password for the user you created with all privileges, and the name of the database (which should remain `benchmarking`).

Save these changes. Then execute the `setup_EC.sh` file inside the `setup` folder to set up the experiment controller.

## Executing the Experiment and Retrieving Results

To run the experiment, execute `LA_INFR.sh` on the node you designated the Experiment Controller.

Results will be generated in the home folder of the Experiment Controller, e.g. `~/BEOF/tmp`. There will be a file for every Zotac node in the experiment. Each file will be named based on the hardware address of the node's first wireless interface. Be sure to save these results before swapping the experiment out or they will be lost.

## Other Notes

The ultimate goal for this experiment is to use a large number of nodes (30+) as listener nodes to increase the complexity of the scenario. However, there is a known issue with this experiment that can cause the experiment to hang indefinitely when a large number of Zotac nodes (e.g. >8) are swapped in. For demonstration purposes, please limit the number of listener nodes to eight or less for the experiment to complete successfully. 

## Running Analysis

(NEEDS UPDATING) -- Need to replace the different responses, LAs, and folders with the proper ones for this experiment. We also need to add in a section about parsing the results. We need to add in the "real" locating array, since the ones used here are already converted to factor levels. Additionally, I am unsure how I did the shuffling for the LAs in ``experiment_LAs``, so I will either need to figure that out or write a program to reshuffle them. 

This repository includes the analysis tool for locating array based screening experiments [2]. The tool has several configurable parameters that can be used to affect the model generated. To compile the tool, navigate to the `la-analysis/` folder and run `make`. 

	cd la-analysis
	make

To simplify the use of the analysis tool, we include a `run_analysis.py` script that will automatically generate models with successively increasing number of terms (up to a user specified value), and then select the best one based on when the R-squared for the new model is less than some user-specified value. We also include an `example_run_analysis.py` script that gives an example of how to pass in the parameters for the analysis script. To run the example analysis, simply run:

	python3 example_run_analysis.py

For non-example data, the `run_analysis.py` script takes in 7 required parameters. They are described below:

- `executable_path`: This is the path to the analysis executable. If compiled directly in the repository, it is simply `"la-analysis/Search"`
- `data_path`: This is the path to the folder containing the locating array and factor data, the output folder, and the responses folder. 
- `LA_name`: This is the name of the locating array, without any path preceding it (assuming it is inside the data path folder)
- `FD_name`: This is the name of the factor data file, again without any path preceding it
- `responses_folder`: This is the path to the folder containing the response files, which in the example data here is called `"responses/"`
- `output_folder`: This is the path to the output folder where the model output will go, which in the example data is called `"models/"`
- `responses`: This is one or more column names from the response output file. In this case, we will pass in `Delay Jitter Throughput`, so the analysis will be run for each of the three responses in the response file. 

So, to run the analysis on the example data, we could run the following command for the complete topology:

	python3 run_analysis.py la-analysis/Search example_data/complete/ comp_la.tsv comp_factors.tsv responses/ models/ Delay Jitter Throughput

For more information on how to run the analysis tool directly, without using the helper script, please refer to the command line output or to the README of an earlier version of the analysis here: https://github.com/sseidel16/v4-la-tools.


## References
[1] R. Compton, M. T. Mehari, C. J. Colbourn, E. D. Poorer, I. Moerman, V. R. Syrotiuk, "Identifying Parameters and Interactions that impact Wireless Network Performance Efficiently," Unpublished?, October 2017.
[2] Y. Akhtar, F. Zhang, C. J. Colbourn, J. Stufken, and V. R. Syrotiuk, “Scalable level-wise screening using locating arrays,” Unpublished manuscript, October 2020.

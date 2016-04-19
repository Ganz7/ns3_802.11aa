#!/bin/bash          
echo Running Simulations...

# Values for the parameters
NWIFI=5
TYPE=true   # true for On/Off and false for Poisson
RATE=30

DURATION_ARRAY=( 0.001 0.005 0.01 0.05 0.1 0.5 1 )

# Vary the duration values
for DURATION in 0.001 0.005 0.01 0.05 0.1 0.5 1
do
	# Put each duration's sims in a seperate folder
	FOLDER=$NWIFI\_$TYPE\_$RATE\_$DURATION
	# Create the folder if it does not exist
	mkdir -p $FOLDER

	# Run the simulation code 100 times
	for i in `seq 1 100`;
	do 
		WAFCMD="./waf --run \"examples/wireless/wifi-udp --nWifi=$NWIFI --type=$TYPE --rate=$RATE --duration=$DURATION\" > $FOLDER/$NWIFI\_$TYPE\_$RATE\_$DURATION\_$i.dat"
		eval $WAFCMD
	done
done

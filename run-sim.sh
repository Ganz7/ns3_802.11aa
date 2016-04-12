#!/bin/bash          
echo Running Simulations

# Some default values for the parameters
NWIFI=5
TYPE=0
RATE=10
DURATION=0.1

# If arguments have been supplied
if [ $# -eq 4 ]
	then
		NWIFI=$1
		TYPE=$2
		RATE=$3
		DURATION=$4
fi

#WAFCMD="./waf --run \"examples/wireless/wifi-udp --nWifi=5 --type=0 --rate=10 --duration=0.1\""
WAFCMD="./waf --run \"examples/wireless/wifi-udp --nWifi=$NWIFI --type=$TYPE --rate=$RATE --duration=$DURATION\""

echo `$WAFCMD`

# Run the simulation code 25 times
for i in `seq 1 25`;
	do 
		#echo `$WAFCMD`
		echo $i

		# Update the variables here for the next iteration
		# NWIFI = ...
	done  
#!/bin/bash          
echo Running Simulations...

# Values for the parameters
NWIFI=5
DURATION=0.01


# Do it for On/Off and Poisson
for TYPE in true false
do
	echo "Running for Type - $TYPE"

	# Vary the RATE values
	for RATE in 10 20 30 40 50 60
	do
		# Put each TYPE and RATE's sims in a seperate folder
		FOLDER=$NWIFI\_$TYPE\_$RATE\_$DURATION
		# Create the folder if it does not exist
		mkdir -p $FOLDER

		echo "Running for Duration - $DURATION"

		# Run the simulation code 100 times
		for i in `seq 1 100`;
		do 
			echo "Running Test $i..."
			WAFCMD="./waf --run \"examples/wireless/wifi-udp --nWifi=$NWIFI --type=$TYPE --rate=$RATE --duration=$DURATION\" > $FOLDER/$NWIFI\_$TYPE\_$RATE\_$DURATION\_$i.dat"
			eval $WAFCMD
		done
	done
done

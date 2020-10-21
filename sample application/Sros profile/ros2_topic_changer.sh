#!/bin/bash



if (( $# < 2 ));
  then
    echo "Not enough arguments supplied. To Run: ./sros_profile_generator <newfilename> <derivedFilename>"
    exit 1
fi

USERNAME=$(whoami)
# source /home/$USERNAME/ros2_ws/install/local_setup.bash

command=""

for ((i = 1; i <= $#; i+=4 )); do
  first=${!i}
  x=$(expr $i + 1)
  second=${!x}
  y=$(expr $i + 2)
  third=${!y}
  z=$(expr $i + 3)
  fourth=${!z}
  if [[ (-z "$first") || (-z "$second") || (-z "$third") || (-z "$fourth") ]]
    then
      echo "Error in arguments! Should be in 4s"
      exit 1
  fi
  #echo `ps -ef | grep -i "$second" | grep -v "/bin/bash" | grep -v grep | grep -v "ros2 run" | grep -v "ros2_topic_changer.sh" `
  pid=`ps -ef | grep -i "$second" | grep -v "/bin/bash" | grep -v grep | grep -v "ros2 run" | grep -v "ros2_topic_changer.sh" | awk '{print $2}'`
  if [[ -z "$pid" ]]
    then
      echo "$second process not running or was not able to find its pid"
      exit
  fi
  
  #echo $first $pid $third $fourth 
  
  if [ ${#command} -eq 0 ]; then
	command="$first, $pid, $third, $fourth"
  else
	command="${command}, $first, $pid, $third, $fourth"
  fi
done

echo $command
ros2 run demo_nodes_cpp colonel "$command" 

  

#to get pid
#ps -ef | grep -i "/talker" | grep -v grep | awk '{print $2}'

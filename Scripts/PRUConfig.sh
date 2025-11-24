#!/bin/bash

# install script with this command:
#    sudo install -m 755 Scripts/PRUConfig.sh /usr/local/sbin/

set -euo pipefail

echo "====== Script for PRU configuration ======"
echo "[INFO] Setting Pin MUX..."
config-pin P9_27 pruout

found=false
state_path="/sys/class/remoteproc/remoteproc1/state"

echo "[INFO] Waiting until remoteproc1 is available..."
for i in {1..100}; do
  if [ -e /sys/class/remoteproc/remoteproc1/state ]; then
		current_state=$(cat "$state_path")
	if [ "$current_state" = "running" ]; then
	  echo "[INFO] PRU0 already started. No action needed."
	else
	  echo "[INFO] Starting PRU0..."
	  echo 'start' > "$state_path"
	fi
	found=true
    break
  fi
  sleep 3
done

if [ "$found" = false ]; then
  echo "[ERROR] remoteproc1/state still not available after 300 seconds."
  exit 1
fi


echo "[INFO] PRU Config ready."
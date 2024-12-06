#!/bin/bash
set -e

apt update && apt upgrade -y
apt install mosquitto -y 
service mosquitto stop
mkdir -m 740 -p /run/mosquitto
chown mosquitto /run/mosquitto
service mosquitto start

cd Mobius

npm install
npm run fix_path_to_regexp

node mobius.js

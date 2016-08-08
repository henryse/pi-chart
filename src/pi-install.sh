#!/usr/bin/env bash

# Setup on startup.
#
sudo update-rc.d -f pichart remove
sudo cp pichart.sh /etc/init.d/pichart
sudo chmod 755 /etc/init.d/pichart
sudo update-rc.d pichart defaults

# Setup we don't want to run as sudo all of the time.
#
sudo chown root /usr/local/bin/pi-chart
sudo chmod 4755 /usr/local/bin/pi-chart
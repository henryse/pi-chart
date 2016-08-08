#!/usr/bin/env bash
# /etc/init.d/pi-chart

function log_message {
    echo $1;
    logger -p info $1;
}

if [ true != "$INIT_D_SCRIPT_SOURCED" ] ; then
    set "$0" "$@"; INIT_D_SCRIPT_SOURCED=true . /lib/init/init-d-script
fi

### BEGIN INIT INFO
# Provides:          pi-chart
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: pi-chart script
# Description:       Used to control the pi-chart
### END INIT INFO

process_id=$(ps -e | grep pi-chart | awk '{print $1}')

# The following part carries out specific functions depending on arguments.
case "$1" in
  start)
    log_message "Starting pi-chart...";
    if [ -z "${process_id}" ]; then
        log_message "pi-chart starting up";
        /bin/bash -c "/usr/local/bin/pi-chart --port=8090 --directory=/home/pi/pi-chart/src/www &";
    else
        log_message "pi-chart is already running: ${process_id}";
    fi
    ;;
  stop)
    log_message "Stopping pi-chart...";

    if [ -z "${process_id}" ]; then
        log_message "pi-chart is not running";
    else
        log_message "Killing ${process_id}";
        kill ${process_id}
    fi
    ;;
  *)
    echo "Usage: /etc/init.d/pichart {start|stop}"
    exit 1
    ;;
esac

exit 0

# Author: Henry Seurer <henry@gmail.com>

DESC="pi-chart"
DAEMON=/usr/local/bin/pi-chart

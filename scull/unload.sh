#!/bin/sh

module = "scull"
device = "gzhSCULL"

/sbin/rmmod $module $* || exit 1

rm -r /dev/${device} /dev/${device}[0-3]

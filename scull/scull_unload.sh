#!/bin/sh

module = "gzhSCULL"
device = "gzhSCULL"

/sbin/rmmod $module $* || exit 1

rm -f /dev/${device} /dev/${device}[0-3]
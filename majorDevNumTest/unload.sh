#!/bin/sh

module="num"
device="gzhNUMTest"

/sbin/rmmod $module $* || exit 1

rm -r /dev/${device} /dev/${device}[0-2]

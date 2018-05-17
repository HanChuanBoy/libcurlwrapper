#!/bin/sh
curdir=`pwd`
OUTFILE=${curdir}/a.txt
cat /proc/uptime > ${OUTFILE}
for i in `seq 100`
do
    (date "+%s.%3N" ;  cat /sys/class/thermal/thermal_zone0/temp) |\
    awk '{if (NR%2) date = $1; else print date, $1;}'
done >> ${OUTFILE}
cat /proc/uptime >> ${OUTFILE}
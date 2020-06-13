#!/bin/bash
#$1: path to the log file to process

if [ $# -eq 0 ]; then
    echo "ERRORE: wrong usage of $(basename $0) tool" 1>&2
    echo "Correct usage: $(basename $0) <logFile_path>" 1>&2
    exit -1
fi

if [ -f "$1" ]; then
    #Read all lines
    while read line; do 
        echo $line;
    done < "./logFiles/log_test.txt"
else
    echo "$0:File $1 is not a regular file or it doesn't exist." 1>&2
fi

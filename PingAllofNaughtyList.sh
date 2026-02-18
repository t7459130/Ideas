#!/bin/sh

while read -r host; do
	[  -z "$host"   ] && continue
	echo "Pinging $host..."
	ping -c 4 "$host"
done < NaughtyList


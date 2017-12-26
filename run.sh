#!/usr/bin/env bash

for i in $(seq 0 3)
do
    ./main $i &
done

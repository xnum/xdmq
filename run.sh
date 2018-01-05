#!/usr/bin/env bash

for i in $(seq 0 2)
do
    ./main $i 3 &
done

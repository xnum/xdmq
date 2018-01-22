#!/usr/bin/env bash

begin=$(date +%s%N)
for _ in $(seq 1 4)
do
    ./client 127.0.0.1 $1 &
done

wait

end=$(date +%s%N)
elapsed=$((end - begin))
bc -l <<< "$elapsed / 1000000000"

#!/bin/bash

## usage
## bash trace_gen.sh [workload_name] <-- a,b,c ...

for j in {a,b,c,d,f,g,h,i} 
do
	bin/ycsb load kvtracer -P workloads/workload$j -p "kvtracer.tracefile=tracea_load_$j.txt" -p "kvtracer.keymapfile=tracea_keys_$j.txt"
	bin/ycsb run kvtracer -P workloads/workload$j -p "kvtracer.tracefile=tracea_run_$j.txt" -p "kvtracer.keymapfile=tracea_keys_$j.txt"
done

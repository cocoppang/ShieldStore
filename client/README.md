# ShieldStore client

Client for ShieldStore

## Build
	$ make VAL_SIZE=[SMALL, MEDIUM, LARGE]
	
Value size should be defined.(SMALL 16byte, MEDIUM 128byte, LARGE 512byte)

## Usage
	$ ./mydb_client [port number] [workload_name]

## Workloads

In this sample file, it uses 10Million Set operation for LOAD, and 30Million SET/GET operation for RUN 
with key size: 16Bytes, value size: 16Bytes.



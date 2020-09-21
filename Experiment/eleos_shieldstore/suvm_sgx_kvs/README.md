# Comparison to Eleos 

This is sgx version of key-value store ported to "Eleos: Exit-Less OS Services for SGX Enclaves"
The experiment is for Firgure 16, 17. in the paper "ShieldStore: Shielded In-memory Key-value Storage with SGX"

## Build

In order to build this project, you need to download eleos [here](https://github.com/acsl-technion/eleos). 
After you locate this directory into root, run 

	$ make SGX_MODE=HW SGX_PRERELEASE=1 

## Usage

	$ ./app	

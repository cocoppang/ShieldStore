# ShieldStore

ShieldStore is a prototype of trusted in-memory key-value stores with Intel SGX based on EuroSys 2019 paper.

## Dependencies

### Hardware

For supporting Intel SGX, you should use Intel CPU with at least Haswell generation. 
You can follow the list as below.
[List of SGX supported HW](https://github.com/ayeks/SGX-hardware)

### Software

For the support of Intel SGX, sgx-linux should be required.

Recommanded version is 1.8 for both SDK and Driver. 

* [Intel SGX Linux 1.8 version](https://github.com/intel/linux-sgx/tree/sgx_1.8)
* [Intel SGX Linux driver](https://github.com/intel/linux-sgx-driver/tree/sgx_driver_1.8)

However, ShieldStore is available on Intel SGX SDK version 1.9, and 2.2 along with Intel SGX Driver version 1.9, 2.0, and 2.1.

## Build

	$ make SGX_MODE=HW SGX_PRERELEASE=1 

## Build Environment

Ubuntu 16.04.5

For using tcmalloc in 16.04, we should download a rebuilt version of tcmalloc library from [here](https://01.org/intel-softwareguard-extensions/downloads/intel-sgx-linux-1.8-release) or use libsgx\_tcmalloc.a in this repo.

Replace libsgx\_tcmalloc.a from SDK in the installed path with this version. 

## Usage
	$ ./app	

First, run the server with above command, and run client.

## Paper

For the details, please refer this paper.
"ShieldStore: Shielded In-memory Key-value Storage with SGX" [EuroSys'19](http://calab.kaist.ac.kr:8080/~jhuh/papers/kim_eurosys19_shieldst.pdf)

## Question?

If you have any question, don't hesitate to send an e-mail to [T.Kim](mailto:thkim@calab.kaist.ac.kr).

From T. Kim, J. Park, J. Woo, S. Jeon J. Huh

# ShieldStore

This is secure key-value storage using Intel SGX.

## Dependencies

### Hardware

For supporting Intel SGX, you should use Intel CPU with at least Haswell generation. 
You can follow the list as below.
[List of SGX supported HW](https://github.com/ayeks/SGX-hardware)

### Software

For the support of Intel SGX, sgx-linux should be required.

* [Intel SGX Linux 1.8 version](https://github.com/intel/linux-sgx/tree/sgx_1.8)
* [Intel SGX Linux driver](https://github.com/intel/linux-sgx-driver/tree/sgx_driver_1.8)

## Build

	$ make SGX_MODE=HW SGX_PRERELEASE=1 VAL_SIZE=[SMALL, MEDIUM, LARGE]

## Usange
	$ ./app	

First, run the server with above command, and run client.

## Question?

If you have any question, don't hesitate to send an e-mail to [T.Kim](mailto:thkim@calab.kaist.ac.kr).

From T. Kim, J. Park, J. Woo, S. Jeon J. Huh

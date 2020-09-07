#!/bin/bash

make clean
sed -i 's/PRINT 1024/PRINT 4/' Include/userdef.h
sed -i 's/0x200000000/0x2000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 4/PRINT 8/' Include/userdef.h
sed -i 's/0x2000000/0x4000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 8/PRINT 12/' Include/userdef.h
sed -i 's/0x4000000/0x6000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 12/PRINT 16/' Include/userdef.h
sed -i 's/0x6000000/0x8000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 16/PRINT 24/' Include/userdef.h
sed -i 's/0x8000000/0xc000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 24/PRINT 32/' Include/userdef.h
sed -i 's/0xc000000/0x10000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 32/PRINT 64/' Include/userdef.h
sed -i 's/0x10000000/0x20000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 64/PRINT 128/' Include/userdef.h
sed -i 's/0x20000000/0x40000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 128/PRINT 256/' Include/userdef.h
sed -i 's/0x40000000/0x80000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 256/PRINT 512/' Include/userdef.h
sed -i 's/0x80000000/0x100000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

make clean
sed -i 's/PRINT 512/PRINT 1024/' Include/userdef.h
sed -i 's/0x100000000/0x200000000/' Enclave/Enclave.config.xml
make SGX_MODE=HW SGX_DEBUG=1
./app

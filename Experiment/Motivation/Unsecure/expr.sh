#!/bin/bash

make clean
sed -i 's/PRINT 1024/PRINT 4/' App.h
make
./app

make clean
sed -i 's/PRINT 4/PRINT 8/' App.h
make
./app

make clean
sed -i 's/PRINT 8/PRINT 12/' App.h
make
./app

make clean
sed -i 's/PRINT 12/PRINT 16/' App.h
make
./app

make clean
sed -i 's/PRINT 16/PRINT 24/' App.h
make
./app

make clean
sed -i 's/PRINT 24/PRINT 32/' App.h
make
./app

make clean
sed -i 's/PRINT 32/PRINT 64/' App.h
make
./app

make clean
sed -i 's/PRINT 64/PRINT 128/' App.h
make
./app

make clean
sed -i 's/PRINT 128/PRINT 256/' App.h
make
./app

make clean
sed -i 's/PRINT 256/PRINT 512/' App.h
make
./app

make clean
sed -i 's/PRINT 512/PRINT 1024/' App.h
make
./app

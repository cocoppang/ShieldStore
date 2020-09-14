import random

name = [16, 32, 48, 64, 96, 128, 256, 512, 1024, 2048, 4096]

for r in name:
	f = open("run_%d.txt" % r, 'a')
	maxnum=16284*r
	for i in range(1, 10000001):
		num = random.randrange(1, maxnum)
		f.write("GET %d\n" % num);
	f.close()

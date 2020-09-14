name = [16, 32, 48, 64, 96, 128, 256, 512, 1024, 2048, 4096]

for r in name:
	f = open("load_%d.txt" % r, 'a')
	maxnum=16384*r
	for i in range(1, maxnum+1):
		f.write("SET %d 1\n" % i);
	f.close()

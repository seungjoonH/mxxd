all: mxxd

mxxd: mxxd.c
	gcc mxxd.c -o mxxd

clean:
	rm -f mxxd
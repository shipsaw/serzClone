serz: serz.o yxml.o map.o xmlToBin.o binToXml.o helpers.o
	gcc serz.o yxml.o map.o xmlToBin.o binToXml.o helpers.o -o serz

serz.o: serz.c
	gcc -c serz.c

yxml.o: yxml.c
	gcc -c yxml.c

map.o: map.c
	gcc -c map.c

xmlToBin.o: xmlToBin.c
	gcc -c xmlToBin.c

binToXml.o: binToXml.c
	gcc -c binToXml.c

helpers.o: helpers.c
	gcc -c helpers.c

clean:
	rm *.o serz

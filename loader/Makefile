#Create lib_simpleloader.so from loader.c
lib_simpleloader.so: loader.o
	gcc loader.o -m32 -shared -o lib_simpleloader.so

loader.o: loader.c loader.h
	gcc -m32 -fpic -c loader.c	

#Provide the command for cleanup
clean:
	-@rm -f loader.o lib_simpleloader.so
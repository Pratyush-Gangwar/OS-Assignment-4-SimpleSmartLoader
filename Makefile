#invoke make inside following directories and in this order: loader, launch, fib
#move the lib_simpleloader.so and launch binaries inside bin directory
#Provide the command for cleanup

all:
	cd ./loader && make && cp lib_simpleloader.so ../bin
	cd ./launcher && make && cp launch.out ../bin
	cd ./test && make

clean:
	rm ./bin/*
	cd ./loader && make clean
	cd ./launcher && make clean
	cd ./test && make clean
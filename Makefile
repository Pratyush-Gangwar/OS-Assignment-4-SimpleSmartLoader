all: launch.out ./test/fib.out

launch.out: launch.c ./signals/signal_handler.c ./loader/loader.c
	gcc -m32 launch.c ./signals/signal_handler.c ./loader/loader.c -o launch.out

./test/fib.out: ./test/fib.c 
	gcc  -m32 -no-pie -nostdlib -o ./test/fib.out ./test/fib.c

clean:
	rm launch.out ./test/fib.out
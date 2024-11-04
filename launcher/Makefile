#Compile the launch.c by linking it with the lib_simpleloader.so
launch.out: launch.c
	gcc -m32 launch.c -l_simpleloader -L../bin -Wl,-rpath='$$ORIGIN' -o launch.out

#Provide the command for cleanup
clean:
	-@rm -f launch.out 
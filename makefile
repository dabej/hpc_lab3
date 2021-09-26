newton : newton.c
	gcc newton.c -O2 -lpthread -lm -o newton
clean :
	rm newton; rm output_file.ppx;

newton : newton.c
	gcc -O2 -lm -lpthread newton.c -o newton
clean : 
	rm newton newton_conv*

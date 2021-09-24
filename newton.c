#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

/* global variable declaration */
int t, l; // no of threads and lines

int main (int argc, char *argv[]) {

	// Constants	
	const float precision = 0.001;

	// Argparse
	int opt, d;
	while ((opt = getopt(argc, argv, "t:l:")) != -1) {
		switch (opt) {
			case 't':
				t = atoi(optarg);
				break;
			case 'l':
				t = atoi(optarg);
				break;
		}
	}
	d = argv[optind];
		
}

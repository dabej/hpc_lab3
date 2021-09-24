#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
	
	// Argparse
	int opt, t, l, d;
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

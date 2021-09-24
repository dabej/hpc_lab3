#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define max_dist 3465

int main (int argc, char *argv[]) {
	
	// Argparse
	int opt, t;
	while ((opt = getopt(argc, argv, "t:")) != -1) {
		switch (opt) {
			case 't':
				t = atoi(optarg);
				break;
		}
	}



}

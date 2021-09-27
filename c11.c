#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <complex.h>

float complex f(float complex x, int exp);

float complex f_prim(float complex x, int exp);

int main (int argc, char *argv[]) {
	int opt, t, l, exp;
	while ((opt = getopt(argc, argv, "t:l:")) != -1) {
		switch (opt) {
			case 't':
				t = atoi(optarg);
				break;
			case 'l':
				l = atoi(optarg);
				break;
		}
	}
	exp = atoi(argv[3]);
	float iter_len = 4. / (l - 1);
	int *picture_data = (int*) malloc(sizeof(int)*l*l);
	int **picture = (int**) malloc(sizeof(int*)*l);
	for (size_t i = 0, j = 0; i < l; i++, j+=l*6)
		picture[i] = picture_data + j;

	complex float roots[exp];
	for (size_t n = 0; n < exp; n++) {
		complex float root = cosf(2*M_PI*n/exp) + sinf(2*M_PI*n/exp) * I;
		roots[n] = root;
	}

	struct timespec start_time;
	struct timespec end_time;
	double time;
	timespec_get(&start_time, TIME_UTC);

	float complex x, root;
	float real, imag, re;
	float im = -2.;
	int cont = 1;
	float low_t = 0.000001;
	long int high_t = 10000000000;
	for (size_t row = 0; row < l; row++) {
		re = -2.;
		for (size_t col = 0; col < l; col++) {
			x = re + im * I;
			cont = 1;
			while (1) {
				real = crealf(x);
				imag = cimagf(x);
				if (real*real+imag*imag < low_t || fabsf(real) > high_t || fabsf(imag) > high_t) {
					picture[col][row] = exp;
					break;
				}
				for (size_t i = 0; i < exp; i++) {
					root = roots[i];
					real = crealf(x) - crealf(root);
					imag = cimagf(x) - cimagf(root);
					if (real*real+imag*imag < low_t) {
						//printf("i: %d\n", i);
						picture[col][row] = i;
						cont = 0;
						break;
					}
				}
				if (cont)
					x -= f(x, exp) / f_prim(x, exp);
				else
					break;
			}
			/*picture[col][row] = 'a';
			picture[col][row+1] = ' ';
			picture[col][row+2] = 'b';
			picture[col][row+3] = ' ';
			picture[col][row+4] = 'c';
			picture[col][row+5] = '\n';*/
			//printf("%.4f %+.4fi, abs_val: %.4f\n", crealf(x), cimagf(x), abs_val_2);
			re += iter_len;
		}
		im += iter_len;
	}

	/*FILE *fp = fopen("newton_convergence_xd.ppm", "w");
	fprintf(fp, "%s\n%d %d\n%c\n", "P3", l, l, 'c');
	fwrite(picture_data, 6, l*l, fp);*/

	for (size_t i = 0; i < l; i++) {
		for (size_t j = 0; j < l; j++)
			printf("%d ", picture[j][i]);
		printf("\n");
	}

	timespec_get(&end_time, TIME_UTC);
	time = difftime(end_time.tv_sec, start_time.tv_sec) +
						(end_time.tv_nsec - start_time.tv_nsec) / 1000000000.;

	printf("Runtime: %f\n", time);

	return 0;
}

float complex f(float complex x, int exp) {
	float complex result = x;
	for (size_t i = 1; i < exp; i++)
		result *= x;
	result -= 1;
	return result;
}

float complex f_prim(float complex x, int exp) {
	float complex result = x;
	for (size_t i = 2; i < exp; i++)
		result *= x;
	result *= exp;
	return result;
}

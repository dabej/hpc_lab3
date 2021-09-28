#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <complex.h>
#include <string.h>

double complex f(double complex x, int exp);

double complex f_prim(double complex x, int exp);

struct color {
	char s[12];
};

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
	double iter_len = 4. / (l - 1);
	char *picture_data = (char*) malloc(12*l*l);
	char **picture = (char**) malloc(sizeof(char*)*l);
	for (size_t i = 0, j = 0; i < l; i++, j+=l*12)
		picture[i] = picture_data + j;

	char rgb[10][12] = {"176 11 105 \n", "215 133 180\n", "105 6 63   \n", "0 128 0    \n",
							  "127 191 127\n", "63 95 63   \n", "25 32 159  \n", "94 98 187  \n",
							  "190 192 227\n", "0 0 0      \n"};
	struct color colors[10];
	for (size_t i = 0; i < 10; i++) {
		struct color c;
		strncpy(c.s, rgb[i], 12);
		colors[i] = c;
	}

	complex double roots[exp];
	for (size_t n = 0; n < exp; n++) {
		complex double root = cosf(2*M_PI*n/exp) + sinf(2*M_PI*n/exp) * I;
		roots[n] = root;
	}

	struct timespec start_time;
	struct timespec end_time;
	double time;
	timespec_get(&start_time, TIME_UTC);

	FILE *fp = fopen("newton_convergence_xd.ppm", "w");
	fprintf(fp, "%s\n%d %d\n%d\n", "P3", l, l, 255);

	double complex x, root;
	double real, imag, re;
	double im = -2.;
	int cont = 1;
	double low_t = 0.000001;
	double high_t = 10000000000.;
	for (size_t row = 0; row < l; row++) {
		re = -2.;
		for (size_t col = 0; col < l*12; col+=12) {
			x = re + im * I;
			cont = 1;
			while (1) {
				real = creal(x);
				imag = cimag(x);
				if (real*real+imag*imag < low_t || fabs(real) > high_t || fabs(imag) > high_t) {
					strncpy(&picture[row][col], colors[exp].s, 12);
					break;
				}
				for (size_t i = 0; i < exp; i++) {
					root = roots[i];
					real = creal(x) - creal(root);
					imag = cimag(x) - cimag(root);
					if (real*real+imag*imag < low_t) {
						strncpy(&picture[row][col], colors[i].s, 12);
						cont = 0;
						break;
					}
				}
				if (cont)
					x -= f(x, exp) / f_prim(x, exp);
				else
					break;
			}
			re += iter_len;
		}
		im += iter_len;
	}

	fwrite(picture_data, 12, l*l, fp);

	fclose(fp);
	free(picture);
	free(picture_data);

	timespec_get(&end_time, TIME_UTC);
	time = difftime(end_time.tv_sec, start_time.tv_sec) +
						(end_time.tv_nsec - start_time.tv_nsec) / 1000000000.;

	printf("Runtime: %f\n", time);
	return 0;
}

double complex f(double complex x, int exp) {
	double complex result = x;
	for (size_t i = 1; i < exp; i++)
		result *= x;
	result -= 1;
	return result;
}

double complex f_prim(double complex x, int exp) {
	double complex result = x;
	for (size_t i = 2; i < exp; i++)
		result *= x;
	result *= exp;
	return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <complex.h>

double complex f(double complex x, int exp);

double complex f_prim(double complex x, int exp);

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
	unsigned char *picture_data = (unsigned char*) malloc(6*l*l);
	unsigned char **picture = (unsigned char**) malloc(sizeof(unsigned char*)*l);
	for (size_t i = 0, j = 0; i < l; i++, j+=l*6)
		picture[i] = picture_data + j;

	unsigned char colors[10][6];
	colors[0][0] = 176; colors[0][2] = 11; colors[0][4] = 105;
	colors[1][0] = 215; colors[1][2] = 133; colors[1][4] = 180;
	colors[2][0] = 105; colors[2][2] = 6; colors[2][4] = 63;
	colors[3][0] = 0; colors[3][2] = 128; colors[3][4] = 0;
	colors[4][0] = 127; colors[4][2] = 191; colors[4][4] = 127;
	colors[5][0] = 63; colors[5][2] = 95; colors[5][4] = 63;
	colors[6][0] = 25; colors[6][2] = 32; colors[6][4] = 159;
	colors[7][0] = 94; colors[7][2] = 98; colors[7][4] = 187;
	colors[8][0] = 190; colors[8][2] = 192; colors[8][4] = 227;
	colors[9][0] = 0; colors[9][2] = 0; colors[9][4] = 0;
	for (size_t i = 0; i < 10; i++) {
		colors[i][1] = ' ';
		colors[i][3] = ' ';
		colors[i][5] = '\n';
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

	double complex x, root;
	double real, imag, re;
	double im = -2.;
	int cont = 1;
	double low_t = 0.000001;
	double high_t = 10000000000.;
	for (size_t row = 0; row < l; row++) {
		re = -2.;
		for (size_t col = 0; col < l*6; col+=6) {
			x = re + im * I;
			cont = 1;
			while (1) {
				real = creal(x);
				imag = cimag(x);
				if (real*real+imag*imag < low_t || fabs(real) > high_t || fabs(imag) > high_t) {
					picture[row][col] = *colors[exp];
					break;
				}
				for (size_t i = 0; i < exp; i++) {
					root = roots[i];
					real = creal(x) - creal(root);
					imag = cimag(x) - cimag(root);
					if (real*real+imag*imag < low_t) {
						picture[row][col] = *colors[i];
						cont = 0;
						break;
					}
				}
				if (cont)
					x -= f(x, exp) / f_prim(x, exp);
				else
					break;
			}
			//printf("%.4f %+.4fi, abs_val: %.4f\n", creal(x), cimag(x), abs_val_2);
			re += iter_len;
		}
		im += iter_len;
	}

	FILE *fp = fopen("newton_convergence_xd.ppm", "w");
	fprintf(fp, "%s\n%d %d\n%c\n", "P3", l, l, 255);
	fwrite(picture_data, 6, l*l, fp);

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

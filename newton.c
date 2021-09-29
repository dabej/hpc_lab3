#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>

int nthrds, l, d; // no. of threads and lines

// Constants	
const double low_t = 0.000001;
const double high_t = 10000000000;

double complex f(double complex x, int d);

double complex f_prim(double complex x, int d);

int main_calc_thread(void *args);

int main_write_thread(void *args);

typedef struct {
    int val;
    char pad[60];
} int_padded;

typedef unsigned char TYPE_ATTR;
typedef unsigned char TYPE_CONV;

typedef struct {
    const double *v;
	TYPE_ATTR **a;
	TYPE_CONV **c;
    int ib;
    int istep;
    int tx;
    mtx_t *mtx;
    cnd_t *cnd;
    int_padded *status;
} calc_thread_info;

typedef struct {
    TYPE_ATTR **a;
	TYPE_CONV **c;
    mtx_t *mtx;
    cnd_t *cnd;
    int_padded *status;
} write_thread_info;

typedef struct {
	char s[12];
} color;

int main (int argc, char *argv[]) {

	struct timespec start_time;
	struct timespec end_time;
	double time;
	timespec_get(&start_time, TIME_UTC);

    // Argparse
    int opt;
    while ((opt = getopt(argc, argv, "t:l:")) != -1) {
		switch (opt) {
			case 't':
			nthrds = atoi(optarg);
			break;
			case 'l':
			l = atoi(optarg);
			break;
		}
    }
    d = atoi(argv[optind]);
    
    double *v = (double*) malloc(l*sizeof(double));
    TYPE_ATTR **a = (TYPE_ATTR**) malloc(sizeof(TYPE_ATTR*)*l);
	TYPE_CONV **c = (TYPE_CONV**) malloc(sizeof(TYPE_CONV*)*l);

	double imag = -2.;
	double step = 4. / (l-1);
	for (size_t i = 0; i < l; i++) {
		v[i] = imag;
		imag += step;
	}
	
    // create t compute threads
    thrd_t calc_thrds[nthrds];
    calc_thread_info calc_thread[nthrds];

    // create 1 writing thread
    thrd_t write_thrd;
    write_thread_info write_thread;

    mtx_t mtx;
    mtx_init(&mtx, mtx_plain);

    cnd_t cnd;
    cnd_init(&cnd);

    int_padded status[nthrds];

    // start up calculation threads
    for (int tx = 0; tx < nthrds; tx++){
		calc_thread[tx].v = (const double*) v;
		calc_thread[tx].a = a;
		calc_thread[tx].c = c;
		calc_thread[tx].ib = tx;
		calc_thread[tx].istep = nthrds;
		calc_thread[tx].tx = tx;
		calc_thread[tx].mtx = &mtx;
		calc_thread[tx].cnd = &cnd;
		calc_thread[tx].status = status;
		
		status[tx].val = -1;
		
		int r = thrd_create(calc_thrds+tx, main_calc_thread, (void*) (calc_thread+tx));
		if ( r != thrd_success ) {
			fprintf(stderr, "failed to create thread\n");
			exit(1);
		}
		thrd_detach(calc_thrds[tx]);
    }
  
    // start up write thread
	write_thread.a = a;
	write_thread.c = c;
    write_thread.mtx = &mtx;
    write_thread.cnd = &cnd;
    write_thread.status = status;
	

    int r = thrd_create(&write_thrd, main_write_thread, (void*) (&write_thread));
    if ( r != thrd_success ) {
		fprintf(stderr, "failed to create thread\n");
		exit(1);
    }

    {
	int r;
	thrd_join(write_thrd, &r);
    }
    free(v);
    free(a);
	free(c);

    mtx_destroy(&mtx);
    cnd_destroy(&cnd);

	timespec_get(&end_time, TIME_UTC);
	time = difftime(end_time.tv_sec, start_time.tv_sec) +
					(end_time.tv_nsec - start_time.tv_nsec) / 1000000000.;

	printf("Runtime: %f\n", time);
    return 0;
}

int main_calc_thread(void *args){
    const calc_thread_info *thrd_info = (calc_thread_info*) args;
    const double *v = thrd_info->v;
    TYPE_ATTR **a = thrd_info->a;
	TYPE_CONV **c = thrd_info->c;
    const int ib = thrd_info->ib;
    const int istep = thrd_info->istep;
    const int tx = thrd_info->tx;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;
    int_padded *status = thrd_info->status;

	complex double roots[d];
	for (size_t n = 0; n < d; n++) {
		complex double root = cosf(2*M_PI*n/d) + sinf(2*M_PI*n/d) * I;
		roots[n] = root;
	}

	double step = 4. / (l - 1);
	double complex x, root;
	double real, imag, re;
	int cont, conv;

    for ( int ix = ib; ix < l; ix+=istep ) {
    
		const double im = v[ix];
		TYPE_ATTR *a_row = (TYPE_ATTR*) malloc(l);
		TYPE_CONV *c_row = (TYPE_CONV*) malloc(l);
		re = -2.;

		for (size_t col = 0; col < l; col++) {
			x = re + im * I;
			cont = 1;
			conv = 0;
			while (1) {
				real = creal(x);
				imag = cimag(x);
				if (real*real+imag*imag < low_t || fabs(real) > high_t || fabs(imag) > high_t) {
					a_row[col] = d;
					c_row[col] = conv;
					break;
				}
				for (size_t i = 0; i < d; i++) {
					root = roots[i];
					real = creal(x) - creal(root);
					imag = cimag(x) - cimag(root);
					if (real*real+imag*imag < low_t) {
						a_row[col] = i;
						c_row[col] = conv;
						cont = 0;
						break;
					}
				}
				if (cont) {
					x -= f(x, d) / f_prim(x, d);
					conv++;
				} else
					break;
			}
			re += step;
		}

		mtx_lock(mtx);
		a[ix] = a_row;
		c[ix] = c_row;
		status[tx].val = ix+istep;
		mtx_unlock(mtx);
		cnd_signal(cnd);
	}
    
    return 0;
}

int main_write_thread(void *args){

	char rgb[10][12] = {"176 11 105 \n", "215 133 180\n", "105 6 63   \n", "0 128 0    \n",
					    "127 191 127\n", "63 95 63   \n", "25 32 159  \n", "94 98 187  \n",
					    "190 192 227\n", "0 0 0      \n"};

	color colors[10];
	for (size_t i = 0; i < 10; i++) {
		color c;
		strncpy(c.s, rgb[i], 12);
		colors[i] = c;
	}

	FILE *fp_a = fopen("newton_attractors_xd.ppm", "w");
	FILE *fp_c = fopen("newton_convergence_xd.ppm", "w");
	fprintf(fp_a, "%s\n%d %d\n%d\n", "P3", l, l, 255);
	fprintf(fp_c, "%s\n%d %d\n%d\n", "P2", l, l, 255);
    
	const write_thread_info *thrd_info = (write_thread_info*) args;
    TYPE_ATTR **a = thrd_info->a;
	TYPE_CONV **c = thrd_info->c;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;
	int_padded *status = thrd_info->status;

	char *buffer = (char*) malloc(12*l);

	// We do not increment ix in this loop, but in the inner one.
	for ( int ix = 0, ibnd; ix <l; ) {

		// If no new lines are available, we wait.
		for ( mtx_lock(mtx); ; ) {
			// We extract the minimum of all status variables.
		    ibnd = l;
			for ( int tx = 0; tx < nthrds; ++tx )
        		if ( ibnd > status[tx].val )
          			ibnd = status[tx].val;

      		if ( ibnd <= ix )
        		// We rely on spurious wake-ups, which in practice happen, but are not
        		// guaranteed.
        		cnd_wait(cnd,mtx);
      		else {
        		mtx_unlock(mtx);
        		break;
      		}

    	}

   		// WRITING TO FILE BELOW, ROW BY ROW
		for ( ; ix < ibnd; ++ix ) {
			for (size_t i = 0; i < l; i++)
				strncpy(&buffer[i*12], colors[a[ix][i]].s, 12);
			free(a[ix]);
			fwrite(buffer, 12, l, fp_a);
			for (size_t i = 0; i < l; i++)
				strcpy(&buffer[i*4], "150\n");//"%d\n", c[ix][i]);
			free(c[ix]);
			fwrite(buffer, 4, l, fp_c);
		}

	}
	free(buffer);
	fclose(fp_a);
  	fclose(fp_c);
	return 0;
}

double complex f(double complex x, int d) {
	double complex result = 1;
	for (size_t i = 0; i < d; i++)
		result *= x;
	result -= 1;
	return result;
}

double complex f_prim(double complex x, int d) {
	double complex result = 1;
	for (size_t i = 1; i < d; i++)
		result *= x;
	result *= d;
	return result;
}

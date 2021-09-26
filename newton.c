#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>

/* global variable declaration */
int t, l; // no. of threads and lines

int main_calc_thread(void *args);

int main_write_thread(void *args);

typedef struct {
    int val;
    char pad[60];
} int_padded;

typedef struct {
    const float **v;
    float **w;
    int ib;
    int istep;
    int sz;
    int tx;
    mtx_t *mtx;
    cnd_t *cnd;
    int_padded *status;
} calc_thread_info;

typedef struct {
    FILE* output_file;
    int* rows_to_print;
    float **w;
    mtx_t *mtx;
    cnd_t *cnd;
} write_thread_info;

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
		l = atoi(optarg);
		break;
	}
    }
    //d = argv[optind]; - vad är denna rad till för? Någon hade skrivit dit den.

    // FOR TESTING PURPOSES
    t = 5;
    l = 100;

    float **v = (float**) malloc(l*sizeof(float*));
    float **w = (float**) malloc(l*sizeof(float*));
    float *ventries = (float*) malloc(l*l*sizeof(float));

    for ( int ix = 0, jx = 0; ix < l; ++ix, jx += l )
	v[ix] = ventries + jx;
	
    for ( int ix = 0; ix < l*l; ++ix )
        ventries[ix] = ix;

    // create t-2 compute threads
    thrd_t calc_thrds[t-2];
    calc_thread_info calc_thread[t-2];

    // create 1 writing thread
    thrd_t write_thrd;
    write_thread_info write_thread;

    mtx_t mtx;
    mtx_init(&mtx, mtx_plain);

    cnd_t cnd;
    cnd_init(&cnd);

    int_padded status[t-2];

    // start up all calculation threads
    for (int tx = 0; tx < t-2; tx++){
	calc_thread[tx].v = (const float**) v;
	calc_thread[tx].w = w;
	calc_thread[tx].ib = tx;
	calc_thread[tx].istep = t-2;
	calc_thread[tx].sz = l;
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
  
    // create pointer to how many rows the writer should print
    int *rows_to_print = (int*) malloc(sizeof(int)); 

    // start up write thread
    write_thread.output_file = fopen("output_file.ppx", "w");
    write_thread.rows_to_print = rows_to_print;
    write_thread.w = w;
    write_thread.mtx = &mtx;
    write_thread.cnd = &cnd;

    int r = thrd_create(&write_thrd, main_write_thread, (void*) (&write_thread));
    if ( r != thrd_success ) {
	fprintf(stderr, "failed to create thread\n");
	exit(1);
    }

    {
	
	// [ check how much is computed (by looking at the minimum value of all thread's status values) and tell writing thread to write stuff to file ]
	for ( int ix = 0, ibnd; ix < l; ) {

	    // wait if no new lines are available
	    for ( mtx_lock(&mtx); ; ) {
		// extract minimum of all status variables
		ibnd = l;
		for ( int tx = 0; tx < t-2; ++tx )
		    if ( ibnd > status[tx].val )
			ibnd = status[tx].val;

		if ( ibnd <= ix ) {
		    // we rely on spurious wake-ups
		    printf("im waiting for a line to appear\n");
		    cnd_wait(&cnd,&mtx);
		} else {
		    mtx_unlock(&mtx);
		    break;
		}

	    }

	    fprintf(stderr, "telling write thread to write until %i\n", ibnd);
	    
	    if (ibnd == 100) // kan detta ersättas med någonting snyggare så att evighetsloopen slutar naturligt av att comp_threadsen slutar beräkna saker?
		break;
	}
    }
    
    {
	int r;
	thrd_join(write_thrd, &r);
    }

    free(ventries);
    free(v);
    free(w);
    free(rows_to_print);

    mtx_destroy(&mtx);
    cnd_destroy(&cnd);

    return 0;
}

int main_calc_thread(void *args){
    const calc_thread_info *thrd_info = (calc_thread_info*) args;
    const float **v = thrd_info->v;
    float **w = thrd_info->w;
    const int ib = thrd_info->ib;
    const int istep = thrd_info->istep;
    const int sz = thrd_info->sz;
    const int tx = thrd_info->tx;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;
    int_padded *status = thrd_info->status;

    for ( int ix = ib; ix < sz; ix+=istep ) {
    
	const float *vix = v[ix];
	float *wix = (float*) malloc(sz*sizeof(float));

	// perform calculations and update status to the highest line computed
	
	printf("computing line %d\n", ix);

	mtx_lock(mtx);
	w[ix] = wix;
	status[tx].val = ix+istep;
	mtx_unlock(mtx);
	cnd_signal(cnd);
    }
    
    return 0;
}

int main_write_thread(void *args){

    const write_thread_info *thrd_info = (write_thread_info*) args;
    FILE *output_file = thrd_info->output_file;
    int* rows_to_print = thrd_info->rows_to_print;
    float **w = thrd_info->w;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;

    // write to file up until line rows_to_print

    printf("printing until row %d", rows_to_print);
}

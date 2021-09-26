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
    float row_to_print;
    float **w;
    mtx_t *mtx;
    cnd_t *cnd; // tror denna är överflödig
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
    //d = argv[optind]; - vad är denna rad till för?

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
    for (int tx = 0; tx < t; tx++){
	calc_thread[tx].v = (const float**) v;
	calc_thread[tx].w = w;
	calc_thread[tx].ib = tx;
	calc_thread[tx].istep = t;
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
	// enligt videon var detach inte rekommenderat?
	thrd_detach(calc_thrds[tx]);
    }
    
    {
	
	// [ check how much is computed (by looking in status's) and tell writing thread to write stuff to file ]
	
    }
    
    {
	int r;
	thrd_join(write_thrd, &r);
    }

    free(ventries);
    free(v);
    free(w);

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

	// perform calculation here
	
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
    float row_to_print;
    float **w = thrd_info->w;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;

    // write to file here

    printf("printing row %d", row_to_print);
}

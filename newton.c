#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>

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
    int* print_from;
    int* print_to;
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
    d = atoi(argv[optind]);
    // FOR TESTING PURPOSES
    // t = 8;
    // l = 200;
    
    int chunk_size = 5;

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

    // start up calculation threads
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
  
    // create variables needed for printing logic
    int start_print = 0;
    int end_print = 0;
    int print_chunk = 0;

    // start up write thread
	FILE *fp = fopen("output_file.ppx", "w");
    write_thread.output_file = fp;
    write_thread.print_from = &start_print;
    write_thread.print_to = &end_print;
    write_thread.w = w;
    write_thread.mtx = &mtx;
    write_thread.cnd = &cnd;

    int r = thrd_create(&write_thrd, main_write_thread, (void*) (&write_thread));
    if ( r != thrd_success ) {
	fprintf(stderr, "failed to create thread\n");
	exit(1);
    }

    { 
    /* 
     * Main loop - check the lowest line that is complete and tell writing thread to write stuff to file if a non-written block of 50 lines is done
     */	

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
		    //printf("main thread \t - waiting for a line to appear\n");
		    cnd_wait(&cnd,&mtx);
		} else {
		    mtx_unlock(&mtx);
		    break;
		}
	    }

	    if (ibnd/chunk_size - start_print/chunk_size > print_chunk){
		//fprintf(stderr, "main thread \t - telling write thread to write until %i\n", ibnd);
		
		end_print = ibnd;
		print_chunk++;
	    }

	    if (ibnd == l) // kan detta ersättas med någonting snyggare så att evighetsloopen slutar automatiskt i samband med att comp_threadsen slutar beräkna saker?
		break;
	}
    }
    
    {
	int r;
	thrd_join(write_thrd, &r);
    }
	fclose(fp);
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

	// [ perform calculations and save result in w ]
	
	//printf("calc thread %d \t - computing line %d\n", tx, ix);

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
    int* print_from = thrd_info->print_from;
    int* print_to = thrd_info->print_to;
    float **w = thrd_info->w;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;

    // [ write lines to file from print_from until print_to ]
	   
 
    for (int i=0; i<1000000; i++){// dummy for-loop to avoid starting threads that run forever now in testing stage. Replace with while(true) or something else that stays "active" and waiting for wor. Replace with while(true) or something else that stays "active" and waiting for work
	if (*print_from < *print_to){
	
		for (int ix = *print_from; ix < *print_to; ix++) {
			free(w[ix]);
		}   
		 // print the stuff to file
	    printf("writing thread \t - printing stuff to file \n");

	    // update "baseline" to keep printing from next time
	    *print_from = *print_to;
	} else
	    rand()%10;
    }    

}

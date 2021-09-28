#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>

int nthrds, l; // no. of threads and lines

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
    float **w;
    mtx_t *mtx;
    cnd_t *cnd;
    int_padded *status;
} write_thread_info;

int main (int argc, char *argv[]) {

    // Constants	
    const float precision = 0.001;

    // Argparse
    int opt, d;
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
    
    float **v = (float**) malloc(l*sizeof(float*));
    float **w = (float**) malloc(l*sizeof(float*));
    float *ventries = (float*) malloc(l*l*sizeof(float));

    for ( int ix = 0, jx = 0; ix < l; ++ix, jx += l )
		v[ix] = ventries + jx;
	
    for ( int ix = 0; ix < l*l; ++ix )
        ventries[ix] = ix;

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
		calc_thread[tx].v = (const float**) v;
		calc_thread[tx].w = w;
		calc_thread[tx].ib = tx;
		calc_thread[tx].istep = nthrds;
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
  
    // start up write thread
	write_thread.w = w;
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
	printf("freeing rows\n");
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

		// [ perform calculations and save result in w ], row by row
		for ( int jx = 0; jx < sz; ++jx ) {
      		wix[jx] = vix[jx];
			//printf("computed index %f\n",wix[jx]);
		}
		mtx_lock(mtx);
		w[ix] = wix;
		status[tx].val = ix+istep;
		mtx_unlock(mtx);
		cnd_signal(cnd);
	}
    
    return 0;
}

int main_write_thread(void *args){

	FILE *fp = fopen("output_file.ppx", "w");
    
	const write_thread_info *thrd_info = (write_thread_info*) args;
    float **w = thrd_info->w;
    mtx_t *mtx = thrd_info->mtx;
    cnd_t *cnd = thrd_info->cnd;
	int_padded *status = thrd_info->status;	
	
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

    	fprintf(stderr, "done with computation until row %i\n", ibnd);

   		// WRITING TO FILE BELOW, ROW BY ROW
		for ( ; ix < ibnd; ++ix ) {
   			printf("writing row %d to string\n",ix);
			free(w[ix]);
			}

		/*if (ix == l){
			printf("exiting..");
			fclose(fp);
			exit(1);
   		}*/
	}
	fclose(fp);
  	
	return 0;
}

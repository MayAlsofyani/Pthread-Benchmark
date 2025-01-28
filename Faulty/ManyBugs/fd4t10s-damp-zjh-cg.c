#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

// Constants
#define CG_MAX 4
#define ND 4
#define X_MAX 4
#define Z_MAX 4
#define ROW 2
#define COL 2
#define d 1

// Global variables
pthread_mutex_t mut1, mut2;
pthread_cond_t cond1, cond2;
int g_status;

// Forward declarations
void master_wait();
void master_signal(struct Param **params);
void slave_wait(struct Param *param);
void slave_signal();
void compute_ldm(struct Param *param);

// Param structure definition
struct Param {
    int nx, nz, nb, ldnx, ldnz, freeSurface, id, nd, icg, status, exit;
    int gnxbeg, gnzbeg;
    const float *prev_wave, *curr_wave, *vel;
    float *next_wave, *u2, *src_wave, *vsrc_wave, *image, scale;
    int crnx, crnz, task;
    struct Param **params;
};

// Function definitions

void master_wait() {

    while (g_status != 0) {
        pthread_cond_wait(&cond1, &mut1);
    }

    g_status = CG_MAX;
}

void master_signal(struct Param **params) {
    int i;

    for (i = 0; i < CG_MAX; i++) {
        params[i * ND]->status = 0;
    }
    pthread_cond_broadcast(&cond2);

}

void slave_wait(struct Param *param) {

    while (param->status) {
        pthread_cond_wait(&cond2, &mut2);
    }

}

void slave_signal() {

    g_status--;
    pthread_cond_signal(&cond1);

}

void compute_ldm(struct Param *param) {
    int nx = param->nx;
    int nz = param->nz;
    int nb = param->nb;
    int cnx = nx - 2 * d;
    int cnz = nz - 2 * d;
    int snx = ceil(cnx * 1.0 / ROW);
    int snz = ceil(cnz * 1.0 / COL);
    int cid = 0;
    int rid = 0;
    int snxbeg = snx * cid - d + d;
    int snzbeg = snz * rid - d + d;
    int snxend = snx * (cid + 1) + d + d;
    int snzend = snz * (rid + 1) + d + d;
    snxend = snxend < nx ? snxend : nx;
    snzend = snzend < nz ? snzend : nz;
    snx = snxend - snxbeg;
    snz = snzend - snzbeg;
    int vel_size = (snx - 2 * d) * (snz - 2 * d) * sizeof(float);
    int curr_size = snx * snz * sizeof(float);
    int total = vel_size * 2 + curr_size * 2 + nb * sizeof(float) + 6 * sizeof(float);
    printf("LDM consume: vel_size = %dB, curr_size = %dB, total = %dB\n", vel_size, curr_size, total);
}

int fd4t10s_4cg(void *ptr) {
    struct Param *param = (struct Param *)ptr;
    struct Param **params = param->params;

    compute_ldm(param);
    while (1) {
        slave_wait(param);
        if (param->exit) {
            break;
        }
        param->status = 1;
        int i;
        for (i = 0; i < ND; i++) {
            param = params[param->id + i];
            if (param->task == 0) {
                // Simulating work done by threads (replace with actual implementation)
            } else if (param->task == 1) {
                // Simulating work done by threads (replace with actual implementation)
            }
            param = params[param->id];
        }
        slave_signal();
    }
    return 0;
}

pthread_t pt[CG_MAX];
struct Param *params[X_MAX * Z_MAX];

void fd4t10s_nobndry_zjh_2d_vtrans_cg(const float *prev_wave, const float *curr_wave, float *next_wave, const float *vel, float *u2, int nx, int nz, int nb, int nt, int freeSurface) {
    static int init = 1;
    if (init) {
        if (pthread_mutex_init(&mut1, NULL) != 0 || pthread_mutex_init(&mut2, NULL) != 0) {
            printf("Mutex init error\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_cond_init(&cond1, NULL) != 0 || pthread_cond_init(&cond2, NULL) != 0) {
            printf("Cond init error\n");
            exit(EXIT_FAILURE);
        }

        for (int ix = 0; ix < X_MAX; ix++) {
            int cnx = nx - 2 * d;
            int snx = ceil(cnx * 1.0 / X_MAX);
            int snxbeg = snx * ix - d + d;
            int snxend = snx * (ix + 1) + d + d;
            snxend = snxend < nx ? snxend : nx;
            snx = snxend - snxbeg;
            for (int iz = 0; iz < Z_MAX; iz++) {
                int cnz = nz - 2 * d;
                int snz = ceil(cnz * 1.0 / Z_MAX);
                int snzbeg = snz * iz - d + d;
                int snzend = snz * (iz + 1) + d + d;
                snzend = snzend < nz ? snzend : nz;
                snz = snzend - snzbeg;
                int id = ix * Z_MAX + iz;
                params[id] = (struct Param *)malloc(sizeof(struct Param));
                if (params[id] == NULL) {
                    perror("malloc failed");
                    exit(EXIT_FAILURE);
                }
                params[id]->nx = snx;
                params[id]->nz = snz;
                params[id]->ldnx = nx;
                params[id]->ldnz = nz;
                params[id]->nb = nb;
                params[id]->freeSurface = freeSurface;
                params[id]->id = id;
                params[id]->nd = ND;
                params[id]->icg = id / ND;
                params[id]->status = 1;
                params[id]->exit = 0;
                params[id]->gnxbeg = snxbeg;
                params[id]->gnzbeg = snzbeg;
                params[id]->params = params;
            }
        }
        g_status = CG_MAX;
    }

    if (init) {
        for (int icg = 0; icg < CG_MAX; icg++) {
            int id_beg = icg * ND;
            if (pthread_create(&pt[icg], NULL, fd4t10s_4cg, (void *)params[id_beg]) != 0) {
                perror("pthread_create failed");
                exit(EXIT_FAILURE);
            }
        }
        init = 0;
    }

    master_signal(params);
    master_wait();
}

// Cleanup function
void fd4t10s_4cg_exit() {
    for (int icg = 0; icg < CG_MAX; icg++) {
        params[icg * ND]->exit = 1;
    }
    master_signal(params);

    for (int icg = 0; icg < CG_MAX; icg++) {
        pthread_join(pt[icg], NULL);
    }

    for (int ix = 0; ix < X_MAX; ix++) {
        for (int iz = 0; iz < Z_MAX; iz++) {
            free(params[ix * Z_MAX + iz]);
        }
    }

    pthread_mutex_destroy(&mut1);
    pthread_mutex_destroy(&mut2);
    pthread_cond_destroy(&cond1);
    pthread_cond_destroy(&cond2);
}


#include <x86intrin.h>

/*
Le fichier fournit les fonctions start_timer(), stop_timer() et dtime().

Start/stop_timer utilisent l'instruction rdtsc pour obtenir le compteur de
temps. La macro USE_RDTSC_INTRINSIC définit si on utilise l'intrisic de gcc
__rdtsc() ou du code assembleur. La première solution est plus standard, mais
__rdtsc() ramène toutes les mesures à une fréquence moyenne du processeur. Si le
processeur a des variations de fréquence importante (par exemple un portable)
les résultats seront peu reproductibles. Si USE_RDTSC_INTRINSIC n'est pas
définit, on utilise du code assembleur. Dans tous les cas un long long est
renvoyé. Details sur TSC
https://forums.guru3d.com/threads/a-bit-detailed-info-on-intel-time-stamp-counter-tsc.433977/

dtime prend en paramètre deux temps long long, applique la correction éventuelle
TSCCYCLES (depend du processeur), et convertit en double la différence.
*/

// suivant l'architecture, le nombre et types de registres touchés par rdtsc
// varie
#ifdef __i386__
#define RDTSC_DIRTY "%eax", "%ebx", "%ecx", "%edx"
#elif __x86_64__
#define RDTSC_DIRTY "%rax", "%rbx", "%rcx", "%rdx"
#else
#error unknown platform
#endif

static inline unsigned long long start_timer(void) {
#if !defined(_rdtsc) // utiliser du code assembleur x86
  unsigned int hi = 0, lo = 0;

  asm volatile("cpuid\n\t"         /* pour vider le pipeline */
               "rdtscp\n\t"        /* on lit le registre TSC */
               "mov %%edx, %0\n\t" /* on restore les registres */
               "mov %%eax, %1\n\t"
               : "=r"(hi), "=r"(lo) /* Le résultat */
                 ::RDTSC_DIRTY); /* les registres à sauvegarder dans la pile */
  unsigned long long that =
      (unsigned long long)((lo) | (unsigned long long)(hi) << 32);

  return that;
#else
  return _rdtsc();
#endif
}

static inline unsigned long long stop_timer(void) {

#if !defined(_rdtsc) // utiliser du code assembleur x86
  unsigned int hi = 0, lo = 0;

  asm volatile("rdtscp\n\t"        /* on lit le registre TSC */
               "mov %%edx, %0\n\t" /* on restore les registres */
               "mov %%eax, %1\n\t"
               "cpuid\n\t"          /* barrière hw */
               : "=r"(hi), "=r"(lo) /* Les PF et pf du résultat */
                 ::RDTSC_DIRTY); /* les registres à sauvegarder dans la pile */
  unsigned long long that =
      (unsigned long long)((lo) | (unsigned long long)(hi) << 32);

  return that;
#else
  return _rdtsc();
#endif
}

// The overhead of the rdtsc intrinsic may be non-negligeable and should be
// accounted in calculations. You can either define a TSCCYCLES macro before
// including the header or measure it at runtime with the eval_tsc_cycles
// function.
#ifdef TSCCYLES

static inline double dtime(long long debut, long long fin) {
  return (double)(fin - debut - TSCCYCLES);
}

#else
static inline double dtime(long long debut, long long fin, double tsccycles) {
  return (double)(fin - debut - tsccycles);
}

#define N 100000
#include <math.h>

int cbench_compare_double(const void *x, const void *y) {
  double xx = *(double *)x, yy = *(double *)y;
  if (xx < yy)
    return -1;
  if (xx > yy)
    return 1;
  return 0;
}

double tmean(double ar[], int n) {
  // moyenne arrangée pour enlever les points abherrants.
  // On cherche la médiane m et les quartiles q1 et q3.
  // On calcule la moyenne sur tout ce qui est dans [m-k*(q3-q1),m+k*(q3-q1)]
  // k=1.0 marche bien

  double q1, q2, q3, sum = 0.0, k = 1.0;
  int ii, jj;
  // On trie les données pour avoir mediane et quartiles
  qsort(ar, n, sizeof(double), cbench_compare_double);
  q1 = ar[N / 4];
  q2 = ar[N / 2];
  q3 = ar[3 * N / 4];

  // calcul de la moyenne des valeurs dans l'intervalle
  // [q2-k*(q3-q1),q2+k*(q3-q1)]
  ii = 0;
  while (ar[ii++] < q2 - k * (q3 - q1))
    ;
  jj = ii;
  while (ar[jj] < q2 + k * (q3 - q1)) {
    sum += ar[jj];
    jj++;
    if (jj >= n - 1)
      break;
  }
  return sum / (double)(jj - ii);
}

typedef struct {
  double tavg;
  double tmin;
  double tmax;
  double tvar;
  double ttavg;
} measurement;

static inline void cbench_update_stats(double t, double *tmin, double *tmax,
                                       double *tsum, double *t2sum) {
  *tsum += t;
  *t2sum += t * t;
  *tmin = (*tmin < t ? *tmin : t);
  *tmax = (*tmax > t ? *tmax : t);
}

static inline measurement cbench_compute_stats(double tmin, double tmax,
                                               double tsum, double t2sum,
                                               double *array, unsigned n) {
  double tvar, tavg, ttavg;
  tavg = tsum / n; // moyenne brute
  tvar =
      sqrt((t2sum + (tavg * tavg * N) - 2 * tavg * tsum) / N); // variance brute
  ttavg = tmean(array, N); // moyenne arrangée
  return (measurement){tavg, tmin, tmax, tvar, ttavg};
}

static measurement compute_measurement(double array[], unsigned n) {
  double tsum = 0.0, t2sum = 0.0, tmin = 1e30, tmax = 0.0;
  for (unsigned i = 0; i < n; i++) {
    double t = array[i];
    // pour calcul moyenne et variance brutes
    cbench_update_stats(t, &tmin, &tmax, &tsum, &t2sum);
  }
  return cbench_compute_stats(tmin, tmax, tsum, t2sum, array, n);
}

static measurement eval_tsc_cycles(void) {
  double tsum = 0.0, t2sum = 0.0, tmin = 1e30, tmax = 0.0;
  double tresults[N];
  long long debut, fin;
  double t;

  for (int i = 0; i < N; i++) {

    debut = start_timer();
    fin = stop_timer();

    t = (double)(fin - debut);
    // mise dans un tableau pour moyenne arrangée
    tresults[i] = t;
    cbench_update_stats(t, &tmin, &tmax, &tsum, &t2sum);
  }
  return cbench_compute_stats(tmin, tmax, tsum, t2sum, tresults, N);
}

#undef N

#endif

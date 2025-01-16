// Pour mesurer les performances d'un pentium

// Suivant la machine, le temps de mesure peut varier.
// Utiliser la fonction eval_tsc_cycles() pour avoir une mesure un peu plus fine
#include <x86intrin.h>

#ifndef TSCCYCLES
#define TSCCYCLES 0.0 // si correction systématique nécessaire. 100.0 donne en général de bons résultats
#endif


/*
Le fichier fournit les fonctions start_timer(), stop_timer() et dtime().

Start/stop_timer utilisent l'instruction rdtsc pour obtenir le compteur de temps.
La macro USE_RDTSC_INTRINSIC définit si on utilise l'intrisic de gcc __rdtsc() ou du code assembleur.
La première solution est plus standard, mais __rdtsc() ramène toutes les mesures à une fréquence moyenne
du processeur. Si le processeur a des variations de fréquence importante (par exemple un portable)
les résultats seront peu reproductibles. Si USE_RDTSC_INTRINSIC n'est pas définit, on utilise du code assembleur.
Dans tous les cas un long long est renvoyé.
Details sur TSC https://forums.guru3d.com/threads/a-bit-detailed-info-on-intel-time-stamp-counter-tsc.433977/

dtime prend en paramètre deux temps long long, applique la correction éventuelle TSCCYCLES (depend du processeur),
et convertit en double la différence.
*/

// #define USE_RDTSC_INTRINSIC

// suivant l'architecture, le nombre et types de registres touchés par rdtsc varie
#ifdef __i386__
#define RDTSC_DIRTY "%eax", "%ebx", "%ecx", "%edx"
#elif __x86_64__
#define RDTSC_DIRTY "%rax", "%rbx", "%rcx", "%rdx"
#else
#error unknown platform
#endif

static inline unsigned long long start_timer()
{
#if !defined(USE_RDTSC_INTRINSIC) // utiliser du code assembleur x86
	unsigned int hi = 0, lo = 0;

	asm volatile("cpuid\n\t"		 /* pour vider le pipeline */
				 "rdtscp\n\t"		 /* on lit le registre TSC */
				 "mov %%edx, %0\n\t" /* on restore les registres */
				 "mov %%eax, %1\n\t"
				 : "=r"(hi), "=r"(lo) /* Le résultat */
				   ::RDTSC_DIRTY);	  /* les registres à sauvegarder dans la pile */
	unsigned long long that = (unsigned long long)((lo) |
												   (unsigned long long)(hi) << 32);

	return that;
#else
	return _rdtsc();
#endif
}

static inline unsigned long long stop_timer()
{

#if !defined(USE_RDTSC_INTRINSIC) // utiliser du code assembleur x86
	unsigned int hi = 0, lo = 0;

	asm volatile("rdtscp\n\t"		 /* on lit le registre TSC */
				 "mov %%edx, %0\n\t" /* on restore les registres */
				 "mov %%eax, %1\n\t"
				 "cpuid\n\t"		  /* barrière hw */
				 : "=r"(hi), "=r"(lo) /* Les PF et pf du résultat */
				   ::RDTSC_DIRTY);	  /* les registres à sauvegarder dans la pile */
	unsigned long long that = (unsigned long long)((lo) |
												   (unsigned long long)(hi) << 32);

	return that;
#else
	return _rdtsc();
#endif
}

// convertit la différence en double pour faciliter les calculs
// et ajuste en enlevant avec le temps moyen d'excécution de l'instruction rdtsc
static inline double dtime(long long debut, long long fin)
{
	double time;
	return (double)(fin - debut - TSCCYCLES);
}

#ifdef EVALTSCTIME

// Programme pour évaluer le temps d'une mesure par rdtsc sur un ordinateur.
// Copier dans un ficher avec l'extension.c
// et compiler avec gcc -O3 -DEVALTSCTIME tsc.c -lm

#define N 100000
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int cmp(const void *x, const void *y)
{
	double xx = *(double *)x, yy = *(double *)y;
	if (xx < yy)
		return -1;
	if (xx > yy)
		return 1;
	return 0;
}

double tmean(double ar[], int n)
{
	// moyenne arrangée pour enlever les points abherrants.
	// On cherche la médiane m et les quartiles q1 et q3.
	// On calcule la moyenne sur tout ce qui est dans [m-k*(q3-q1),m+k*(q3-q1)]
	// k=1.0 marche bien

	double q1, q2, q3, sum = 0.0, k = 1.0;
	int ii, jj;
	// On trie les données pour avoir mediane et quartiles
	qsort(ar, n, sizeof(double), cmp);
	q1 = ar[N / 4];
	q2 = ar[N / 2];
	q3 = ar[3 * N / 4];

	// calcul de la moyenne des valeurs dans l'intervalle [q2-k*(q3-q1),q2+k*(q3-q1)]
	ii = 0;
	while (ar[ii++] < q2 - k * (q3 - q1))
		;
	jj = ii;
	while (ar[jj] < q2 + k * (q3 - q1))
	{
		sum += ar[jj];
		jj++;
		if (jj >= n - 1)
			break;
	}
	return sum / (double)(jj - ii);
}

void eval_tsc_cycles()
{

	double tsum = 0.0, t2sum = 0.0, tavg, tvar, tmin = 1e30, tmax = 0.0, ttavg;
	double tresults[N];
	long long debut, fin;
	double t;

	for (int i = 0; i < N; i++)
	{

		debut = start_timer();
		fin = stop_timer();

		t = (double)(fin - debut);
		// mise dans un tableau pour moyenne arrangée
		tresults[i] = t;
		// pour calcul moyenne et variance brutes
		tsum += t;
		t2sum += t * t;
		tmin = (tmin < t ? tmin : t);
		tmax = (tmax > t ? tmax : t);
	}
	tavg = tsum / N;												// moyenne brute
	tvar = sqrt((t2sum + (tavg * tavg * N) - 2 * tavg * tsum) / N); // variance brute
	ttavg = tmean(tresults, N);										// moyenne arrangée
	printf("moyenne=%f\tmin=%f\tmax=%f\tvariance=%f\tavg=%f\n", tavg, tmin, tmax, tvar, ttavg);
}

int main()
{
	eval_tsc_cycles();
}

#endif

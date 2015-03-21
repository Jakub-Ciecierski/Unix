/*
Napisz program składający sie z ponumerowanych 1,2,...,n procesów.
 Proces o numerze 1 (rodzic) ma utworzyć proces 2, ten tworzy proces 3 itd.
  Proces nr 1 wysyła sygnał SIGUSR1 do procesu nr 2, 
  ten losową ilość razy <1,10> wypisuje na ekran swój pid (bez \n). 
  Pomiędzy wypisaniami występuje przerwa 0,1s. 
  Po zakończeniu wypisywania k-ty proces wysyła sygnał SIGUSR1 do k+1 potomka itd.
   Proces o numerze n po wypisywaniu na ekran odwraca kolejność i 
   wysyła sygnał SIGUSR2 do procesu n-1. 
   Tu znowu następuje wypisywanie po czym wysyłany jest SIGUSR2 do 
   procesu n-2 itd. Kiedy SIGUSR2 dotrze do procesu numer 1 ten 
   poza standardowym wypisaniem rozpoczyna kolejny cykl 
   (wysyła SIGUSR1 do nr 2) lub jeśli odbyło się już k takich cykli 
   kończy cały program. Proces rodzic ma poczekać 
   aż zakończą się procesy potomne. 
   
Programu nie można przerwać klawiszami C-c.

Liczby 1<n<10 i 1<k<10 są parametrami wywołania programu. 
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

int setaction( void (*f)(int, siginfo_t *, void *), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

void alrm_handler(int sig)
{
	fprintf(stderr,"Alarm ! \n");
}


void usage()
{
	fprintf(stderr,"main \n");
	fprintf(stderr,"... \n");
}

int main(int argc, char** argv)
{
	struct timeval current_timeval = {1, 500000};
	struct timeval next_timeval = {0, 500000};
	// first alarm will yeild after 1.5sec, next once after 0.5sec
	struct itimerval it = {next_timeval, current_timeval};
	setitimer(ITIMER_REAL,&it,NULL);
	
	sethandler(alrm_handler, SIGALRM);
	
	while(1) {
		//fprintf(stderr,"sleeping... \n");
		sleep(2);
	}

	fprintf(stderr,"Terminating \n");
	
	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	exit(EXIT_SUCCESS);
}

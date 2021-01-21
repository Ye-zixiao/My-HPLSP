#include "MyUNP.h"


void* sig_handler_thread(void* args) {
	sigset_t* psigset = (sigset_t*)args;
	int err, signo;
	for (;;) {
		if ((err = sigwait(psigset, &signo)) != 0)
			err_exit(err, "sigwait error");
		switch (signo) {
		case SIGQUIT:
			printf("\nquitting...\n");
			exit(EXIT_SUCCESS);
			break;//can't get it 
		case SIGINT:
			printf("\nI received SIGINT\n");
			break;
		default:
			break;//can't get it
		}
	}
}


int main(int argc, char* argv[]) {
	sigset_t sigset;
	pthread_t tid;
	int err;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	if ((err = pthread_sigmask(SIG_BLOCK, &sigset, NULL)) != 0)
		err_exit(err, "pthread_sigmask error");
	if ((err = pthread_create(&tid, NULL, sig_handler_thread, &sigset)) != 0)
		err_exit(err, "pthread_create error");

	pause();
	exit(EXIT_SUCCESS);
}
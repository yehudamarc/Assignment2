#include "types.h"
#include "stat.h"
#include "user.h"


int f = 0;
void handler (int a) { 
	// printf(1, "%s\n", "shit");
	// f = 1;
}


int fib(int n) 
{ 
    if (n <= 1) 
        return n; 
    return fib(n-1) + fib(n-2); 
}

int
main(int argc, char *argv[])
{
	//----- Test for handling Signals --------------
	printf(1, "%s\n", "Just testing...");

	struct sigaction* act1;
	struct sigaction* act2;
	// struct sigaction* act3;
	act1 = malloc(sizeof(struct sigaction*));
	act2 = malloc(sizeof(struct sigaction*));
	// act3 = malloc(sizeof(struct sigaction*));
	printf(1, "%s%d\n", "act 1 malloc: ", act1);
	printf(1, "%s%d\n", "act 2 malloc: ", act2);

	
	act1->sa_handler = &handler;
	printf(1, "%s%d\n", "act1 sa_handler: " ,act1->sa_handler);

	if(sigaction(20, act1, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");


	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		exit();
	}
	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		kill(3, 20);

		while(i--){
			fib(5);
			dummy+=i;	
		}
		exit();
	}

	wait();
	wait();

	printf(1, "%s%d\n", "Value of f is: ", f);


	/*
	uint ibit = 0;
	uint pending = 8;
	for(int i = 0; i < 32; i++){
		ibit = (pending & (1u << i)) >> i;
		printf(1, "%s%d%s%d\n", "i = ", i, " ibit = ", ibit);
	}
	*/

	//-----	Test for sigaction system call ---------
/*	
	if(f)
	printf(1, "%s\n", "boolean test failed");

	struct sigaction* act1;
	struct sigaction* act2;
	struct sigaction* act3;
	act1 = malloc(sizeof(struct sigaction*));
	act2 = malloc(sizeof(struct sigaction*));
	act3 = malloc(sizeof(struct sigaction*));
	printf(1, "%s%d\n", "act 1 malloc: ", act1);
	printf(1, "%s%d\n", "act 2 malloc: ", act2);

	
	act1->sa_handler = &handler;
	printf(1, "%s%d\n", "act1 sa_handler: " ,act1->sa_handler);

	if(sigaction(10, act1, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");


	getpid();

	if(f)
		printf(1, "%s\n", "boolean changed!");



	if(sigaction(17, act3, act2) == -1)
		printf(1, "%s\n", "Successfull catch!");

	if(sigaction(10, act3, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");

	if(act2 == ((void*) 0))
		printf(1, "%s\n", "act2 is NULL");

	act2->sa_handler(10);

*/

	//----- Test for sigprocmask system call -------
	/*
	uint old_mask;
	uint new_mask;
	uint mask = 2020;
	printf(1, "%d\n", mask);

  	old_mask = sigprocmask(mask);
  	printf(1, "%d\n", old_mask);

  	new_mask = getpid();
  	printf(1, "%d\n", new_mask);
	*/
	
  exit();
}

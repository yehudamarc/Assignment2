#include "types.h"
#include "stat.h"
#include "user.h"


void handler (int a) { 
	printf(1, "%s\n", "shit");
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

	//-----	Test for sigaction system call ---------

	printf(1, "%s\n", "sigaction test");

	struct sigaction* act1;
	struct sigaction* act2;
	struct sigaction* act3;
	act1 = malloc(sizeof(struct sigaction*));
	act2 = malloc(sizeof(struct sigaction*));
	act3 = malloc(sizeof(struct sigaction*));

	act1->sa_handler = &handler;	

	if(sigaction(10, act1, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");
	if(act2->sa_handler != 0)
		printf(1, "%s\n", "sigaction test failed");

	// Try to modify SIGSTOP
	if(sigaction(17, act3, act2) != -1)
		printf(1, "%s\n", "sigaction test failed");

	// Make sure act2 recieved old act, and that it was act1
	if(sigaction(10, act3, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");
	if(act2->sa_handler != &handler)
		printf(1, "%s\n", "sigaction test failed");

	printf(1, "%s\n", "sigaction test ok");

	//-------- Test Kernel Space signals --------------

	printf(1, "%s\n", "kernel space signals test");

	int parent_pid = getpid();

	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		// Send SIGSTOP to parent
		kill(parent_pid, 17);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		// Send SIGCONT to parent
		kill(parent_pid, 19);

		// Send SIGKILL to itself
		kill(getpid(), 9);
	}

	wait();

	printf(1, "\n%s\n", "kernel space signals test ok");

	// ---------- CAS alloc test ----------------------

	printf(1, "%s\n", "CAS alloc test");

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

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}
		exit();
	}

	wait();
	wait();
	wait();

	printf(1, "%s\n", "CAS alloc test ok");


	// ---------- General test 1 ----------------------
	/*	
	printf(1, "%s\n", "Test 1 Running...");

	int pid1 = fork();
	if(pid1 == 0){
		int i = 1000000;
		int dummy = 0;

		sleep(10);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}
		exit();
	}

	int pid2 = fork();
	if(pid2 == 0){
		int i = 1000000;
		int dummy = 0;

		// Send SIGSTOP
		kill(pid1, 17);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		// exit();
	}

	int pid3 = fork();
	if(pid3 == 0){
		int i = 1000000;
		int dummy = 0;

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		// Send SIGCONT
		kill(pid1, 19);
		// Seng SIGKILL
		kill(pid2, 9);

		exit();
	}

	wait();
	wait();
	wait();

	printf(1, "%s\n", "Test 1 ok");
	sleep(100);
	*/
	// ---------- General test 2 ----------------------
	/*
	printf(1, "%s\n", "Test 2 Running...");

	int pid4 = fork();
	if(pid4 == 0){
		int i = 1000000;
		int dummy = 0;

		sleep(10);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}
		exit();
	}

	int blah = 0;
	int i = 5;
	while(i--){
		if(blah == 0)
			blah = fork();
	}

	int pid5 = fork();
	if(pid5 == 0){
		int i = 1000000;
		int dummy = 0;

		// Send SIGSTOP
		kill(pid4, 17);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		// exit();
	}

	int pid6 = fork();
	if(pid6 == 0){
		int i = 1000000;
		int dummy = 0;

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		// Send SIGCONT
		kill(pid4, 19);
		// Seng SIGKILL
		kill(pid5, 9);

		exit();
	}

	wait();
	wait();
	wait();
	wait();
	wait();

	printf(1, "%s\n", "Test 2 ok");
	*/

	// ---------- Test for user space signals --------------

	printf(1, "%s\n", "user space signals test");

	if(sigaction(20, act1, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");

	parent_pid = getpid();

	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		kill(parent_pid, 20);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		exit();
	}

	wait();

	

	//----- Test for handling Signals --------------
	/*
	printf(1, "%s\n", "Just testing...");

	struct sigaction* act1;
	struct sigaction* act2;
	// struct sigaction* act3;
	act1 = malloc(sizeof(struct sigaction*));
	act2 = malloc(sizeof(struct sigaction*));
	// act3 = malloc(sizeof(struct sigaction*));
	printf(1, "%s%d\n", "act 1 malloc: ", act1);
	printf(1, "%s%d\n", "act 2 malloc: ", act2);

	printf(1, "%s%d\n", "handler is: ", handler);
	printf(1, "%s%d\n", "&handler is: ", &handler);
	
	act1->sa_handler = &handler;
	printf(1, "%s%d\n", "act1 sa_handler: " ,act1->sa_handler);

	if(sigaction(20, act1, act2) == -1)
		printf(1, "%s\n", "Error in sigaction!");

	int father_pid = getpid();

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

		kill(father_pid, 20);

		while(i--){
			fib(5);
			dummy+=i;	
		}
		exit();
	}

	wait();
	wait();

	printf(1, "%s%d\n", "Value of f is: ", f);

	*/

	/*
	uint ibit = 0;
	uint pending = 8;
	for(int i = 0; i < 32; i++){
		ibit = (pending & (1u << i)) >> i;
		printf(1, "%s%d%s%d\n", "i = ", i, " ibit = ", ibit);
	}
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

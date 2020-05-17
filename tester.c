#include "types.h"
#include "stat.h"
#include "user.h"

#define NULL ((void*) 0)

void handler1 (int a) { 
	printf(1, "%s\n", "handler1");
}

void handler2 (int a) { 
	printf(1, "%s\n", "handler2");
}

void failPrint (int a) { 
	printf(1, "%s\n", "Test failed!");
}

int fib(int n) 
{ 
    if (n <= 1) 
        return n; 
    return fib(n-1) + fib(n-2); 
}


void handler3 (int a) { 
	printf(1, "%s\n", "Handler3 starting...");

		int i = 10;
		int dummy = 0;

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

	printf(1, "%s\n", "Handler3 finished");
}

int
main(int argc, char *argv[])
{

	//-----	Test for sigaction system call ---------

	printf(1, "%s\n", "sigaction test");

	struct sigaction* act1;
	struct sigaction* act2;
	struct sigaction* act3;
	struct sigaction* act4;
	act1 = malloc(sizeof(struct sigaction*));
	act2 = malloc(sizeof(struct sigaction*));
	act3 = malloc(sizeof(struct sigaction*));
	act4 = malloc(sizeof(struct sigaction*));

	act1->sa_handler = &handler1;	

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
	if(act2->sa_handler != &handler1)
		printf(1, "%s\n", "sigaction test failed");

	// Try to call sigaction with null act
	if(sigaction(10, NULL, act2) != -1)
		printf(1, "%s\n", "sigaction test failed");

	// Make sure it works with null oldact
	if(sigaction(10, act1, NULL) != 0)
		printf(1, "%s\n", "sigaction test failed");

	printf(1, "%s\n\n", "sigaction test ok");
	sleep(50);

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

	printf(1, "%s\n\n", "kernel space signals test ok");
	sleep(50);

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

	printf(1, "%s\n\n", "CAS alloc test ok");
	sleep(100);

	// ---------- Test for user space signals --------------

	printf(1, "%s\n", "user space signals test 1");

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

	printf(1, "%s\n\n", "user space signals test 1 ok");
	sleep(50);

	// ------------ Userspace signals test 2 ----------------------

	printf(1, "%s\n", "user space signals test 2");

	act2->sa_handler = &handler2;

	if(sigaction(25, act1, NULL) == -1)
		printf(1, "%s\n", "Error in sigaction!");
	if(sigaction(26, act2, NULL) == -1)
		printf(1, "%s\n", "Error in sigaction!");

	parent_pid = getpid();

	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		kill(parent_pid, 25);
		kill(parent_pid, 26);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		exit();
	}

	wait();

	printf(1, "%s\n\n", "user space signals test 2 ok");
	sleep(50);

	// --------------- Mixed signals test ------------------
	
	printf(1, "%s\n", "Mixed signals test");

	parent_pid = getpid();

	if(fork() == 0){
		int i = 1000000;
		int dummy = 0;

		kill(parent_pid, 25);
		kill(parent_pid, 17);

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

		kill(parent_pid, 19);
		kill(parent_pid, 26);

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		exit();
	}

	wait();
	wait();

	printf(1, "%s\n\n", "Mixed signals test ok");
	sleep(50);

	//----- Test for masking signals with handler -------
	
	printf(1, "%s\n", "Masking signals test");

	uint mask = 2020;
  	if(sigprocmask(mask) != 0)
  		printf(1, "%s\n", "Masking signals test failed!");
  	if(sigprocmask(mask) != mask)
  		printf(1, "%s\n", "Masking signals test failed!");

  	act3->sa_handler = &handler3;
  	act3->sigmask = 138412032;	// blocking signal 22 and 27
  	sigaction(21, act3, NULL);

  	act3->sa_handler = (void*) 17;	// SIGSTOP
  	act3->sigmask = 0;
  	sigaction(22, act3, NULL);

  	act3->sa_handler = (void*) 19;	// SIGCONT
  	act3->sigmask = 0;
  	sigaction(27, act3, NULL);

  	int pid1 = fork();

  	if(pid1 == 0){
		int i = 10000;
		int dummy = 0;
		
		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		printf(1, "%s\n", "Continued...");

		exit();
	}

	if(fork() == 0){
		int i = 100;
		int dummy = 0;

		kill(pid1, 21);	// handler3

		// Spend some time...
		while(i--){
			fib(5);
			dummy+=i;	
		}

		kill(pid1, 22);	// STOP

		exit();
	}

	sleep(50);
	kill(pid1, 27);	//CONT

	wait();
	wait();

	printf(1, "%s\n\n", "Masking signals test ok");
	sleep(50);

	//----- Test for sigprocmask system call and default kill -------

	printf(1, "%s\n", "Mask and default signals test");

	int pid2;

	act4->sa_handler = &failPrint;
	act4->sigmask = 0;
  	sigaction(28, act4, NULL);
	sigaction(29, act4, NULL);
	sigaction(30, act4, NULL);

	uint mask2 = 1879048192; // Block 28,29,30

	pid2 = fork();

	if(pid2 == 0)
	{
		sigprocmask(mask2);
		fib(50);
		// no exit - parent should send kill

	} else {
		fib(35);
		kill(pid2, 28);
		kill(pid2, 29);
		kill(pid2, 30);
		// shoudln't print anything
		fib(15);
		kill(pid2, 31); // Should kill child
	}

	wait();

	printf(1, "%s\n\n", "Mask and default signals test ok");

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
	
  exit();
}

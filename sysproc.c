#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{

  // myproc()->handlers[10]->sa_handler(10);

  // void (*ptr) (int) 
  // if(ptr == NULL)
  //   cprintf("ptr is null\n");
  // else{
  //   cprintf("ptr is not null\n");
  //   cprintf("%d\n", ptr);
  //   ptr(10);

  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_sigprocmask(void)
{
  int mask;
  //@TODO: check if needed to cast/change to uint
  if(argint(0, &mask) < 0)
    return -1;

  return sigprocmask(mask);
}

int
sys_sigaction(void)
{
  // cprintf("I'm here!!\n");
  int signum;
  const struct sigaction* act;
  struct sigaction* oldact;

  // Get arguments from stack
  if(argint(0, &signum) < 0)
    return -1;
  if(argptr(1, ((void*)&act),sizeof(*act)) < 0)
    return -1;
  if(argptr(2, ((void*)&oldact),sizeof(*oldact)) < 0)
    return -1;

  return sigaction(signum, act, oldact);
}

int
sys_sigret(void)
{



  return 0;
}

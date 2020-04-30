#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
// Temporarely holds process mask
uint proc_mask;

// Execute all pending signal
void
execPendings(struct trapframe *tf)
{ 
    // Check pending signals before returning to user space 
    for(int i = 0; i < 32; i++){
      // Check if the i-th bit is on
      if((myproc()->pending & (1u << i)) >> i){
        // Save initial state of mask
        proc_mask = myproc()->mask;
        // Replace with current signal mask
        myproc()->mask = myproc()->handlers[i]->mask;
        // if it's kill or stop - execute
        if(i == SIGKILL){
          sig_kill(myproc()->pid);
          myproc()->mask = proc_mask;
          continue;
        }
        if(i == SIGSTOP){
          sig_stop(myproc()->pid);
          myproc()->mask = proc_mask;
          continue;
        }
        // Else - check if masked
        if((myproc()->mask & (1u << i)) >> i){
          myproc()->mask = proc_mask;
          continue;
        }
        // Check if it's kernel space signal
        if(myproc()->handlers[i]->sa_handler == SIG_DFL){
          if(i == SIGCONT)
            sig_cont(myproc()->pid);
          else
            sig_kill(myproc()->pid);
        }
        else if(myproc()->handlers[i]->sa_handler == SIG_IGN){
          // Do nothing
        }
        else if(myproc()->handlers[i]->sa_handler == SIGKILL){
          sig_kill(myproc()->pid);
        }
        else if(myproc()->handlers[i]->sa_handler == SIGSTOP){
          sig_stop(myproc()->pid);
        }
        else if(myproc()->handlers[i]->sa_handler == SIGCONT){
          // @TODO: call to sa_handler of SIGCONT
          sig_cont(myproc()->pid);
        }
        // if it's user space program
        else{
          // Backup process trapframe
          myproc()->backup = myproc()->tf;
          // Push i (= signum)
          myproc()->tf->eax = i;
          // Push sigret as return address
          myproc()->tf->esp -= &call_sigret_end - &call_sigret;
          myproc()->tf->esp = memmove(myproc()->tf->esp, call_sigret, &call_sigret_end - &call_sigret);
          // jump to the corresponding sa_handler
          myproc()->tf->eip = myproc()->handlers[i]->sa_handler;
          }

        // Restore process mask state
        myproc()->mask = proc_mask;
        // Turn off the pending bit of the signal we handled
        myproc()->pending = (myproc()->pending ^ (1u << i));
      }
    }
}

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();

    //@TODO: Check if returns to userspace (trapret)
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}

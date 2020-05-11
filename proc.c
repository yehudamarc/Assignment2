#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
static int checkCont(struct proc *p);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}




int 
allocpid(void) 
{
  
  int pid;
  pushcli();
  do{
    pid = nextpid;
  } while(!cas(&nextpid, pid, pid+1));
  popcli();
  return pid+1;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  pushcli();

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(cas(&p->state, UNUSED, EMBRYO))
      goto found;

  popcli();
  return 0;

found:
  popcli();

  p->pid = allocpid();

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    cas(&p->state, EMBRYO, UNUSED);
    // p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // Leave room for backup trapframe
  p->backup = (struct trapframe*)p->kstack;

  // Initilize fields
  p->pending = 0;
  p->mask = 0;
  p->stopped = 0;
  p->handling_signal = 0;
  p->sigaction = 0;
  p->mask_backup = 0;
  for(int i = 0; i < 32; i++){
  	p->masksArr[i] = 0;
  	p->handlers[i] = 0;
  }

  return p;
}




//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  pushcli();

  cas(&p->state, EMBRYO, RUNNABLE);

  popcli();
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Inherit father signal mask and signal handlers
  np->mask = curproc->mask;
  for(int i = 0; i < 32; i++){
  np->handlers[i] = curproc->handlers[i];
  np->masksArr[i] = curproc->masksArr[i];
  }
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  pushcli();

  cas(&np->state, EMBRYO, RUNNABLE);

  popcli();

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  pushcli();

  // Start transition to ZOMBIE
  enum procstate old;
  do{
    old = curproc->state;
  } while(!cas(&curproc->state, old, -ZOMBIE));

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE || p->state == -ZOMBIE){
      	// wait on -ZOMBIE in wait()
        wakeup1(initproc);
    }
    }
  }

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);
  // Will be changed into ZOMBIE inside scheduler
  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  pushcli();
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      // If find zombie - transition it to unused
      // if children in transitioning, wait until it's zombie
      if(p->state == ZOMBIE || p->state == -ZOMBIE){
        // Found one.
        do{} while(!cas(&p->state, ZOMBIE, -UNUSED));
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        cas(&p->state, -UNUSED, UNUSED);
        popcli();
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      popcli();
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    pushcli();
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
      // Check if process is stopped
      if(checkCont(p))
      	continue;

      // Make sure only 1 CPU taking the process
      if(!cas(&p->state, RUNNABLE, -RUNNING))
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      // @TODO: changing CPU process can fail?
      c->proc = p;
      switchuvm(p);
      cas(&p->state, -RUNNING, RUNNING);

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // If returning from yield, it should be -RUNNABLE
      cas(&p->state, -RUNNABLE, RUNNABLE);
      // If returning from sleep, it should be -SLEEPING
      cas(&p->state, -SLEEPING, SLEEPING);
      // If returning from exit, it should be -ZOMBIE
      cas(&p->state, -ZOMBIE, ZOMBIE);

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    popcli();

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();

  pushcli();
  // acquire(&ptable.lock);  //DOC: yieldlock
  enum procstate old;
  do{
    old = p->state;
    // Changed to -RUNNABLE so it won't be running while in
    // sched, change to RUNNABLE when returns to scheduler
  } while(!cas(&p->state, old, -RUNNABLE));

  sched();
  popcli();
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  popcli();
  // release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Start transition to SLEEPING
  if(!cas(&p->state, RUNNING , -SLEEPING))
    panic("sleep: process not running");
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    pushcli();
    release(lk);  //DOC: sleeplock1
  }
  // Go to sleep.
  void* old_chan1;
  do{
    old_chan1 = p->chan;
    // @TODO: make sure cast is okay
  } while(!cas(&p->chan, (int)old_chan1, (int)chan));

  // Will change to SLEEPING inside scheduler
  sched();

  // Tidy up.
  void* old_chan2;
  do{
    old_chan2 = p->chan;
  } while(!cas(&p->chan, (int)old_chan2, 0));

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    popcli();
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if((p->state == SLEEPING || p->state == -SLEEPING) && p->chan == chan){
      // If in transition to sleep, wait for it to end
      while(p->state == -SLEEPING){}
      // Try only once, maybe other process awakened it already
      cas(&p->state, SLEEPING, RUNNABLE);
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  pushcli();
  wakeup1(chan);
  popcli();
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid, int signum)
{
  // cprintf("in kill!\n");
  // cprintf("%s%d\n", "pid: ", pid);
  // cprintf("%s%d\n", "signum: ", signum);
  // cprintf("%s%d\n", "proc pid: ", myproc()->pid);
  // cprintf("%s%d\n", "proc pending: ", myproc()->pending);

  struct proc *p;
  if(signum < 0 || signum > 31)
    return -1;

  pushcli();
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      cprintf("%s%d\n", "p pending before: ", p->pending);
      // Update pending bit atomically
      uint old_pending;
      do{
        old_pending = p->pending;
      } while(!cas(&p->pending, old_pending, (p->pending | (1u << signum))));

      cprintf("%s%d\n", "p pending after: ", p->pending);
      
      // Wake process from sleep if necessary
      if(signum == SIGKILL && (p->state == SLEEPING || p->state == -SLEEPING)){
          // If in transition to sleep, wait for it to end
          while(p->state == -SLEEPING){}
          cas(&p->state, SLEEPING, RUNNABLE);
      }
      popcli();

      return 0;
    }
  }
  popcli();
  // release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Change process signal mask
// return old signal mask value
uint
sigprocmask(uint mask){

  pushcli();
  uint old_mask;
  do{
    old_mask = myproc()->mask;
  } while(!cas(&myproc()->mask, old_mask, mask));

  popcli();
  return old_mask;
}

int
sigaction (int signum, const struct sigaction* act, struct sigaction* oldact)
{
  // if trying to modify SIGKILL or SIGSTOP - fail
  if(signum == SIGKILL || signum == SIGSTOP)
    return -1;
  // No such signal
  if(signum < 0 || signum > 31)
    return -1;
  if(act == NULL)
  	return -1;

  struct proc *p = myproc();

  // Make sure the it's the only one changing process handlers
  pushcli();
  while(!cas(&p->sigaction, 0, 1)){}

  // Save the old handler
  if(oldact != NULL){
    oldact->sa_handler = p->handlers[signum];
    oldact->sigmask = p->masksArr[signum];
  }

  p->handlers[signum] = act->sa_handler;
  p->masksArr[signum] = act->sigmask;

  p->sigaction = 0;
  popcli();
  return 0;
}
int fib(int n) 
{ 
    if (n <= 1) 
        return n; 
    return fib(n-1) + fib(n-2); 
}

void
sigret (void)
{
	struct proc *p = myproc();

	// Realese user space signal lock
   p->handling_user_signal = 0;

  cprintf("I'm in sigret!\n");
  // Restore process mask state
   p->mask = p->mask_backup;

   
  *p->tf = *p->backup;
  // memmove(myproc()->tf, &myproc()->backup, sizeof(struct trapframe));
}

static int
checkCont(struct proc *p)
{
	// If p in stopped state and there isn't SIGCONT pending
    int ret = 0;
     if(p->stopped == 1){
      ret = 1;
      for(int i = 0; i < 32; i++){
      	// Check if there is pending signal with handler SIGCONT which is not masked 
      	if(p->handlers[i] == (void *)SIGCONT && ((p->pending & (1u << i)) >> i) && !((p->mask & (1u << i)) >> i))
      			ret = 0;
      	// Check if SIGCONT is pending and not masked
      	if(p->handlers[SIGCONT] == SIG_DFL && ((p->pending & (1u << SIGCONT)) >> SIGCONT) && !((p->mask & (1u << SIGCONT)) >> SIGCONT))
      		ret = 0;
      }
     }
      return ret;
}
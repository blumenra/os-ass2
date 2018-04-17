#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int setSignal(struct proc *p, int signum, int swtch);
int setSigFaild(struct proc *p, int signum, int swtch);
int sigFaild(int signum, sighandler_t handler);
int turnOnSignal(struct proc *p, int signum);
int turnDownSignal(struct proc *p, int signum);
int isSignalOn(struct proc *p, int signal);
int handleSigKill(struct proc *p);
int handleSigStop(struct proc *p);
int handleSigCont(struct proc *p);
int isValidSig(int signum);
int isMaskOn(struct proc *p, int sig);
void handleUserModeSigs(struct proc *p, int sig);
void handlePendingSigs(/*???*/);
  

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

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
  // Original Impl:
    // int pid;
    // acquire(&ptable.lock);
    // pid = nextpid++;
    // release(&ptable.lock);
    // return pid;


  int old_pid;
  do{
    
    old_pid = nextpid;

  } while(!cas(&nextpid, old_pid, old_pid+1)); // if nextpid == old_pid then nextpid = old_pid+1 (i.e, nextpid++)

  return old_pid+1;
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

  // Original Impl:
    // acquire(&ptable.lock);

    // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    //   if(p->state == UNUSED)
    //     goto found;

    // release(&ptable.lock);
    // return 0;

  /*
    run over the ptable and find the first unsued proc.
    if found the break from the for loop and do cas.
    The thing is that if the state of p remains UNSUSED until cas, it will change to EMBRYO and get out of the while loop.
    otherwise, the state was changed until cas and then cas will fail and we will look for another UNUSED proc from the beginning.
    if we went all over the ptable and didnt find an UNUSED proc, it means that the table is full of necessary procs and the allocproc function should fail (return 0).
  */
  pushcli(); // disable interrupts to avoid interuppt handeling during the following code so that the code will be atomic.
  do {
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      if(p->state == UNUSED)
        break;
    if (p == &ptable.proc[NPROC]) {
      popcli();
      return 0; // ptable is full
    }
  } while (!cas(&p->state, UNUSED, EMBRYO));
  popcli();

  // Original Impl:
    // found:
    // p->state = EMBRYO;
    // release(&ptable.lock);
  p->pid = allocpid();


  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
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


  //SIGNAL HANDLING
  p->pending_sigs = 0;
  p->sig_masks = 0;

  for(int i=0; i < NUM_OF_SIG_HANDLERS; i++){

    p->sig_handlers[i] = (void*) SIG_DFL;
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
  



  //acquire(&ptable.lock);
  pushcli();
  p->state = RUNNABLE;
  popcli();
  //release(&ptable.lock);
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

  //COPY SIGNALS MASK AND HANDLERS
  np->sig_masks = curproc->sig_masks;
  for(int k=0; k < NUM_OF_SIG_HANDLERS; k++)
    np->sig_handlers[k] = curproc->sig_handlers[k];

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  //acquire(&ptable.lock);
  pushcli();
  np->state = RUNNABLE;
  //release(&ptable.lock);
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

  //acquire(&ptable.lock);
  pushcli();
  if(!cas(&curproc->state, RUNNING, NEG_ZOMBIE))
    panic("exit: cas #1 failed");

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  //curproc->state = ZOMBIE;
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
  
  //acquire(&ptable.lock);
  pushcli();
  for(;;){
    // Scan through table looking for exited children.
    if (!cas(&curproc->state, RUNNING, NEG_SLEEPING))
      panic("scheduler: cas failed");

    curproc->chan = curproc;

    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(cas(&p->state, ZOMBIE, NEG_UNUSED)){
        // Found one.
        pid = p->pid;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        curproc->chan = 0;
        cas(&curproc->state, NEG_SLEEPING, RUNNING);

        cas(&p->state, NEG_UNUSED, UNUSED);
        popcli();
        //release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      //release(&ptable.lock);
      curproc->chan = 0;
      if (!cas(&curproc->state, NEG_SLEEPING, RUNNING))
        panic("Inside wait(): state must be NEG_SLEEPING");
      popcli();
      return -1;
    }

    sched();
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    //sleep(curproc, &ptable.lock);  //DOC: wait-sleep
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
    // acquire(&ptable.lock);
    pushcli();
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(!cas(&p->state, RUNNABLE, RUNNING)) { // Find the first RUNNABLE proc and change its state to RUNNING
        continue;
      }

      if(isSignalOn(p, SIGSTOP)){
        if(isSignalOn(p, SIGCONT)){
          setSignal(p, SIGSTOP, 0); //turn off SIGSTOP
          setSignal(p, SIGCONT, 0); //turn off SIGCONT
        }
        else{
          continue;
        }
      }

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      // p->state = RUNNING;  // ORIGINALLY WAS HERE

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;

      // Change each negative state to positive
      if (cas(&p->state, NEG_SLEEPING, SLEEPING)) {
        // TODO: Find out if the following transition is necessary
        if (cas(&p->killed, 1, 0))
          p->state = RUNNABLE;
      }
      if (cas(&p->state, NEG_RUNNABLE, RUNNABLE)) {
      }
      if (p->state == NEG_ZOMBIE) {
        
          kfree(p->kstack);
          p->kstack = 0;
          freevm(p->pgdir);
          p->killed = 0;
          p->chan = 0;
        if (cas(&p->state, NEG_ZOMBIE, ZOMBIE))
          wakeup1(p->parent);
      }
    }
    // release(&ptable.lock);
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

  // if(!holding(&ptable.lock))
  //   panic("sched ptable.lock");
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
  //cquire(&ptable.lock);  //DOC: yieldlock
  //myproc()->state = RUNNABLE;
  pushcli();
  if (!cas(&myproc()->state, RUNNING, NEG_RUNNABLE))
    panic("Inside yield: The process state is not RUNNING! BAD! SAD!");
  sched();
  popcli();
  //release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  //release(&ptable.lock);
  popcli();

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
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");


  pushcli();
  p->chan = chan;

  // Go to sleep.
  if (!cas(&p->state, RUNNING, NEG_SLEEPING))
    panic("sleep: cas failed");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    // acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  sched();

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    // release(&ptable.lock);
    acquire(lk);
  }
  popcli();



  // //OUR Impl. If we choose the following impl, we should remove the original code in below our impl..
  // pushcli();
  // if(lk == &ptable.lock){  //DOC: sleeplock0
  //   release(&ptable.lock);  //DOC: sleeplock1
  //   p->chan = chan;
    
  //   if(!cas(&p->state, RUNNING, NEG_SLEEPING))
  //     panic("");
    

  //   // sched();

  //   p->chan = 0;
  // }
  // popcli();




  // // Must acquire ptable.lock in order to
  // // change p->state and then call sched.
  // // Once we hold ptable.lock, we can be
  // // guaranteed that we won't miss any wakeup
  // // (wakeup runs with ptable.lock locked),
  // // so it's okay to release lk.
  // if(lk != &ptable.lock){  //DOC: sleeplock0
  //   acquire(&ptable.lock);  //DOC: sleeplock1
  //   release(lk);
  // }
  // // Go to sleep.
  // p->chan = chan;
  // p->state = SLEEPING;

  // sched();

  // // Tidy up.
  // p->chan = 0;

  // // Reacquire original lock.
  // if(lk != &ptable.lock){  //DOC: sleeplock2
  //   release(&ptable.lock);
  //   acquire(lk);
  // }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  //   if(p->state == SLEEPING && p->chan == chan)
  //     p->state = RUNNABLE;


  // Find all procs which are sleeping on channel chan and wake them up
  // by changing thair state to RUNNABLE
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->chan == chan) {
      while (p->state == NEG_SLEEPING) { // If NEG_SLEEPING, wait until the state chges
        // busy-wait
      }
      if(p->state == SLEEPING){
        if (cas(&p->state, SLEEPING, NEG_RUNNABLE)) {
          

          p->chan = 0;
          
          // Change state to RUNNABLE here and not wait for scheduler() to change
          // after the context switch because we want to wake them up imidiatley
          if (!cas(&p->state, NEG_RUNNABLE, RUNNABLE))
            panic("wakeup1: cas failed");
        }
      }
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  //acquire(&ptable.lock);
  pushcli();
  wakeup1(chan);
  popcli();
  //release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid, int signum)
{
  struct proc *p;

  //acquire(&ptable.lock);
  pushcli();
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){

      setSignal(p, signum, 1);

      /*
      * should be moved to sigkill handler
      */
      p->killed = 1;


      // Wake process from sleep if necessary.
      //if(p->state == SLEEPING)
        //p->state = RUNNABLE;
      
      // didnt add NEG_SLEEPING case because it's handled in schedular() after the context switch
      
      /*
      * should be moved to sigkill handler
      */
      cas(&p->state, SLEEPING, RUNNABLE);
      

      //release(&ptable.lock);
      popcli();
      return 0;
    }
  }
  //release(&ptable.lock);
  popcli();
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
  [UNUSED]       "unused",
  [NEG_UNUSED]   "neg_unused",
  [EMBRYO]       "embryo",
  [SLEEPING]     "sleep ",
  [NEG_SLEEPING] "neg_sleep ",
  [RUNNABLE]     "runble",
  [NEG_RUNNABLE] "neg_runnable",
  [RUNNING]      "run   ",
  [ZOMBIE]       "zombie",
  [NEG_ZOMBIE]   "neg_zombie"
  };
  //int i;
  struct proc *p;
  char *state;
  //uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    // if(p->state == SLEEPING){
    //   getcallerpcs((uint*)p->context->ebp+2, pc);
    //   for(i=0; i<10 && pc[i] != 0; i++)
    //     cprintf(" %p", pc[i]);
    // }
    cprintf("\n");
  }
}

uint
sigprocmask(uint sigmask){

  uint oldSigmask = myproc()->sig_masks;

  myproc()->sig_masks = sigmask;

  return oldSigmask;
}

sighandler_t
signal(int signum, sighandler_t handler){

  struct proc *p = myproc();

  if(sigFaild(signum, handler))
    return (sighandler_t)-2;

  sighandler_t oldHandler = p->sig_handlers[signum];
  p->sig_handlers[signum] = handler;

  return oldHandler;
}

void
sigret(void){
  //cprintf("Inside sigret:)))");
}

int
sigFaild(int signum, sighandler_t handler) {

  if(!isValidSig(signum))
    return 1;
  
  //TODO: IMPLEMENT once we know what it means that signal() fails
  
  return 0;
}

int
setSignal(struct proc *p, int signum, int swtch){

  if(setSigFaild(p, signum, swtch)){

    return -1;
  }

  if(swtch)
    return turnOnSignal(p, signum);
  else
    return turnDownSignal(p, signum);
}

/*
*DO NOT USE THIS DIRECTLY, BUT ONLY BY setSignal(..., 1)
*/
int
turnOnSignal(struct proc *p, int signum){
  
  if(!isValidSig(signum))
    return 0;

  p->pending_sigs |= 1 << signum;
  return 1;
}

/*
*DO NOT USE THIS DIRECTLY, BUT ONLY BY setSignal(..., 0)
*/
int
turnDownSignal(struct proc *p, int signum){

  if(!isValidSig(signum))
    return 0;

  p->pending_sigs &= ~(1 << signum);
  return 1;
}

int
setSigFaild(struct proc *p, int signum, int swtch){
  
  if(!isValidSig(signum)|| swtch < 0)
    return 1;

  if(p == 0)
    return 1;

  return 0;
}

int
isSignalOn(struct proc *p, int signal){
  
  uint ret = (p->pending_sigs >> signal) & 1;
  return ret;
}

int
sigKillDefaultHandle(struct proc *p){
  
  p->killed = 1;
  cas(&p->state, SLEEPING, RUNNABLE);

  return 0;
}

int
sigStopDefaultHandle(struct proc *p){
  
  return 0;
}

int
sigContDefaultHandle(struct proc *p){
  
  return 0;
}

int
isValidSig(int signum){

  return 0 <= signum && signum < 32;
}

void
handlePendingSigs(/*???*/){

  struct proc *p = myproc();   // = process we handle..(how to know???)
  
  for(int sig=0; sig < 32; sig++){

    if(!isSignalOn(p, sig)) // if currently iterated signal is on..
      continue;
    
    if(isMaskOn(p, sig))
      continue;

    if((int)p->sig_handlers[sig] == SIG_IGN)
      continue;
    

    switch(sig) {
      case SIGKILL:

          if((int)p->sig_handlers[sig] == SIG_DFL)
            sigKillDefaultHandle(p);
      
      case SIGSTOP:
          if((int)p->sig_handlers[sig] == SIG_DFL)
            sigStopDefaultHandle(p);

      case SIGCONT:
          if((int)p->sig_handlers[sig] == SIG_DFL)
            sigContDefaultHandle(p);
      
       default:
          if((int)p->sig_handlers[sig] == SIG_DFL)
            sigKillDefaultHandle(p);
          else
            handleUserModeSigs(p, sig);
    }
  }
}

int
isMaskOn(struct proc *p, int sig){
  
  if(!isValidSig(sig))
    panic("isMaskOn: Invalid signum");

  uint ret = (p->sig_masks >> sig) & 1;
  return ret;
} 

void
handleUserModeSigs(struct proc *p, int sig){


}
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct proc proc[];

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
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

uint64
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

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_memsize(void)
{
  return myproc()->sz;
}

uint64
sys_co_yield(void)
{
  
  int target_pid, value;
  struct proc *p = myproc();
  struct proc *target = 0;
  // Read arguments from the user trapframe
  if(argint(0, &target_pid) < 0 || argint(1, &value) < 0)
    return -1;

  // sanity checks.
  if(target_pid <= 0 || target_pid == p->pid)
    return -1;

  if(p->killed)
    return -1;

  //find the target process
  for(struct proc *pp = proc; pp < &proc[NPROC]; pp++){
    acquire(&pp->lock);
    if(pp->pid == target_pid){
      // Check if the target is killed while we were looking for it.
      if(pp->killed){
        release(&pp->lock);
        return -1;
      }
      target = pp;
      break;  // keep lock held
    }
    // we didn't find the target, release the lock and keep looking.
    release(&pp->lock);
  }
  // If we didn't find the target, or if it is in an invalid state, return -1.
  if(target == 0)
    return -1;

  // If the target is sleeping on us, we can directly wake it up and pass the value.
  if(target->state == SLEEPING && target->chan == (void*)p){
    //value from the target 
    int received_value = (int)target->trapframe->a0;
    //value of the target will be the value from the caller
    target->trapframe->a0 = (uint64)value;
    //value from the target to the caller
    p->trapframe->a0 = received_value;
    target->state = RUNNING;
    acquire(&p->lock);
    p->chan = (void*)target;
    p->state = SLEEPING;
    
    // switch to the target process and let it run.
    struct cpu *c = mycpu();
    c->proc = target;
    release(&p->lock);
    swtch(&p->context, &target->context);
    // after the target process runs and yields back to us, we will continue from here.
    c->proc = p;
    p->chan = 0;
    release(&p->lock);

    if(p->killed)
      return -1;

    return p->trapframe->a0;
  
  // Otherwise, we need to sleep on the target and wait for it to wake us up.
  } else {

    p->trapframe->a0 = (uint64)value;
    //sleep(target, &target->lock);
    acquire(&p->lock);
    p->chan = (void*)target;
    p->state = SLEEPING;
    release(&target->lock);
    sched();
    p->chan = 0;
    release(&p->lock);
    if(p->killed)
      return -1;
    // Our a0 was overwritten by the process that woke us.
    return p->trapframe->a0;
  }
}
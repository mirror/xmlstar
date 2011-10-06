#ifndef PROCESS_H
#define PROCESS_H

typedef struct Proc *ProcPtr;

/* allocates a ProcPtr */
ProcPtr proc_spawn(char **argv);

int proc_read(ProcPtr proc, void *buf, int count);
int proc_write(ProcPtr proc, void *buf, int count);

/* frees the ProcPtr */
int proc_wait(ProcPtr proc);

#endif  /* PROCESS_H */

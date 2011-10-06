#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>

typedef struct Proc {
    pid_t pid;
    int in;
    int out;
} *ProcPtr;

ProcPtr
proc_spawn(char **argv)
{
    pid_t pid;
    int infds[2], outfds[2];
    pipe(infds);
    pipe(outfds);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return NULL;
    } else if (pid == 0) {      /* child */
        dup2(infds[0], STDIN_FILENO);
        close(infds[1]);
        dup2(outfds[1], STDOUT_FILENO);
        close(outfds[0]);
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {                    /* parent */
        ProcPtr proc = xmlMalloc(sizeof(*proc));
        proc->pid = pid;
        close(infds[0]);
        proc->in = infds[1];
        close(outfds[1]);
        proc->out = outfds[0];
        return proc;
    }
}

int
proc_read(ProcPtr proc, void *buf, int count)
{
    return read(proc->out, buf, count);
}

int
proc_write(ProcPtr proc, void *buf, int count)
{
    return write(proc->in, buf, count);
}

int
proc_wait(ProcPtr proc)
{
    int status;
    close(proc->in);
    close(proc->out);

    waitpid(proc->pid, &status, 0);
    xmlFree(proc);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return WTERMSIG(status);
    else
        return -1;              /* probably should never happen */
}

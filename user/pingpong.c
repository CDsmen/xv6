#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    if (fork() == 0)
    {
        int pid = getpid();
        close(p[0]);
        printf("%d: received ping\n", pid);
        write(p[1], "A", 1);
        close(p[1]);
        exit(0);
    }
    else
    {
        int pid = getpid();
        close(p[1]);
        char buf[5];
        read(p[0], buf, 1);
        printf("%d: received pong\n", pid);
    }
    exit(0);
}

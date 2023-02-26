#include "kernel/types.h"
#include "user/user.h"

void son_do(int p[])
{
    close(p[1]);
    int last = dup(p[0]);
    close(p[0]);

    int pri;
    int status = read(last, &pri, 4);
    if (status == 0)
    {
        close(last);
        exit(0);
    }
    printf("prime %d\n", pri);
    pipe(p);
    if (fork() == 0)
    {
        son_do(p);
    }
    else
    {
        close(p[0]);
        int tmp;
        while ((status = read(last, &tmp, 4)) != 0)
        {
            if (tmp % pri == 0)
                continue;
            write(p[1], &tmp, 4);
        }
        close(last);
        close(p[1]);
        wait((int *)0);
        exit(0);
    }
}
int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    if (fork() == 0)
    {
        son_do(p);
    }
    else
    {
        close(p[0]);
        for (int i = 2; i < 36; i++)
        {
            int num = i;
            write(p[1], &num, 4);
        }
        close(p[1]);
        wait((int *)0);
    }
    exit(0);
}

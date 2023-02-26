#include "kernel/types.h"
#include "user/user.h"

char **getline(int *argc)
{
    *argc = 0;
    int siz = 0;
    static char buf[5][128], *p;
    p = buf[0];
    while (read(0, p, 1) == 1 && *p != '\n')
    {
        // printf("lchar: %d(%c)\n", *p, *p);
        if (*p == ' ')
        {
            if (siz == 0)
                continue;
            *p = 0;
            siz = 0;
            if (++*argc == 5)
            {
                fprintf(2, "arg too many\n");
                exit(1);
            }
            p = buf[*argc];
            continue;
        }
        siz++;
        p++;
        if (siz + 1 > 128)
        {
            fprintf(2, "arg too long\n");
            exit(1);
        }
    }
    if (*p == '\n')
    {
        *p = 0;
        ++*argc;
    }

    static char *res[5];
    for (int i = 0; i < *argc; i++)
    {
        res[i] = buf[i];
    }
    return res;
}
int main(int argc, char *argv[])
{
    // char *na[] = {"echo", "hello", "world", 0};
    // exec("echo", na);
    // exit(0);

    if (argc < 2)
    {
        fprintf(2, "arg low\n");
        exit(0);
    }
    int num = 0, argc2 = argc - 1;
    char **p;
    char buf[10][128];
    char *argv2[10];
    for (int i = 0; i < 10; i++)
    {
        argv2[i] = buf[i];
        if (i + 1 < argc)
        {
            memcpy(buf[i], argv[i + 1], sizeof(argv[i + 1]));
        }
    }

    while ((p = getline(&num)) && num != 0)
    {
        // printf("num: %d\n", num);
        if (fork() != 0)
        {
            wait(0);
            continue;
        }
        if (argc2 + num > 9)
        {
            fprintf(2, "arg too many\n");
            exit(0);
        }
        for (int i = 0; i < num; i++)
        {
            memcpy(buf[argc2 + i], p[i], sizeof(p[i]));
            // printf("%s\n", p[i]);
        }
        argc2 += num;
        // printf("new fork:\n");
        // for (int i = 1; i < argc2; i++)
        // {
        //     printf("%s%c", argv2[i], " \n"[i == argc2 - 1]);
        // }
        argv2[argc2] = 0;
        exec(argv2[0], argv2);
        exit(0);
    }
    // printf("num: %d\n", num);
    exit(0);
}

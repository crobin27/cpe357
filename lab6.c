#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>

void sigHandler(int sig);

int main(int argc, char *argv[])
{
    int Ppid = getpid();
    int *Cpid = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    signal(SIGINT, sigHandler);
    signal(SIGQUIT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGTSTP, sigHandler);

    //pretty sure these signals are unblockable but I'll account for them anyways
    signal(SIGSTOP, sigHandler);
    signal(SIGKILL, sigHandler);

    while (1)
    {
        printf("Parent's PID: %d\n", Ppid);
        if (fork() == 0)
        {
            while (1)
            {
                //child process id
                *Cpid = getpid();
                printf("\nChild's PID: %d\n", *Cpid);

                //print time
                time_t T = time(NULL);
                struct tm tm = *localtime(&T);
                printf("Current Time: %d:%d\n", tm.tm_hour, tm.tm_min);

                //print directory info
                DIR *dir;
                dir = opendir(".");
                struct dirent *entry;
                printf("Current Directory: \n");
                while ((entry = readdir(dir)) != NULL)
                {
                    printf("%s \n", entry->d_name); //print contents in current directory
                }
                printf("\n");
                closedir(dir);
                //wait 10s
                sleep(10);
            }
            *Cpid = 0;
            return 0;
        }

        else
        {
            int pid = wait(0);
            if (*Cpid == pid)
            {
                printf("\nI can respawn ;)\n");
            }
        }
    }
}

void sigHandler(int sig)
{
    printf("\nnice try sucka! \n");
}

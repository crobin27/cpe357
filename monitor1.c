#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>

void printfileinfo(struct stat sb);
void navigate(char *path, char *desired);
void printdir(char *path);
char *changedir(char *path, char *desired, char *currentdirectory);
void sigHandler(int sig);

int main(int argc, char *argv[])
{
    signal(SIGINT, sigHandler);
    signal(SIGSTOP, sigHandler);
    signal(SIGQUIT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGTSTP, sigHandler);

    char *shortened = mmap(NULL, sizeof(char) * 100, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    char *currentdirectory = mmap(NULL, sizeof(char) * 500, PROT_WRITE | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *childpid = (int*)mmap(NULL, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    char *fileName = mmap(NULL, sizeof(char) * 100, PROT_WRITE | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int parentpid = getpid();
    currentdirectory = "."; //start at directory currently in
while(1){
    if (fork() == 0)
    {
        while (1)
        {
            *childpid = getpid();
            printf("\033[0;34m");
            printf("./monitor1");
            printf("%s", shortened);
            printf("\033[0m");
            printf(" $ ");
            int valid = scanf("%s", fileName);
            if (strcmp(fileName, "list") == 0)
                printdir(currentdirectory);
            else if (strcmp(fileName, "q") == 0)
            {
                kill(parentpid, SIGKILL);
                return 0;
            }
            /*
                else if (fileName[0] == '/')
                {
                    //this would be if input is a directory
                    currentdirectory = changedir(currentdirectory, fileName, shortened);
                }*/
            else if (strcmp(fileName, currentdirectory) == 0)
            {
                //this would be to go back a directory
            }
            else
            {
                //find the file or report not there
                navigate(currentdirectory, fileName);
            }
        }
        return 0;
    }
    else
    {
        while (1)
        {
            char *temp1 = malloc(sizeof(char) * 100);
            char *temp2 = malloc(sizeof(char) * 100);
            strcpy(temp1, fileName);
            sleep(10);
            strcpy(temp2, fileName);
            if (strcmp(temp1, temp2) == 0)
            {
                kill(*childpid, SIGKILL);
                wait(0);
                break;
            }
        }
    }
}
    wait(0);
    return 0;
}

/*
char *changedir(char *path, char *desired, char *directory)
{
    desired = &(desired[1]);
    char *temp = path;
    DIR *current = opendir(temp);
    if (!current)
        return NULL;
    struct dirent *entry;
    while ((entry = readdir(current)) != NULL)
    {
        struct stat sb;
        stat(entry->d_name, &sb);
        if (S_ISDIR(sb.st_mode))
        {
            int valid = strcmp(entry->d_name, desired);
            if (valid == 0)
            {
                char cwd[500];
                //getcwd(cwd, 500);
                //strcat(cwd, "/");
                cwd[0] = '/';
                strcat(cwd, entry->d_name);
                closedir(current);
                directory = entry->d_name;
                char *finalptr = &(cwd[0]);
                return finalptr;
            }
        }
    }
    printf("Directory could not be found, please try again");
    return NULL;
}
*/

void printdir(char *path)
{
    DIR *current = opendir(path);
    if (!current)
        return;
    struct dirent *entry;
    while ((entry = readdir(current)) != NULL)
    {
        struct stat sb;
        stat(entry->d_name, &sb);
        printf("\nComponent Name: %s\n", entry->d_name);
        printfileinfo(sb);
        printf("\n**********************************************\n");
    }
    return;
}

void navigate(char *path, char *desired)
{
    //attemp to move into given directory
    DIR *current = opendir(path);
    if (!current)
        return;

    struct dirent *entry;
    while ((entry = readdir(current)) != NULL)
    {
        struct stat sb;
        stat(entry->d_name, &sb);
        //base case technically
        int valid = strcmp(entry->d_name, desired);
        if (valid == 0)
        {
            printf("\nComponent Name: %s\n", entry->d_name);
            printfileinfo(sb);
            closedir(current);
            return;
        }
    }
    closedir(current);
    //This area should only be reached if the file cannot be found
    printf("File could not be found, please try again\n");
    return;
}

void printfileinfo(struct stat sb)
{
    printf("ID of containing device:  [%lx,%lx]\n",
           (long)major(sb.st_dev), (long)minor(sb.st_dev));

    printf("File type:                ");

    switch (sb.st_mode & S_IFMT)
    {
    case S_IFBLK:
        printf("FIFO/pipe\n");
        break;
    case S_IFLNK:
        printf("symlink\n");
        break;
    case S_IFREG:
        printf("regular file\n");
        break;
    case S_IFSOCK:
        printf("socket\n");
        break;
    default:
        printf("unknown?\n");
        break;
    }

    printf("I-node number:            %ld\n", (long)sb.st_ino);

    printf("Mode:                     %lo (octal)\n",
           (unsigned long)sb.st_mode);

    printf("Link count:               %ld\n", (long)sb.st_nlink);
    printf("Ownership:                UID=%ld   GID=%ld\n",
           (long)sb.st_uid, (long)sb.st_gid);

    printf("Preferred I/O block size: %ld bytes\n",
           (long)sb.st_blksize);
    printf("File size:                %lld bytes\n",
           (long long)sb.st_size);
    printf("Blocks allocated:         %lld\n",
           (long long)sb.st_blocks);

    printf("Last status change:       %s", ctime(&sb.st_ctime));
    printf("Last file access:         %s", ctime(&sb.st_atime));
    printf("Last file modification:   %s", ctime(&sb.st_mtime));
}

void sigHandler(int sig)
{
    printf("\nnice try! \n");
}

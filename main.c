#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int total_level;
    sscanf(argv[1], "%d", &total_level);
    create_process(total_level - 1);
}
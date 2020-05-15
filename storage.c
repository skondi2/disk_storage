#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>          
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
    char* full_path = NULL;
    if (argc == 2) {
        full_path = argv[1];
    }

    int my_pipe[2];
    pipe(my_pipe); 

    fcntl(my_pipe[0], F_SETFL, O_NONBLOCK);
    if (full_path != NULL) {
        int switched = chdir(full_path);
        if (switched == -1) {
            perror("Invalid directory: ");
            return 1;
        }
        //char s[100];
        //printf("current directory is %s \n", getcwd(s, 100));
    }

    int child = fork();
    if (child == 0) { // child
        dup2(my_pipe[1], 1); // send du output into pipe
        close(my_pipe[1]);
        
        // du -s -h * command:
        execl("/bin/zsh", "/bin/zsh", "-c", "du -s -h *", 0);

    } else { // parent
       int status;
        waitpid(child, &status, 0); // wait for child:
        
        if (!WIFEXITED(status)) {
            printf("failed to exit child\n");
        } 
    }
    
    FILE* read_end = fdopen(my_pipe[0], "r");
    if (read_end == NULL) {
        return 1;
    }

    double max_space = 0.0;
    char max_suffix = 0;
    int max_suffix_val = 0;
    char* max_file = NULL;

    // read from read-end of pipe
    char buffer[1000];
    while (fgets(buffer, 1000, read_end) != NULL) {
        char* space = malloc(100);
        char* start_name = malloc(100);
        sscanf(buffer, "%s  %s", space, start_name);

        char* name = strstr(buffer, start_name);
        name[strlen(name) - 1] = 0;
        
        char suffix = space[strlen(space) - 1];
        space[strlen(space) - 1] = 0;
        
        double space_num = 0;
        sscanf(space, "%lf", &space_num);

        // B=Byte, K=kilobyte, M=megabyte, G=gigabyte, T=teraByte and P=petabyte
        int suffix_val = 0;
        if (suffix == 'K') suffix_val = 1;
        if (suffix == 'M') suffix_val = 2;
        if (suffix == 'G') suffix_val = 3;
        if (suffix == 'P') suffix_val = 4;

        if ((max_space <  space_num && suffix_val == max_suffix_val) || (suffix_val > max_suffix_val)) {
            // found a new max
            max_space = space_num;
            max_file = name;
            max_suffix = suffix;
            max_suffix_val = suffix_val;
        }
    }

    printf("Name: %s, Space: %lf%c\n", max_file, max_space, max_suffix);
    close(my_pipe[0]);
    fclose(read_end);

    return 0;
}
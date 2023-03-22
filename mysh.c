#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "words.h"
#include <unistd.h>
#include <sys/stat.h>

#define PROMPT "mysh> "
#define IS_EXCEPTIONS(x) (x == '|' | x == '<' | x == '>')

static char* built_in_commands[] = {"cd", "pwd"};

static char* search_paths[] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin", NULL};

struct exec_info {
    char **arguments;
    char *executable_path;
    char *input_file;
    char *output_file;
};

#define is_builtin_commands(x) (strcmp(x, "cd") == 0 | strcmp(x, "pwd") == 0)

char *readCommand() {
        char c;
        int pos = 0;
        char* command = malloc(sizeof(char) * 1024);

        // readCommand
        while (1) {
            c = getchar();    
            if (c == '\n' | c == EOF) {
                command[pos] = c;
                break;
            }
            else {
                command[pos] = c;
            }
            pos++;
        }

        return command;
}

char *get_executable_path(char *word) {
    // printf("%ld", word);s
    // if (word == '\n') {
    //     return EXIT_SUCCESS;
    // }
    if (word[0] == '/') {
        return word;
    }

    if (is_builtin_commands(word)) {
        return word;
    }

    for (int i = 0; search_paths[i] != NULL; i++) {
        char* search_path = search_paths[i];

        if (search_path == NULL) break;
        
        struct stat buffer;

        // construct a new path 
        char* executable;
        executable = malloc(strlen(search_path) + strlen(word) + 2);
        memcpy(executable, search_path, strlen(search_path));
        executable[strlen(search_path)] = '/';
        memcpy(executable + strlen(search_path) + 1, word, strlen(word));
        executable[strlen(search_path) + strlen(word) + 1] = '\0';
        
        int exist = stat(executable, &buffer);
        if (exist == 0) {
            // printf("File exists\n");
            return executable;
        }
        else {
            // printf("File does not exist\n");
        }
        
    }
    
}

void print_string_list(char **list, int num_strings) {
    int i;
    // fflush(stdout);
    putchar('{');
    for (i = 0; i < num_strings; i++) {
        // fflush(stdout);
        printf("%s, ", list[i]);
        // fflush(stdout);
    }
    
    putchar('}');
    putchar('\n');
    
}

struct exec_info* parseCommand() {
    words_init(STDIN_FILENO);
    char *word;
    char **arguments;
    char *executable_path;
    char *input_file = NULL;
    char *output_file = NULL;

    // executable
    word = words_next();

    // EOF in batch mode
    if (word == NULL) {
        return EXIT_SUCCESS;
    }

    executable_path = get_executable_path(word);

    if (strcmp(word, "exit") == 0) {
            printf("mysh: exiting\n");
            return EXIT_SUCCESS;
    }

    // command as first arg.
    arguments = malloc(sizeof(char*));
    arguments[0] = (char *) malloc(sizeof(char) * strlen(word) + 1);

    int args_pos = 1;

	while ((word = words_next())) {
        if (strcmp(word, "\n") == 0) {
            break;
        }
        
        if (strcmp(word, ">") == 0) {
            // next token set to STDOUT 
            word = words_next();
            output_file = malloc(sizeof(char) * strlen(word) + 1);
            strcpy(output_file, word);

        } else if (strcmp(word, "<") == 0) {
            // next token set to STDIN
            word = words_next();
            input_file = malloc(sizeof(char) * strlen(word) + 1);
            strcpy(input_file, word);
        } else {
            // arguments
            // printf("args\n");
            arguments = realloc(arguments, args_pos + 1 * sizeof(char*));
            arguments[args_pos] = (char *) malloc(sizeof(char) * strlen(word) + 1);
        
            strcpy(arguments[args_pos], word);
            
            ++args_pos;
        }

        free(word);
        // word = NULL;
	}

    arguments[args_pos] = NULL;

    struct exec_info* info = malloc(sizeof(struct exec_info));

    info->arguments = arguments;
    info->executable_path = executable_path;
    info->input_file = input_file;
    info->output_file = output_file;

    return info;
}

void mysh_cd(char **args) {
    if (chdir(args[1]) != 0) {
        perror("!mysh> ");
    }
}

// void mysh_pwd(char **args) {

//     if (sizeof(args) > 1) {
//         perror("!mysh> ");
//     }

//     getcwd(stdout, 1024);
// }

int execute_command(struct exec_info **pointer) {
    int child_id, wstatus;
    struct exec_info* info = *pointer;
    int fd_input;
    int fd_output;
    
    int pid = fork();
	if (pid == 0) {
        
        if (info->input_file != NULL) {
            fd_input = open(info->input_file, O_RDONLY);
           
            if (fd_input == -1) {
                perror("open");
            }
            dup2(fd_input, STDIN_FILENO);
        }

        if (info->output_file != NULL) {
            // printf("output_file: %s", info->output_file);
            fd_output = open(info->output_file, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP);
           
            if (fd_output == -1) {
                perror("open");
            }
            dup2(fd_output, STDOUT_FILENO);
        }
        
        if (fd_input) close(fd_input);
        if (fd_output) close(fd_output);

        execvp(info->executable_path, info->arguments);

        
        // must use execv, because the number of arguments is dynamic
        // if (fd_output) close(fd_output);
        perror(info->executable_path);
        return EXIT_FAILURE;
    }

    child_id = wait(&wstatus);

    // if (fd_input) close(fd_input);
    // if (fd_output) close(fd_output);

    // child_id = wait(&wstatus);
    
    // if (WIFEXITED(wstatus)) {
    //     printf("%s exited with status %d\n", info->executable_path, WEXITSTATUS(wstatus));
    // } else {
    //     printf("%s exited abnormally\n", info->executable_path);
    // }

    return 1;
}

void interactiveMode() {
    printf("%s", "Welcome to my shell!\n");
    while (1) {
        fflush(stdout);
        printf("%s", PROMPT);
        fflush(stdout);
        struct exec_info* info = parseCommand();
        if (info == EXIT_SUCCESS) {
            break;
        }
        execute_command(&info);
        // printf("%s", PROMPT);
    }
}

void batchMode(char *fname) {
    int fd = open(fname, O_RDONLY);
    dup2(fd, STDIN_FILENO);
    
    while (1) {
        struct exec_info* info = parseCommand();
        if (info == EXIT_SUCCESS) {
            break;
        }
        execute_command(&info);
    }
    close(fd);
}
 
int main(int argc, char **argv)
{
    
    if (argc == 1) {
        // interactive mode
        interactiveMode();
        
    } else if (argc == 2) {
        // batch mode 
        batchMode(argv[1]);
    }

    // batch mode


}
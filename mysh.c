#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "words.h"
#include <unistd.h>
#include <sys/stat.h>

#define PROMPT "mysh> "
#define FAIL_PROMPT "!mysh> "
#define IS_EXCEPTIONS(x) (x == '|' | x == '<' | x == '>')
#define is_builtin_cmds(x) (x == "cd" | x == "pwd")

static int is_cmd_success = 1;
static int not_stopped_at_newline = 1;
static int reached_EOF = 0;
static char* search_paths[] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin", NULL};
static int status;
struct exec_info {
    char **arguments;
    char *executable_path;
    char *cmd;
    char *input_file;
    char *output_file;
    int args_len;
};

#define is_builtin_commands(x) (strcmp(x, "cd") == 0 | strcmp(x, "pwd") == 0)


char *get_executable_path(char *word) {
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

int mysh_cd(char *path) {
    if (path == NULL) {
        if (chdir(getenv("HOME")) == -1) {
            perror("cd");
            is_cmd_success = 0;
            return EXIT_FAILURE;
        } 
    } else {
        if (chdir(path) == -1) {
                perror("cd");
                is_cmd_success = 0;
                return EXIT_FAILURE;
            } 
    }

    is_cmd_success = 1;
    return EXIT_SUCCESS;
}

int mysh_pwd() {
    char *cwd;
    int size = pathconf(".", _PC_PATH_MAX); // get maximum path length

    if ((cwd = (char *)malloc(size)) != NULL) { // allocate memory for cwd
        if (getcwd(cwd, size) != NULL) { // get the current working directory
            printf("%s\n", cwd);
        } else {
            perror("pwd");
            is_cmd_success = 0;
            return EXIT_FAILURE;
        }
        free(cwd); // free memory allocated for cwd
    } else {
        perror("malloc");
        is_cmd_success = 0;
        return EXIT_FAILURE;
    }

    is_cmd_success = 1;
    return EXIT_SUCCESS;
}

struct exec_info* parseCommand() {
    char *word;
    char **arguments;
    char *executable_path;
    char *cmd;
    char *input_file = NULL;
    char *output_file = NULL;

    struct exec_info* info = malloc(sizeof(struct exec_info));

    // executable
    word = words_next();
    info->cmd = word;
    // printf("word: %s\n", word);

    if (word == NULL) {
        return NULL;
    }

    if (strcmp(word, "\n") == 0) {
        fflush(stdout);
        printf("%s", PROMPT);
        fflush(stdout);
        // return NULL;
        return NULL;
    }

    if (strcmp(word, "exit") == 0) {
            printf("mysh: exiting...\n");
            exit(1);
    }

    executable_path = get_executable_path(word);

    // command as first arg.
    arguments = malloc(sizeof(char*));
    arguments[0] = (char *) malloc(sizeof(char) * strlen(word) + 1);

    int args_pos = 1;

	while (1) {
        word = words_next();
        // fflush(stdout);
        // printf("token: %s\n", word);
        // fflush(stdout);
        // EOF in a batch mode
        if (word == NULL) {
            // EOF
            reached_EOF = 1;
            arguments[args_pos] = NULL;

            info->arguments = arguments;
            info->executable_path = executable_path;

            return info;
        }

        if (strcmp(word,"|") == 0) {
            break; 
        }
        
        if (strcmp(word, "\n") == 0) {
            not_stopped_at_newline = 0;
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
        } else if (strncmp(word, "~/", 2) == 0) {
            // home dir
            char *home_env = getenv("HOME");
            char* home_word = strcat(home_env, &word[1]);
            // printf("home_word: %s", home_word);
            // arguments
            arguments = realloc(arguments, args_pos + 1 * sizeof(char*));
            arguments[args_pos] = (char *) malloc(sizeof(char) * strlen(home_word) + 1);
        
            strcpy(arguments[args_pos], home_word);
            
            ++args_pos;

        } 
        else {
            // arguments
            arguments = realloc(arguments, args_pos + 1 * sizeof(char*));
            arguments[args_pos] = (char *) malloc(sizeof(char) * strlen(word) + 1);
        
            strcpy(arguments[args_pos], word);
            
            ++args_pos;
        }

        free(word);
	}

    
    arguments[args_pos] = NULL;

    info->arguments = arguments;
    info->executable_path = executable_path;
    info->input_file = input_file;
    info->output_file = output_file;
    info->args_len = args_pos - 1;

    return info;
}

struct exec_info** pareseLine() {
    struct exec_info** exec_infos;
    struct exec_info* info;
    
    int pos = 0;

    exec_infos = (struct exec_info**) malloc(sizeof(struct exec_info *));

    while (not_stopped_at_newline) {
        info = parseCommand();
        if (info != NULL) {
            exec_infos = realloc(exec_infos, pos + 1 * sizeof(struct exec_info *));
            exec_infos[pos] = info;
            ++pos;
        } else {
            break;
        }
    }

    not_stopped_at_newline = 1;
    exec_infos[pos] = NULL;

    return exec_infos;
    
}

int execute_command(struct exec_info **pointer) {
    int child_id, wstatus;
    struct exec_info* info = *pointer;
    int fd_input;
    int fd_output;

    if (strcmp(info->cmd, "cd") == 0) {
        printf("%d\n", info->args_len);
        if (info->args_len == 0) {
            return mysh_cd(NULL);
        } else {
            return mysh_cd(info->arguments[1]);
        }
    }

    if (strcmp(info->cmd, "pwd") == 0) {
        return mysh_pwd();
    }
    
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

    child_id = wait(&wstatus);
    
    if (WIFEXITED(wstatus)) {
        is_cmd_success = 1;
        // printf("%s exited with status %d\n", info->executable_path, WEXITSTATUS(wstatus));
    } else {
        is_cmd_success = 0;
        // printf("%s exited abnormally\n", info->executable_path);
    }

    return EXIT_SUCCESS;
}

int execute_pipe(struct exec_info ***pointer) {
    int child_id;
    struct exec_info** infos = *pointer;
    int fd_input;
    int fd_output;
	int i, pid1, pid2, clid, wstatus, p[2];
	char *clname;
    
    int pos = 0;
    while (infos[pos+1] != NULL) {
        if (pipe(p) == -1) {
            perror("pipe");
            return EXIT_FAILURE;
        }
        
        pid1 = fork();
        if (pid1 == 0) {
            
            dup2(p[1], STDOUT_FILENO);
        
            // close excess file descriptors
            close(p[0]);
            close(p[1]);
            
            execvp(infos[pos]->executable_path, infos[pos]->arguments);
                // must use execv, because the number of arguments is dynamic
            
            perror(infos[pos]->cmd);
            return EXIT_FAILURE;
        }
        
        pid2 = fork();
        if (pid2 == 0) {
        
            //child process reads from pipe
            dup2(p[0], STDIN_FILENO);

            // dup2(p[1], STDOUT_FILENO);
            
            // close excess file descriptors
            close(p[0]);
            close(p[1]);
            
            execvp(infos[pos+1]->executable_path, infos[pos+1]->arguments);
        
            perror(infos[pos+1]->cmd);
            return EXIT_FAILURE;
        }
        
        // close excess file descriptors
        close(p[0]);
        close(p[1]);  // what happens if we move this after the wait loop?
        
        // wait for child processes to end
        // (is the order always deterministic? why or why not?)
        for (i = 0; i < 2; i++) {
            clid = wait(&wstatus);
            clname = clid == pid1 ? infos[pos]->cmd : infos[pos+1]->cmd;
            if (WIFEXITED(wstatus)) {
                printf("%s exited with status %d\n", clname, WEXITSTATUS(wstatus));
            } else {
                printf("%s exited abnormally\n", clname);
            }
        }

        ++pos;
    }
	

	return EXIT_SUCCESS;

}

void interactiveMode() {
    printf("%s", "Welcome to my shell!\n");
    words_init(STDIN_FILENO);
    while (1) {
        if (is_cmd_success == 1) {
            fflush(stdout);
            printf("%s", PROMPT);
            fflush(stdout);
        } else {
            fflush(stdout);
            printf("%s", FAIL_PROMPT);
            fflush(stdout);
        }

        struct exec_info** infos = pareseLine();

        // getting command count
        int count = 0;
        while (infos[count] != NULL) {
            count++;
        }

        if (count == 1) {
            execute_command(&infos[0]);
        } else {
            execute_pipe(&infos);
        }
    }
}

void batchMode(char *fname) {
    int fd = open(fname, O_RDONLY);
    dup2(fd, STDIN_FILENO);
    words_init(STDIN_FILENO);
    
    while (1) {
        if (reached_EOF) {
            exit(1);
        }
        struct exec_info** infos = pareseLine();

        // getting command count
        int count = 0;
        while (infos[count] != NULL) {
            count++;
        }

        if (count == 1) {
            execute_command(&infos[0]);
        } else {
            execute_pipe(&infos);
        }
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
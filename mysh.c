#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "words.h"
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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

char** traverse(char *dname, char** sub_files, size_t* size) {
    struct dirent *de;
    long offset;
    int flen;
    int dlen = strlen(dname);
    char *pname;

    DIR *dp = opendir(dname);

    // char *sub_files = malloc(sizeof(char) * (size + 1));
    // char **sub_files = malloc(sizeof(char *));
    
    if (!dp) {
        perror(dname);
        return NULL;
        // return;
    }
    // printf("Traversing %s\n", dname);

    int i = 0;
    
    while ((de = readdir(dp))) {
        // printing out all the files/subdirectoreis inside the directory
        // printf("%s/%s\n", dname, de->d_name);
        if (de->d_type == DT_DIR && de->d_name[0] != '.') {
            // construct new path
            flen = strlen(de->d_name);
            pname = malloc(dlen + flen + 2);
            memcpy(pname, dname, dlen);
            pname[dlen] = '/';
            memcpy(pname + dlen + 1, de->d_name, flen);
            pname[dlen + 1 + flen] = '\0';
            // save location in directory
            offset = telldir(dp);
            closedir(dp);
            // recursively traverse subdirectory
            sub_files = traverse(pname, sub_files, size);
            free(pname);
            // restore position in directory
            dp = opendir(dname); // FIXME: check for success
            seekdir(dp, offset);
        }
        else/* if(de->d_type == DT_REG) */{
            char *file_name = malloc(sizeof(de->d_name)+sizeof("./"));
            strcpy(file_name, "./"); // all file starts with "./""
            strcat(file_name, de->d_name);
            // printf("%s", slash);
            int len = strlen(file_name);
            // printf("%s and %d, i: %d\n", file_name, len, i);

            // printf("%ld\n", *size);
            sub_files[*size] = file_name;
            *size = *size + 1;
            sub_files = (char**) realloc(sub_files, sizeof(char *)*(*size + 1)); // resize sub_files array to hold i + 1 pointers
            
            i++;
        }
    }
    // printf("%p %p\n", readdir(dp), dp);
    // printf("%ld\n", *size);
    closedir(dp);
    return sub_files;
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

        } else if (strchr(word, '*') != NULL) { // check if the token contains "*"
            char delim[] = "*"; // The character on the basis of which the split will be done
            char *token = strtok(word, delim);

            // printf("%s\n", token);

            int i = 0;

            char *pattern[2];

            // printf("%d\n", word[0]);

            // splitting the token and save it
            // do I need this if/else if statement?
            if(word[0] == '*') {
                pattern[0] = NULL;
                pattern[1] = token;
            }
            else {
                while(token != NULL) {
                    pattern[i] = token;
                    // printf("%s \n", pattern[i]);
                    token = strtok(NULL, delim); // this gives the next remaining string
                    i++;
                }
            }

            // printf("pattern[0] = %s, pattern[1] = %s \n", pattern[0], pattern[1]);

            // traversing and storing all the file name
            char **files = malloc(sizeof(char *));
            size_t num = 0;
            size_t *size = &num;

            files = traverse(".", files, size); // stores all the subdirectory/file name
            
            // divide the cases of the pattern
            
            // check all file name that has certain pattern required
            // use strstr() to compare
            char *first_pattern;
            char *second_pattern;

            char **replace_files = malloc(sizeof(char *));
            

            // checks the first pattern is in the file name (when it only has front)
            if(pattern[0] != NULL) {
                for(int i = 0; i < *size; i++) {
                    first_pattern = strstr(files[i], pattern[0]);
                    if(first_pattern != NULL) {
                        printf("%s \n", files[i]);
                        second_pattern = strstr(first_pattern, pattern[1]);
                        // checking the pattern when it has string both front and back
                        if(second_pattern != NULL) {
                            printf("%s \n", files[i]);
                            // replace_files = realloc(replace_files, (i+1));
                            // return replace_files;
                        }
                    }

                    // else if(first_pattern == NULL) {
                    //     // printf("No match \n");
                    // }
                }
            }
            // when it has only back
            else {
                for(int i = 0; i < *size; i++) {
                    second_pattern = strstr(files[i], pattern[1]);
                    if(second_pattern != NULL) {
                        printf("%s \n", files[i]);
                    }
                }
            }

            // for(int i = 0; i < *size; i++) {
            //     first_pattern = strstr(files[i], pattern[0]);
            //     second_pattern = strstr(files[i], pattern[1]);

            //     // checks if it contains both pattern in order (first_pattern comes first)
            //     if(first_pattern != NULL && second_pattern != NULL && first_pattern < second_pattern) {
            //         // contains string in the front and back of "*"
            //         printf("%s\n", files[i]);
            //     }
            //     else if(first_pattern == NULL && second_pattern != NULL) {
                    
            //         printf("%s\n", files[i]);
            //     }
            // }

            // for(int i = 0; i < *size; i++){
            //     // printf("%s\n", files[i]);
            //     if(files[i])
            // }

            

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

    // char **sub_files = malloc(sizeof(char *));
    // size_t num = 0;
    // size_t *size = &num;

    // printf("%ld\n", *size);

    // sub_files = traverse(".", sub_files, size);

    // printf("%ld\n", *size);

    // for(int i = 0; i < *size; i++){
    //     printf("%s\n", sub_files[i]);
    // }

}
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

int main(int argc, char **argv)
{
    
    // if (argc == 1) {
    //     // interactive mode
    //     interactiveMode();
        
    // } else if (argc == 2) {
    //     // batch mode 
    //     batchMode(argv[1]);
    // }

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

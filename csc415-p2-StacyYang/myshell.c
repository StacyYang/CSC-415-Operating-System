/****************************************************************           bulletin:
 * Name        :  Xiaoqian Yang                                 *               fgets
 * Class       :  CSC 415                                       *               strtok  
 * Date        :  06/16/2020                                    *               strcmp
 * Description :  Writting a simple shell program               *               strncpy 
 *                that will execute simple commands. The main   *               strstr
 *                goal of the assignment is working with        *               chidr
 *                fork, pipes and exec system calls.            *               execvp
 *                                                              *               dup2()
 *                                                              *               wait vs waitpid   https://webdocs.cs.ualberta.ca/~tony/C379/C379Labs/Lab3/wait.html
 ****************************************************************/             

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

/* CANNOT BE CHANGED */
#define BUFFERSIZE 256
/* --------------------*/
#define PROMPT "* myShell "
#define PROMPTSIZE sizeof(PROMPT)
#define DELIM " \t\n"
#define ARGVMAX 100


void my_prompt();
void changeDir(int, char* );
int length(char** ); // aka myargc
void execute(char**, int, int, int, int);
void execute_pipe(char**, int);

int main(int argc, char** argv){
    char input[BUFFERSIZE]; 
    char* temp; //temperay input token storage
    char* myargv[ARGVMAX];//store myarguments
    int myargc;

    int background; //set to 1/true when & present
    int has_pipe;//set to 1/true when | present
    int redirect_stdout, redirect_stdout_append, redirect_stdin; // set to 1(true) if condition meet
    
    
    while(1){
        myargc = 0; //set myargc back to 0 for every outer while loop.
        temp = NULL;

        background = 0;
        has_pipe = 0;
        redirect_stdout = 0; redirect_stdout_append = 0; redirect_stdin = 0;

        my_prompt();
        if( fgets(&input[0], BUFFERSIZE, stdin) == NULL){
            perror("Error: Failed to get user input...\n");
            exit(-1);
        }

        temp = strtok(input, DELIM);
        if(temp == NULL){
            continue;
        }
        while(temp != NULL){
            myargv[myargc] = temp;
            temp = strtok(NULL, DELIM);
            myargc++;
        }
        myargv[myargc] = NULL; //set the last element (aka the enter) of myargv to NULL

        if(myargc != 0){
            //check for "exit" or "quit"
            if(strcmp(myargv[0], "exit") == 0 || strcmp(myargv[0], "quit") == 0){
                exit(0);
            }
            
            if(strcmp(myargv[0], "cd") == 0) {
                char *directory = myargv[1];
                changeDir(myargc, directory); 
                continue; //need continue here because otherwise the code continues to parse the command as an external executable, which fails.
            }
            
            if(strcmp(myargv[0], "pwd") == 0) {
                char pwd[BUFFERSIZE];
                if (getcwd(pwd, BUFFERSIZE) == NULL) {
                    perror("Error: getting pwd failed...\n");
                }
            }
            
            if(strcmp(myargv[myargc - 1], "&") == 0){
                background = 1;
                myargv[myargc -1] = NULL;  //make & Null
                printf("Execute background process, do not wait.\n");
            }
            
            if(myargc >= 3){//NOTE: I/O redirection doesn't work with pipe/&
                            //Because of the filename and ">" are location based (the last two arguments).  
                if(strcmp(myargv[myargc - 2], ">") == 0){ 
                    redirect_stdout = 1;
                }else if(strcmp(myargv[myargc - 2], ">>") == 0 ){
                    redirect_stdout_append= 1;
                }else if(strcmp(myargv[myargc - 2], "<") == 0){
                    redirect_stdin = 1;
                }else {
                    for(int i = 0; i < myargc; i++){
                        if(strcmp(myargv[i], "|") == 0){
                            has_pipe = 1;
                            printf("detect pipe\n");
                        }
                    }
                }
            }//end of if(myargc > 3)
          
            if(has_pipe == 0){
                execute(myargv, background, redirect_stdout, redirect_stdout_append, redirect_stdin);
            }else{
                execute_pipe(myargv, background);
            }

        }//end of if(myargc != 0)
    }//end of while(1)
    
    return 0;
}



void my_prompt(){ 
    char pwd[BUFFERSIZE];
    if (getcwd(pwd, BUFFERSIZE) == NULL) {
        perror("Error: getting pwd failed...\n");
    }
    char* temp = pwd;

    char simpleDir[BUFFERSIZE];
   
    if(strcmp(temp,"/home") == 0){
        strcpy(simpleDir,"~/");
    }else if(temp[0] == '/' && temp[1] == 'h' && temp[2] == 'o' && temp[3] == 'm' && temp[4] == 'e' ){
        strcpy(simpleDir,"~");
        memcpy(simpleDir+1, pwd+5, strlen(pwd)-4);
    }else{
        strcpy(simpleDir,pwd);
    }
    printf("\n%s%s >> ", PROMPT, simpleDir);
}

void changeDir(int myargc, char* directory) {
    if(myargc == 1){
        char *home = getenv("HOME");
        if(chdir(home) != 0){
            perror("Error: chdir going home directory failed...\n");
            exit(-1);
        }
    }else{
        //char *directory = myargv[1];
        int destination = chdir(directory);
        if (destination == -1) {
            perror("Error: executing \"cd\" command failed...\n");
            exit(-1);
        }
    }
}

int length(char** myargv) {
	int count = 0;
	while (myargv[count] != NULL)
		count++;
	return count;
}

void execute(char** myargv, int background, int redirect_stdout, int redirect_stdout_append, int redirect_stdin){
    pid_t child_pid;

    int myargc = length(myargv);
    int bg = background;

    child_pid = fork();
    if(child_pid < 0){
        perror("Error: fork failed...\n");
        exit(-1);
    }else if(child_pid == 0){//child process
        if(redirect_stdout){// ">"
            int output = open(myargv[myargc - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            if(output < 0){
                perror("Error: > failed...\n");
                exit(-1);
            }
            int ret_stdout = dup2(output, 1); // here the newfd is the file descriptor of stdout (i.e. 1), can also use STDOUT_FILENO
            if(ret_stdout < 0){
                perror("Error: dup2 stdout failed...\n");
                exit(-1);
            }
            myargv[myargc - 2] = NULL;     
            myargv[myargc - 1] = NULL;
            close(output);
        }

        if(redirect_stdout_append){//">>"
            int output = open(myargv[myargc - 1], O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR );
            if(output < 0){
                perror("Error: >> failed...\n");
                exit(-1);
            }
            int ret_stdout_append = dup2(output, 1); 
            if(ret_stdout_append < 0){
                perror("Error: dup2 stdout failed...\n");
                exit(-1);
            }
            myargv[myargc - 2] = NULL;     
            myargv[myargc - 1] = NULL;
            close(output);
        }

        if(redirect_stdin){// "<"
            int output = open(myargv[myargc - 1], O_RDONLY);
            if(output < 0){
                perror("Error: < failed...\n");
                exit(-1);
            }
            int ret_stdin = dup2(output, 0); 
            if(ret_stdin < 0){
                perror("Error: dup2 stdin failed...\n");
                exit(-1);
            }
            myargv[myargc - 2] = NULL;     
            myargv[myargc - 1] = NULL;
            close(output);
        }

        if(execvp(myargv[0], &myargv[0]) < 0){
            if(strcmp(myargv[0], "exit") != 0 || strcmp(myargv[0], "quit") != 0){ 
                printf("Unknown command.\n");
            }
            exit(0);
        }

    }else{//parent process
        if(!bg){
            int status = 0;
            wait(&status);
            sleep(1); //to give more time for the child. so the prompt and the output will be aligned from next command instead of messy all the time.
        }
    }
}

void execute_pipe(char** myargv, int background){  // multiple pipes idea references from  https://github.com/conv1d/myshell/blob/master/myshell.c
    int bg = background;
    char **argv = myargv;
    int argc = length(myargv);

    int i = 0, j = 1, status;

    int pipe_num = 0;
    for(i = 0; i < argc; i++){ //count pipe numbers
        if(strcmp(argv[i], "|") == 0){
            pipe_num++;
        }
    }

    pid_t pid[20];
    int fd[20][2];

    int index[20];
    for(i = 0; i < 20; i++){
        index[i] = 0;
    }                                                            //    i=0                        i=1                       i=2
    for(i = 0; i < argc; i++){ // separate commands              //    ls    -la    |           wc    -l         |           wc     -c
        if(strcmp(argv[i], "|") == 0){                           //argv[0]   [1]   [2]         [3]    [4]       [5]          [6]    [7]
            index[j] = i + 1;                                    //               index[j]                    index[j]
            argv[i] = NULL;                                      //               index[1]=2+1                index[2]=5+1
            j++;                                                 //                       =3                          =6
        }                                                        //                 j++                             j++
    }

    for(i = 0; i < pipe_num; i++){  //make pipes
        if(pipe(fd[i]) < 0){
            perror("Error: pipe failed...\n");
            exit(-1);
        }
    }
    
    i = 0;
    pid[0] = fork();
    if(pid[0] < 0){
        perror("Error: fork failed...\n");
        exit(-1);
    }else if(pid[0] == 0){ //child 1
        close(1);
        dup(fd[0][1]);
        close(fd[0][1]);
        close(fd[0][0]);
        if(execvp(argv[0], &argv[0]) < 0){
            perror("Error: execute pipe command failed...\n");
            exit(-1);
        }
    }else{
        close(fd[0][1]);
    }
    // }else{                          *****************************************************
    //     if(!bg){                     parent process don't wait other than the last child
    //         int status = 0;         *****************************************************
    //         wait(&status);
    //         sleep(1); 
    //     }
    // }

    i++;
    while(i < pipe_num){
        pid[i] = fork();
        if(pid[i] < 0){
            perror("Error: fork failed...\n");
            exit(-1);
        }else if(pid[i] == 0){//child i
            close(0);
            dup(fd[i-1][0]);
            close(1);
            dup(fd[i][1]);
            close(fd[i][1]);
            close(fd[i-1][0]);
            if(execvp(argv[index[i]], &argv[index[i]]) < 0){
                perror("Error: execute pipe command failed...\n");
                exit(-1);
            }
        }else{
            close(fd[i][1]);
        }
        i++;
    }

    pid[i] = fork();
    if(pid[i] < 0){
        perror("Error: fork failed...\n");
        exit(-1);
    }else if(pid[i] == 0){ //last child
        close(0);
        dup(fd[i-1][0]);
        close(fd[i-1][0]);
        close(fd[i-1][1]);
        if(execvp(argv[index[i]], &argv[index[i]]) < 0){
            perror("Error: execute pipe command failed...\n");
            exit(-1);
        }
    }else{
        if(!bg){
            close(fd[i-1][1]);
            sleep(1);
            wait(&status);
            sleep(1); 
        }
    }
}




/*   <stdio.h>
fgets: reads a line from the specified stream and stores it into the string pointed to by str. 
       It stops when either (n-1) characters are read, the newline character is read, or the end-of-file is reached, 
       whichever comes first.
Declaration: char *fgets(char *str, int n, FILE *stream)
Parameters: str − This is the pointer to an array of chars where the string read is stored.
             n  − This is the maximum number of characters to be read (including the final null-character).
                 Usually, the length of the array passed as str is used.
            stream − This is the pointer to a FILE object that identifies the stream where characters are read from.
Return value: On success, the function returns the same str parameter. If the End-of-File is encountered and no characters have been read, 
              the contents of str remain unchanged and a null pointer is returned.
              If an error occurs, a null pointer is returned.
*/

/*   <string.h>
strtok: breaks string str into a series of tokens using the delimiter delim.
Declaration: char *strtok(char *str, const char *delim)
Parameters: str − The contents of this string are modified and broken into smaller strings (tokens).
            delim − This is the C string containing the delimiters. These may vary from one call to another.
Return value: This function returns a pointer to the first token found in the string. 
              A null pointer is returned if there are no tokens left to retrieve.
*/

/*  <string.h>
strcmp:compares two strings and returns 0 if both strings are identical.
Declaration: int strcmp (const char* str1, const char* str2);
Return: 0 -- if both strings are identical (equal)
        negative -- if the ASCII value of the first unmatched character is less than second.
        positive intege -- if the ASCII value of the first unmatched character is greater than second.
*/

/*
strncpy: copies up to n characters from the string pointed to, by src to dest. 
         In a case where the length of src is less than that of n, the remainder of dest will be padded with null bytes.
Declaration: char *strncpy(char *dest, const char *src, size_t n)
Parameters: dest − This is the pointer to the destination array where the content is to be copied.
            src − This is the string to be copied.
            n − The number of characters to be copied from source.
Return value:  returns the final copy of the copied string.
*/

/*
strstr: finds the first occurrence of the substring needle in the string haystack. The terminating '\0' characters are not compared.
Declaration: char *strstr(const char *haystack, const char *needle)
Parameters: haystack − This is the main C string to be scanned.
            needle − This is the small string to be searched with-in haystack string.
Return value: This function returns a pointer to the first occurrence in haystack of any of the entire sequence of characters specified 
              in needle, or a null pointer if the sequence is not present in haystack.
*/

/*  <unistd.h>
chdir: command is a system function (system call) which is used to change the current working directory. 
Declaration: int chdir(const char *path);
Parameter: the path is the Directory path which the user want to make the current working directory.
Return value:  zero (0) on success. 
               -1 is returned on an error and errno is set appropriately.
*/


/*
The dup() system call creates a copy of a file descriptor.
Syntax: int dup2(int oldfd, int newfd);
        oldfd: old file descriptor
        newfd new file descriptor which is used by dup2() to create a copy.
*/

/*   <unistd.h>
The exec family of functions replaces the current running process with a new process. 
It can be used to run a C program by using another C program. 

execvp: the created child process does not have to run the same program as the parent process does. 
Declaration: int execvp (const char *file, char *const argv[]);
Parameters: file - points to the file name associated with the file being executed.
            argv -  is a null terminated array of character pointers.
*/




/**
 * Author: Ethan Schrack
 * Date: 12/4/2022
 * Program that imitates the basic function of the bash shell
 * including input and output redirection, a prompt that displays
 * the present working directory, and a functional CD command.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

// 1024 * 1024
#define BUFFSIZE 1048576

int main() {

    char cmd[BUFFSIZE]; // whatever is gathered from stdin
    char* cmds[10000]; // the arguments that are put into a string array
    char* cmdarg[10000]; // the final string array (not including >, <, etc.
    char* first; // the command that is put in the prompt
    char* homedir; // the home directory when the user first launches the program
    char cwd[4096]; // the cwd string
    char finalcwd[4096]; // the cwd string that includes a tilde if applicable
    pid_t pid;
    int count; // counts the number of arguments

    setbuf(stdout, NULL); //unbuffer output
    homedir = getenv("HOME"); // get home dir

    // main loop of the program
    while(1) {

        // reset things
        first = NULL;
        count = 0;

        // get cwd
        getcwd(cwd, sizeof(cwd));

        // reset what is input into chdir
        for (int i = 0; i < 4096; i++) {
            finalcwd[i] = '\0';
        } // for

        // if the user is in the home directory
        if (strcmp(cwd, homedir) == 0) {
            finalcwd[0] = '~';
            finalcwd[1] = '\0';
        } else {
            // string that is the same size as home directory
            char tempName[strlen(homedir)];
            // copy the whatever is in cwd to tempName for the length of the homedir string
            for (int i = 0; i < strlen(homedir); i++) {
                tempName[i] = cwd[i];
            } // for

            // if the first part of cwd is equal to homedir
            if (strcmp(tempName, homedir) == 0) {
                // add tilde
                finalcwd[0] = '~';
                // the length of what is left to write
                int difference = strlen(cwd) - strlen(homedir);
                // the point after the homedir in the cwd
                int start = strlen(homedir);
                // start at 1 and go through the cwd and add to the finalcwd
                for (int i = 1; i <= difference; i++) {
                    finalcwd[i] = cwd[start];
                    start++;
                } // for
            } else { // the user is not in the home directory at all
                for (int i = 0; i < strlen(cwd); i++) {
                    finalcwd[i] = cwd[i];
                } // for
            } // if
        } // if

        printf("1730sh:%s$ ", finalcwd); // prompt

        // read from stdin
        int bits;
        if ((bits = read(STDIN_FILENO, cmd, BUFFSIZE) == -1)) perror ("read");

        cmd[strcspn(cmd, "\n")] = 0; // change newline character at end

        first = strtok(cmd, " "); // use strtok on cmd

        // loop through cmd and assign to string array while counting number of strings
        while (first != NULL) {
            cmds[count] = first;
            first = strtok(NULL, " ");
            count++;
        } // while

        // null terminate array
        cmds[count + 1] = NULL;

        if (strcmp(cmds[0], "exit") == 0) { // exit condition
            exit(0);
        } else if (strcmp(cmds[0], "cd") == 0) { // cd condition
            if (count == 1) { // if only arg is cd, then go home
                if (chdir(homedir) == -1) perror("chdir");
            } else if (strcmp(cmds[1], "~") == 0) { // if next arg is tilde, go home
                if (chdir(homedir) == -1) perror("chdir");
            } else if (strcmp(cmds[1], "..") == 0) {
                // if next arg is .., go to parent dir
                // I didn't know the dirname function existed until after I hard coded it :/
                int slashcount = 0; // the number of slashes
                int slashcounter = 0; // counts the number of slashes
                // loops through cwd and finds number of slashes
                for (int i = 0; i < strlen(cwd); i++) {
                    if (cwd[i] == '/') slashcount++;
                } // for
                // once slash counter finds the last slash, cut off the rest of the string
                for (int i = 0; i < strlen(cwd); i++) {
                    if (cwd[i] == '/') slashcounter++;
                    if (slashcount == slashcounter) {
                        cwd[i] = '\0';
                    } // if
                } // for
                if (chdir(cwd) == -1) perror("chdir");
            } else { // if a file path is specified
                char* newpath = cmds[1]; // the path argument
                int cwdlength = strlen(cwd); // length of cwd
                cwd[cwdlength] = '/'; // add new slash to end of cwd
                cwdlength++; // increment so it doesn't replace the slash
                // add the newpath to cwd
                for (int i = 0; i <= strlen(newpath); i++) {
                    cwd[cwdlength + i] = newpath[i];
                }
                if (chdir(cwd) == -1) perror("chdir");
            } // if
        } else { // if no exit or cd, fork
            if ((pid = fork()) < 0) perror("fork");
            else if (pid == 0) { // child process
                for (int i = 0; i < count; i++) { // loop through command args
                    if (strcmp(cmds[i], "<") == 0) { // redirect input
                        int fdi = open(cmds[i + 1], O_RDONLY); // open next arg
                        if (fdi == -1) perror("open");
                        dup2(fdi, STDIN_FILENO);
                        i++; // move index to after < then the loop will increment it again
                    } else if (strcmp(cmds[i], ">") == 0) { // redirect output
                        int fdo;
                        fdo = open(cmds[i + 1], O_RDWR | O_TRUNC | O_CREAT, 0644);
                        if (fdo == -1) perror("open");
                        dup2(fdo, STDOUT_FILENO);
                        i++;
                    } else if (strcmp(cmds[i], ">>") == 0) { // redirect output append
                        int fdoo;
                        fdoo = open(cmds[i + 1], O_RDWR | O_APPEND | O_CREAT, 0644);
                        if (fdoo == -1) perror("open");
                        dup2(fdoo, STDOUT_FILENO);
                        i++;
                    } else {
                        // no redirection
                        cmdarg[i] = cmds[i];
                    } // else
                } // for

                //exec cmdarg
                if (execvp(cmdarg[0], cmdarg) == -1) {
                    perror("execvp");
                    return EXIT_FAILURE;
                } // if

            } else { // parent process
                // wait on child
                int status;
                wait(&status);
            }
        } // if
    } // while
} // main

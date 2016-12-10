#ifndef FRANK_SHELL_C
#define FRANK_SHELL_C

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* 
   Frank Dattalo
   dattalo.2@osu.edu
   compile instructions: gcc frank_shell.c -O3 -o Frank-Shell
   to exit the shell type either exit or quit
*/

#define MAX_LINE 80
#define MAX_LINE_PLUS_ONE 81
#define HIST_SIZE 10
#define COMMAND_DELIMITER "@"
#define COMMAND_DELIMITER_AS_CHAR '@'
#define OUTPUT_FILE_NAME ".frank_shell_history"
#define BACKGROUND_SIGNIFIER "&"
#define EMPTY_STRING ""

typedef struct history {
    int command_number;
    char input_buffer[MAX_LINE];
} history;

void read_line(char inputBuffer[]) {	
	bzero(inputBuffer, MAX_LINE);
    
    inputBuffer[0] = '\0';

    char c;
    int i = 0;

    c = getc(stdin);

    while(c != '\n' && c != EOF) {
        inputBuffer[i] = c;
        c = getc(stdin);
        i++;
    }

    inputBuffer[i] = '\0';
}

void parse_line(char inputBuffer[MAX_LINE], char args[MAX_LINE_PLUS_ONE][MAX_LINE], int *background) {
    int ib;
    *background = 0;

    int i;
    for(i = 0; i < MAX_LINE_PLUS_ONE; i++) {
        args[i][0] = '\0';
    }

    i = 0;
    int j = 0;

    for(ib = 0; ib < MAX_LINE && i < MAX_LINE_PLUS_ONE; ib++) {

        if(inputBuffer[ib] == ' ') {
            args[i][j] = '\0';
            
            j = 0;
            i++;

        } else {
            args[i][j++] = inputBuffer[ib];
        }
    }

    args[i][j] = '\0';

    *background = !strncmp(args[i], BACKGROUND_SIGNIFIER, MAX_LINE);
}

void push_hist(history hist[HIST_SIZE], int command_number, char input_buffer[MAX_LINE]) {
    int i;

    for(i = 0; i < HIST_SIZE; i++) {
        if(hist[i].command_number == 0) {
            hist[i].command_number = command_number;
            strncpy(hist[i].input_buffer, input_buffer, MAX_LINE);

            break;
        }
    }
}

void init_hist(history hist[HIST_SIZE]) {
    int i;

    for(i = 0; i < HIST_SIZE; i++) {
        hist[i].command_number = 0;
        
        strncpy(hist[i].input_buffer, EMPTY_STRING, MAX_LINE);
    }

    FILE* file = fopen(OUTPUT_FILE_NAME, "r");

    /* no history file exists */
    if(!file) return;

    int command_number = 0;
    int past_delimiter = 0;
    char c = fgetc(file);

    char line[MAX_LINE];
    strncpy(line, EMPTY_STRING, MAX_LINE);

    int lineIndex = 0;

    while(c != EOF) {
        
        /* if at the end of line of file */
        if(c == '\n') {
            /* if the parsed line is a valid line */
            if(command_number > 0 && strnlen(line, MAX_LINE) > 0) {
                push_hist(hist, command_number, line);

            } else { /* else the history file is corrupted */
                printf("History file corrupted.\n");
                break;
            }

            /* reset control variables */
            strncpy(line, EMPTY_STRING, MAX_LINE);
            lineIndex = 0;
            past_delimiter = 0;
            command_number = 0;
            
        } else { /* continue parsing line */

            /* if c is delimiter */
            if(c == COMMAND_DELIMITER_AS_CHAR) {
                past_delimiter = 1;

            } else { /* else parse one of two states */

                if(past_delimiter) {
                    line[lineIndex++] = c;

                } else {
                    command_number = 10 * command_number + c - '0';
                }
            }
        }

        c = fgetc(file);
    }


    fclose(file);
}

void write_hist(history hist[HIST_SIZE]) {
    FILE* file = fopen(OUTPUT_FILE_NAME, "w");

    /* if file could be open then inform user */
    if(!file) {
        printf("Could not save history.\n");
        return;
    }

    int i;
    for(i = 0; i < HIST_SIZE && hist[i].command_number != 0; i++) {
        fprintf(file, "%d%s%s\n", hist[i].command_number, COMMAND_DELIMITER, hist[i].input_buffer);
    }

    fclose(file);
}

void add_hist(history hist[HIST_SIZE], char inputBuffer[MAX_LINE]) {
    int i;
    int largerst_command = 0;
    int foundSpot = 0;

    for(i = 0; i < HIST_SIZE; i++) {

        /* find largest command number */
        if(hist[i].command_number > largerst_command) {
            largerst_command = hist[i].command_number;
        }

        if(hist[i].command_number == 0) {
            /* insert in open spot */
            hist[i].command_number = largerst_command + 1;
            strncpy(hist[i].input_buffer, inputBuffer, MAX_LINE);
            foundSpot = 1;

            break;
        }
    }

    if(!foundSpot) {
        /* shift history down */
        for(i = 0; i < HIST_SIZE - 1; i++) {
            hist[i].command_number = hist[i + 1].command_number;
            strncpy(hist[i].input_buffer, hist[i + 1].input_buffer, MAX_LINE);
        }

        hist[HIST_SIZE - 1].command_number = largerst_command + 1;
        strncpy(hist[HIST_SIZE - 1].input_buffer, inputBuffer, MAX_LINE);
    }

    write_hist(hist);
}

void run_recent(history hist[HIST_SIZE], char inputBuffer[MAX_LINE]) {
    int i;

    inputBuffer[0] = '\0';

    for(i = HIST_SIZE - 1; i >= 0; i--) {
        if(hist[i].command_number != 0 && strnlen(hist[i].input_buffer, MAX_LINE) > 0) {
            strncpy(inputBuffer, hist[i].input_buffer, MAX_LINE);
            break;
        }
    }
}

void run_number(history hist[HIST_SIZE], char input_buffer[MAX_LINE], int number) {
    int i;
    input_buffer[0] = '\0';

    for(i = 0; i < HIST_SIZE; i++) {
        if(number == hist[i].command_number) {
            strncpy(input_buffer, hist[i].input_buffer, MAX_LINE);
            break;
        }
    }
}

void print_hist(history hist[HIST_SIZE]) {
    int i;
    for(i = 0; i < HIST_SIZE; i++) {
        if(hist[i].command_number == 0) break;

        printf("[%10d] %s\n", hist[i].command_number, hist[i].input_buffer);
    }
}

void input_loop() {

    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background; /* equals 1 if a command is followed by '&' */
	
    char args[MAX_LINE_PLUS_ONE][MAX_LINE]; /* command line arguments */
    
    history hist[HIST_SIZE];
    pid_t pid;

    init_hist(hist);

    while (1) {
	    background = 0;

        printf("Frank-Shell> ");
        read_line(inputBuffer); /* get next command */

        /* split line into args */
	    parse_line(inputBuffer, args, &background);

        /* if no command was entered go to setup */
        if(inputBuffer[0] == 0) continue;

        /* if user enters exit or quit then exit the program */
        if(!strncmp(args[0], "exit", MAX_LINE) ||
            !strncmp(args[0], "quit", MAX_LINE)) {

            break;
        }

        /* if user enters history or h or hist */
        if(!strncmp(args[0], "history", MAX_LINE) || 
            !strncmp(args[0], "h", MAX_LINE) || 
            !strncmp(args[0], "hist", MAX_LINE)) {

            print_hist(hist);
            continue;
        }

        /* if user entered rr */
        if(!strncmp(args[0], "rr", MAX_LINE)) {
            run_recent(hist, inputBuffer);

             /* check to see if we actually found a command to run */
            if(strnlen(inputBuffer, MAX_LINE) == 0) {
                printf("No commands found.\n");
                continue;
            }

            /* split line into args */
            parse_line(inputBuffer, args, &background);
        }

        /* if user entered r as first command, look for next number */
        if(!strncmp(args[0], "r", MAX_LINE)) {
            int num = atoi(args[1]);

            /* invalid number formatting check */
            if(num == 0) {
                printf("Invalid number formatting.\n");
                continue;
            }

            run_number(hist, inputBuffer, num);

            /* check to see if we actually found a command to run */
            if(strnlen(inputBuffer, MAX_LINE) == 0) {
                printf("Invalid command number.\n");
                continue;
            }

            /* split line into args */
            parse_line(inputBuffer, args, &background);
        }

        /* spawn process */
        pid = fork();

        if(pid < 0) { /* check against failed fork */
        
            printf("Fork did not work.\n");
        
        } else if(pid == 0) { /* if child */
        
            /* parse args into pointers for execvp */
            char* arg_ptr[MAX_LINE_PLUS_ONE];

            int arg_index;
            for(arg_index = 0; arg_index < MAX_LINE_PLUS_ONE; arg_index++) {

                /* if the arg is empty or is an ampersand break out of loop and execute */
                if(args[arg_index][0] == '\0' || !strncmp(args[arg_index], BACKGROUND_SIGNIFIER, MAX_LINE)) { 
                    break; 
                }

                arg_ptr[arg_index] = args[arg_index];
            }

            arg_ptr[arg_index] = NULL;

            /* check against failed exec */
            if(-1 == execvp(arg_ptr[0], arg_ptr)) {
                printf("Command not recognized.\n");
                break;
            }

        } else { /* if parent */

            int status;
            pid_t endedPID = 0;

            if(!background) {
             	
                while(endedPID != pid) {
                    endedPID = wait(&status);
                }
            }
        }

        /* add command to history */
        add_hist(hist, inputBuffer);
    }
}

void signal_interceptor(int dummy) {
    /* noop to prevent CNTRL-C or CNTRL-D from killing program */
}

int main(void) {

    /* block cntrol-d or cntrol-c from killing process */
    signal(SIGINT, signal_interceptor);
    signal(30, signal_interceptor);

	input_loop();

    return 0;
}

#endif /*FRANK_SHELL_C */
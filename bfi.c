/* 
 * Kiernan Roche
 * 11/13/2019
 */

#include <stdio.h>
#include <stdlib.h>

#define NUMCELLS 1024   // number of cells per tape section. must be an even number
#define NUMSECTS 1      // number of sections on program start
#define STACKSIZE 1024  // loop depth, maximum size of stack

typedef struct sect {        // single section of tape, NUMCELLS cells wide
    char cell[NUMCELLS]; // array of characters, or tape cells
    struct sect *previous;  // pointer to previous tape section
    struct sect *next;      // pointer to next tape section
} section;

// Creates a new tape section given a pointer to the existing end of the section
// next determines whether the new section is appended before or after the section
// pointed to by end_section
// WARNING: if only one section exists and both previous and next are NULL, this function
// will create a single section after the current section. this is a problem if we create
// a new section and then try to decrement the tape pointer. the solution here is to
// create at least two sections on program start - then no section with two null pointers
// will exist, and this bug doesn't affect anything.
section * create_section(section *end_section) {
    section *new_section = (section *)calloc(1, sizeof(section)); // allocate memory for the new section and initialize its memory to 0
    if (new_section == NULL) {
        printf("Couldn't allocate tape section memory!\n");
        exit(10);
    }

    if (end_section == NULL) {
        return new_section;
    }

    if (end_section->next == NULL) { // the new section is added after the end section
        end_section->next = new_section;
        new_section->previous = end_section;
    } else if (end_section->previous == NULL) { // the new section is added before the end section
        end_section->previous = new_section;
        new_section->next = end_section;
    }

    return new_section;
}

void free_tape(section *cursect) {
    section *head = cursect; // this will be the first node in the list

    while (head->previous != NULL) {
        head = head->previous;   // get to the first node
    }
    
    while((cursect = head) != NULL) {   // set current node to first
        head = head->next;              // move to next
        free(cursect);                  // free current
    }

    return;
}

int main(int argc, char *argv[]) {
    char **ip;              // instruction pointer
    int sp;                 // stack pointer
    char *text;             // instruction memory pointer
    char *tp;               // tape pointer
    int fsize;              // size of the program in bytes
    section *cursect;       // tape section pointer
    char *pend;             // program end pointer (end of the program in memory)

    /* loading a program */

    if (argc < 2) {
        printf("Give us a program, mate!\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");  // open the program for reading
   
    if (fp == NULL) {
        printf("Couldn't open the program!\n");
        return 2;
    }

    // get the size of the file so we know how much memory to allocate
    fseek(fp, 0L, SEEK_END);    // seek to end of file
    fsize = ftell(fp);          // program size in bytes
    rewind(fp);                 // rewind the pointer

    // allocate instruction memory
    text = malloc(fsize);
    if (text == NULL) {
        printf("Couldn't allocate program memory!\n");
        return 3;
    }

    // read program into memory from file
    fread(text, 1, fsize, fp);
    fclose(fp); // close the file

    // TODO: parse for matching braces before executing
   
    /* end loading a program */

    ip = malloc(STACKSIZE * sizeof(char *));
    if (ip == NULL) {
        printf("Couldn't allocate stack memory!\n");
        return 4;
    }

    sp = 0;
    ip[sp] = text;

    // initialize tape
    cursect = create_section(NULL);
    for (int i = 1; i < NUMSECTS; i++) {
        cursect->next = create_section(cursect);
        cursect = cursect->next;
    }

    tp = &cursect->cell[NUMCELLS/2]; // middle of the section
    // TODO: calculate the center of ALL sections
    // Use a counter that keeps its value after the loop ends, multiply by NUMCELLS and NUMSECTS, then account for link pointers

    // Main loop
    pend = ip[sp] + fsize;
    do {
        switch(*ip[sp]) {
            case '>':    // >
                if ((tp - cursect->cell) == (NUMCELLS - 2)) { // pointer arithmetic. find array position by subtracting array base address from current position
                    if (cursect->next == NULL) { // if we are at the end of the tape, allocate a new section
                        create_section(cursect);
                    }
                    
                    cursect = cursect->next;    // adjust current section pointer
                    tp = cursect->cell;          // adjust tape pointer to base of new section array
                } else {
                    ++tp;                       // increment tape pointer
                }
                break;
            case '<':    // <
                if (tp == &cursect->cell[0]) { // if we are at the base of the array
                    if (cursect->previous == NULL) {
                        create_section(cursect);
                    }

                    cursect = cursect->previous;        // adjust current section pointer
                    tp = &cursect->cell[NUMCELLS - 1];    // adjust tape pointer to top of new section array
                } else {
                    --tp;
                }
                break;
            case '+':    // +
                (*tp)++;
                break;
            case '-':    // -
                (*tp)--;
                break;
            case '.':    // .
                putchar(*tp);
                break;
            case ',':    // ,
                *tp = getc(stdin);
                break;
            case '[':    // [
                if (*tp == 0) { // search for matching ] and set ip to its address + 1
                    int tsp = sp + 1;
                    char *tip = ip[sp] + 1;
                    while (tsp != sp) {
                        if (*tip == '[') {
                            ++tsp, ++tip;
                        } else if (*tip == ']') {
                            --tsp;
                        }
                    }

                    ip[sp] = tip;   // move ip to the matching ]
                                    // because ip is incremented below, this will actually be tip + 1
                } else {
                    ++sp;                   // increment stack pointer
                    ip[sp] = ip[sp - 1];    // set new ip to old ip
                }
                break;
            case ']':    // ]
                if (*tp != 0) {
                    ip[sp] = ip[sp - 1];    // move ip back to the matching ]
                                            // because ip is incremented below, this will actually be ip[sp - 1] + 1 on the next iteration
                } else {
                    ip[sp - 1] = ip[sp];
                    --sp;           // pop off the stack, we've exited the loop
                }
                break;
        }
    } while (++ip[sp] < pend);
    
    free(ip);           // free ip stack
    free(text);         // free instruction memory
    free_tape(cursect); // free tape memory

    return 0;
}

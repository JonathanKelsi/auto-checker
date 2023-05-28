#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// ----------------- Utility Functions -----------------

void close_files(int f1, int f2) {
    if (close(f1) == -1 || close(f2) == -1) {
        perror("Error in: close");
        exit(-1);
    }
}

int read_char(int fd, char *c) {
    int bytes;
    
    if ((bytes = read(fd, c, 1)) == -1) {
        perror("Error in: read");
        exit(-1);
    }   

    return bytes;
}

int is_space(char c) {
    return c == ' ' || c == '\n';
}

char to_lower_case(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }

    return c;
}

// ----------------- Main Functions -----------------

int is_equal(int f1, int f2) {
    char c1, c2;
    int b1 = 1, b2 = 1;

    while (b1 != 0 && b2 != 0) {
        // Read a character from each file 
        b1 = read_char(f1, &c1);
        b2 = read_char(f2, &c2);

        // Compare the characters
        if (c1 != c2) {
            return 0;
        }
    }

    return b1 == 0 && b2 == 0;
}

int is_similar(int f1, int f2) {
    char c1, c2;
    int b1 = read_char(f1, &c1), b2 = read_char(f2, &c2);

    while (b1 != 0 && b2 != 0) {
        // Check for whitespaces
        if (is_space(c1)) {
            b1 = read_char(f1, &c1);
            continue;
        } 

        if (is_space(c2)) {
            b2 = read_char(f2, &c2);
            continue;
        }

        // Compare the characters
        if (to_lower_case(c1) != to_lower_case(c2)) {
            return 0;
        }

        // Read a character from each file
        b1 = read_char(f1, &c1);
        b2 = read_char(f2, &c2);
    }

    // Read trailing whitespaces
    while (b1 != 0 && is_space(c1)) {
        b1 = read_char(f1, &c1);
    }

    while (b2 != 0 && is_space(c2)) {
        b2 = read_char(f2, &c2);
    }

    return b1 == 0 && b2 == 0;
}

int main(int argc, char *argv[]) {
    // Open the given files
    int f1, f2;

    if ((f1 = open(argv[1], O_RDONLY)) < 0 || (f2 = open(argv[2], O_RDONLY)) < 0) {
        perror("Error in: open");
        return -1;
    }

    // Check if the files are equal
    if (is_equal(f1, f2)) {
        close_files(f1, f2);
        return 1;
    } 

    // Reset the file descriptors
    if (lseek(f1, 0, SEEK_SET) == -1 || lseek(f2, 0, SEEK_SET) == -1) {
        perror("Error in: lseek");
        return -1;
    }

    // Check if the files are similar
    if (is_similar(f1, f2)) {
        close_files(f1, f2);
        return 3;
    }


    close_files(f1, f2);
    return 2;
}
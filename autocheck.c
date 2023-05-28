#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

// ----------------- Exec implementation -----------------

void dummy_handler(int sig) {
    return;
}

int __exec(char *pathname, char *argv[], int timeout, int *alarm_stat, int new_stdin, int new_stdout) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error in: fork");
        exit(-1);
    }

    if (pid == 0) {
        // Add a timeout if needed
        if (timeout != -1) {
            if (signal(SIGALRM, dummy_handler) == SIG_ERR) {
                perror("Error in: signal");
                exit(-1);
            }

            if (alarm(timeout) == -1) {
                perror("Error in: alarm");
                exit(-1);
            }
        }

        // Redirect stdin/stdout if needed
        if (new_stdin != -1) {
            if (dup2(new_stdin, 0) == -1) {
                perror("Error in: dup2");
                exit(-1);
            }
        }

        if (new_stdout != -1) {
            if (dup2(new_stdout, 1) == -1) {
                perror("Error in: dup2");
                exit(-1);
            }
        }

        // Redirect errors to a designated file
        int err_fd;

        if ((err_fd = open("errors.txt", O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
            perror("Error in: open");
            exit(-1);
        }

        if (dup2(err_fd, 2) == -1) {
            perror("Error in: dup2");
            exit(-1);
        }

        // Execute the given command
        if (execvp(pathname, argv) == -1) {
            perror("Error in: execvp");
            exit(-1);
        }

        // Close the error file
        close(err_fd);
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("Error in: waitpid");
        exit(-1);
    }

    if (WIFSIGNALED(status)) {
        *alarm_stat = 1;
    }

    return WEXITSTATUS(status);
}

// ----------------- Grading Stages -----------------

int has_c_src(char *dir, char *name) {
    // Open the given folder
    DIR *dr;

    if ((dr = opendir(dir)) == NULL) {
        perror("Error in: opendir");
        exit(-1);
    }

    // Iterate through it
    struct dirent *de;
    errno = 0;
    int len;

    while(1) {
        // Check for error/end of stream
        if ((de = readdir(dr)) == NULL) {
            if (errno) {
                perror("Error in: open");
                exit(-1);
            }

            break;
        }

        // Check for a C source file
        len = strlen(de->d_name);

        if (de->d_type == DT_REG && len > 2 && strcmp(de->d_name + len - 2, ".c") == 0) {
            strcpy(name, de->d_name);

            if (closedir(dr) == -1) {
                perror("Error in: closedir");
                exit(-1);
            }

            return 1;
        }
    }

    if (closedir(dr) == -1) {
        perror("Error in: closedir");
        exit(-1);
    }

    return 0;
}

int compile_c(char *dir, char *name) {
    // Setup c src path
    char src[256];
    sprintf(src, "%s/%s", dir, name);

    // Compile the C source
    char *argv[] = {"gcc", src, "-o", "student.out", NULL};
    return __exec("gcc", argv, -1, NULL, -1, -1) == 0;
}

int run_exe(char *dir, char *input_path) {
    // Open the input and output files
    int input_fd, output_fd;

    if ((input_fd = open(input_path, O_RDONLY)) == -1 || 
        (output_fd = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
        perror("Error in: open");
        exit(-1);
    }
    
    // Run the executable
    char *argv[] = {"./student.out", NULL};
    int did_timeout = 0;
    int res = __exec("./student.out", argv, 5, &did_timeout ,input_fd, output_fd);

    // Delete the executable
    if (remove("student.out") == -1) {
        perror("Error in: remove");
        exit(-1);
    }

    // Close the files and return the exit code
    if (close(input_fd) == -1 || close(output_fd) == -1) {
        perror("Error in: close");
        exit(-1);
    }

    return !did_timeout;
}

int cmp_outputs(char *dir, char *correct_output_path) {
    // Compare the outputs
    char *argv[] = {"./comp.out", "output.txt", correct_output_path, NULL};
    int res =  __exec("./comp.out", argv, -1, NULL, -1, -1);

    // Delete the student's output
    if (remove("output.txt") == -1) {
        perror("Error in: remove");
        exit(-1);
    }

    return res;
}

// ----------------- Grading Util -----------------

void write_grade(char *name, char *grade, char *reason, int grades_fd) {
    char final_grade[256] = {0};
    sprintf(final_grade, "%s,%s,%s\n", name, grade, reason);
    write(grades_fd, final_grade, strlen(final_grade));
}

// ----------------- Main Function -----------------
 
int main(int argc, char *argv[]) {
    // Open the configuration and grades files
    int conf_fd, grades_fd;

    if ((conf_fd = open(argv[1], O_RDONLY)) < 0 ||
        (grades_fd = open("results.csv", O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0) {
        perror("Error in: open");
        return -1;
    }

    // Get configurations from the conf file
    char conf[512];

    if (read(conf_fd, conf, 512) == -1) {
        perror("Error in: read");
        return -1;
    }

    char *folder_path = strtok(conf, "\n");
    char *input_path = strtok(NULL, "\n");
    char *correct_output_path = strtok(NULL, "\n");  

    // Open the input and output files
    if (access(input_path, F_OK) == -1) {
        char *msg = "Input file not exist\n";
        write(2, msg, strlen(msg));
        return -1;
    }

    if (access(correct_output_path, F_OK) == -1) {
        char *msg = "Correct output file not exist\n";
        write(2, msg, strlen(msg));
        return -1;
    }

    // Open the given folder
    DIR *dr;

    if ((dr = opendir(folder_path)) == NULL) {
        char *msg = "Not a valid directory\n";
        write(2, msg, strlen(msg));
        return -1;
    }

    // Iterate through it
    struct dirent *de;
    errno = 0;

    while(1) {
        // Check for error/end of stream
        if ((de = readdir(dr)) == NULL) {
            if (errno) {
                perror("Error in: open");
                return -1;
            }

            break;
        }

        // Handle student's directory
        if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0 ) {
            // Save the student's directory's full path
            char student_path[512] = {0};
            sprintf(student_path, "%s/%s", folder_path, de->d_name);

            // Save the C source name
            char c_src_name[256] = {0};

            // Check for C files
            if (!has_c_src(student_path, c_src_name)) {
                write_grade(de->d_name, "0", "NO_C_FILE", grades_fd);
                continue;
            }

            // Compile the C file inside
            if (!compile_c(student_path, c_src_name)) {
                write_grade(de->d_name, "10", "COMPILATION_ERROR", grades_fd);
                continue;
            }

            // Run the compiled file with the input
            if (!run_exe(student_path, input_path) != 0) {
                write_grade(de->d_name, "20", "TIMEOUT", grades_fd);
                continue;
            }

            // Compare the output with the correct one
            switch (cmp_outputs(student_path, correct_output_path)) {
                case 1:
                    write_grade(de->d_name, "100", "EXCELLENT", grades_fd);
                    break;
                case 3:
                    write_grade(de->d_name, "75", "SIMILAR", grades_fd);
                    break;
                case 2:
                    write_grade(de->d_name, "50", "WRONG", grades_fd);
                    break;
            }
        }
    }

    // Close open streams
    if (closedir(dr) == -1) {
        perror("Error in: closedir");
        return -1;
    }

    if (close(conf_fd) == -1 || close(grades_fd) == -1) {
        perror("Error in: close");
        return -1;
    }

    return 0;
}
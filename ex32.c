//Rivka Schuss 340903129
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <fcntl.h>

#define GREAT_JOB_STRING "GREAT_JOB"
#define NO_C_FILE_STRING "NO_C_FILE"
#define TIMEOUT_STRING "TIMEOUT"
#define COMP_ERROR "COMPILATION_ERROR"
#define BAD_OUTPUT_STRING "BAD_OUTPUT"
#define SIMILAR_OUTPUT_STRING "SIMILAR_OUTPUT"
#define MAX_FILENAME 160
#define A_OUT "./a.out"
#define COMP_OUT "./comp.out"
#define TEMP_OUT_FILE "output.txt"
#define RESULTS_FILE "results.csv"
#define GCC "gcc"
#define ERROR "Error in system call"
#define INPUT 0
#define OUTPUT 1

/**
 * writes an error to the error file descriptor
 */
void error() {
    write(2, ERROR, sizeof(ERROR));
    exit(-1);

}

/**
 * appends all the information to grade the student and writes it to the results file
 * @param studentName the student name to write
 * @param grade the grade to write
 * @param comment the comment to write
 * @param fd the file descriptor of the results file
 */
void gradeStudent(char* studentName, char *grade, char *comment, int fd) {
    char buffer[MAX_FILENAME];

    //appends all the information to write to the results file
    sprintf(buffer, "%s, %s, %s\n ", studentName, grade, comment);

    //writes to the result file
    if (write(fd, buffer, strlen(buffer + 1)) < 0) {
        printf("error writing to results file");
        error();
    }
}

/**
 * checks whether or not to ignore the empty directories
 * @param dirName the directory name to check
 * @return return 1 to ignore, 0 not to ignore
 */
int checkIgnoreDir(char *dirName) {
    const char *ignoreDirs[] = {".", ".."};
    int i;
    for (i = 0; i < (sizeof(ignoreDirs) / sizeof(ignoreDirs[0])); ++i) {
        if (strcmp(dirName, ignoreDirs[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * compiles the student file using gcc
 * @param studentDirPath the student directory path to the c file to compile
 * @return returns 0 if the compilation failed, 1 if it succeeded
 */
int compileStudentFile(char *studentDirPath) {
    int status;
    //builds the command
    char *args[] = {GCC, studentDirPath, NULL};
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        error();
    } else if (pid == 0) {
        // in child
        printf("in child");
        //runs the compiling command and compiles the student c file
        if (execvp(GCC, args) < 0) {
            // execvp failed
            error();
            printf("failed gcc");
        }

    } else {
        // in parent
        // wait for child
        waitpid(pid, &status, 0);
        //if the compilation failed
        if ((WEXITSTATUS(status) != 0)) {
            return 0;
        }
    }
    return 1;
}


/**
 * runs the student file using a.out
 * @param studentFile the student file to run
 * @param inputFile the input file to use as input
 * @return returns 0 if the run failed, 1 in success
 */
int runStudentFile(char* studentFile, char *inputFile) {
    int status, check;
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        printf("pid error in running");
        error();
    } if (pid == 0) {
        printf("in child");
        //builds the command to run the a.out
        char *args[] = {A_OUT, studentFile, NULL};
        //opens the input file
        int fd = open(inputFile, O_RDONLY);
        if (fd < 0) {
            error();
        }
        //switches the input file to be able to be used as input
        if (dup2(fd, INPUT) < 0) {
            error();
        }
        if (close(fd) < 0) {
            error();
        }

        //opens the output file
        int fd2 = open(TEMP_OUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        //switches the output file to be able to be used as output
        if (dup2(fd2, OUTPUT) < 0) {
            error();
        }
        if (fd2 < 0) {
            error();
        }
        if (close(fd2) < 0) {
            error();
        }
        //runs the file using the input and the output
        execvp(A_OUT, args);
    } else {
        // in parent
        // wait for child
        sleep(5);
        check = waitpid(pid, &status, WNOHANG);
        if (check < 0) {
            error();
            //if the run failed
        } else if (check == 0) {
            return 0;
        }
        if (unlink(A_OUT) < 0) {
            error();
        }
    }
    return 1;
}


/**
 * determines the grade based on the exit status of the comparing program and determines the grade
 * @param exitStatus the exit status of the comparing program
 * @param resultsFd the results file descriptor
 * @param studentName the student name to write
 */
void determineGrade(int exitStatus, int resultsFd, char* studentName) {
    switch(exitStatus) {
        case 1:
            gradeStudent(studentName, "60", BAD_OUTPUT_STRING, resultsFd);
            break;
        case 2:
            gradeStudent(studentName, "80", SIMILAR_OUTPUT_STRING, resultsFd);
            break;
        case 3:
            gradeStudent(studentName, "100", GREAT_JOB_STRING, resultsFd);
            break;
        default:
            break;
    }

}


/**
 * compares the output of the student file to the correct output
 * @param outputFile the correct output
 * @param resultsFd the file descriptor of the results file
 * @param studentName the student name
 */
void compareBeforeGrade(char *outputFile, int resultsFd, char* studentName) {
    int status;
    //builds the command to compare the correct output to the student's output
    char * args[] = { COMP_OUT, TEMP_OUT_FILE, outputFile,  NULL};
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        // in child
        //runs the comparison
        if (execvp(COMP_OUT, args) < 0) {
            error();
        }
    } else if (pid < 0) {
        error();
    } else {
        waitpid(pid, &status, 0);
        determineGrade(WEXITSTATUS(status), resultsFd, studentName);
        //deletes the temporary output
        unlink(TEMP_OUT_FILE);
    }
}


/**
 * checks if the file is a c file
 * @param fileName the file name to check
 * @return returns 1 if the file is a c file, 0 if not
 */
int isCFile(const char fileName[]) {
    int i = 0;
    while (fileName[i] != '\0') {
        if (fileName[i] == '.' && fileName[i + 1] == 'c' && fileName[i + 2] == '\0') {
            return 1;
        }
        i++;
    }
    return 0;
}

/**
 * finds the c file within the folder, and then compiles, runs, and compares the output
 * @param studentDirPath the student directory path
 * @param inputFile the input file
 * @param outputFile the output file
 * @param resultsFd the results file descriptor
 * @param studentName the student name
 * @return returns 1 in success
 */
int findCFile(char *studentDirPath, char *inputFile, char *outputFile, int resultsFd, char* studentName) {
    DIR *innerDir;
    struct dirent *innerDirent;
    char copy[MAX_FILENAME];
    innerDir = opendir(studentDirPath);
    if (innerDir == NULL) {
        error();
    }
    strcpy(copy, studentDirPath);
    while ((innerDirent = readdir(innerDir)) != NULL) {

        memset(studentDirPath, '\0', MAX_FILENAME);
        strcpy(studentDirPath, copy);

        if (isCFile(innerDirent->d_name)) {

            char buffer[MAX_FILENAME];

            //appends the current student name to the path
            sprintf(buffer, "%s/%s", studentDirPath, innerDirent->d_name);

            printf("%s\n", buffer);

            if (!compileStudentFile(buffer)) {
                gradeStudent(studentName, "0", COMP_ERROR, resultsFd);
            } else if(!runStudentFile(buffer, inputFile)) {
                gradeStudent(studentName, "0", TIMEOUT_STRING, resultsFd);
            } else {
                compareBeforeGrade(outputFile, resultsFd, studentName);
            }
            return 1;
            //if the file is not a link and there is a directory to check, return recursively
        } else if (innerDirent->d_name[0] != '.' && innerDirent->d_type == DT_DIR) {
            char buffer[MAX_FILENAME];
            sprintf(buffer, "%s/%s", studentDirPath, innerDirent->d_name);
            findCFile(buffer, inputFile, outputFile, resultsFd, innerDirent->d_name);
        }
    }
}

/**
 * checks the students directory
 * @param studentDirPath the student direct path
 * @param inputFile the input file
 * @param outputFile the output file
 */
void checkStudents(char *studentDirPath, char *inputFile, char *outputFile) {
    char copy[MAX_FILENAME];
    DIR *studentsDir;
    struct dirent *dirInfo;
    //opens the results file
    int studFd = open(RESULTS_FILE, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (studFd < 0) error();

    //opens the student directory
    studentsDir = opendir(studentDirPath);
    if (studentsDir == NULL)
        error();

    strcpy(copy, studentsDir);

    //goes over the entire student directory
    while ((dirInfo = readdir(studentsDir)) != NULL) {

        if (!checkIgnoreDir(dirInfo->d_name) && dirInfo->d_type == DT_DIR) {
            char buffer[MAX_FILENAME];

            sprintf(buffer, "%s/%s", studentDirPath,  dirInfo->d_name);

            //if there is no c file
            if (!findCFile(buffer, inputFile, outputFile, studFd, dirInfo->d_name)) {
                gradeStudent(dirInfo->d_name, "0", NO_C_FILE_STRING, studFd);
            }
            strcpy(buffer, copy);
        }
    }
    if (closedir(studentsDir) < 0) {
        error();
    }
    if (close(studFd) < 0) {
        error();
    }
}

/**
 * reads until the next line
 * @param buffer the buffer to save the path to
 * @param fd the file descriptor of the config file
 */
void readUntilNewline(char *buffer, int fd) {
    char readChar;
    int result;
    while ((result = (int) read(fd, &readChar, 1)) != 0) {
        if (result < 0) {
            error();
        }
        if (readChar == '\n') {
            *(buffer) = '\0';
            break;
        }
        *(buffer++) = readChar;
    }
}

/**
 * reads the configuration file and gets the 3 paths
 * @param configFile the config file path
 * @param studentsDir the students directory path
 * @param inputFile the input file path
 * @param outputFile the output file path
 */
void readConfigFile(char *configFile, char *studentsDir, char *inputFile, char *outputFile) {
    int fd = open(configFile, O_RDONLY);

    if (fd < 0) {
        error();
    }
    //changes the config file to be able to be used for input
    if (dup2(fd, 0) < 0) {
        error();
    }

    readUntilNewline(studentsDir, fd);
    readUntilNewline(inputFile, fd);
    readUntilNewline(outputFile, fd);
}

/**
 * reads the config file and checks the students
 * @param argc num of args
 * @param argv the args
 * @return 0
 */
int main(int argc, char **argv) {

    if (argc != 2) {
        error();
    }

    char inputFile[MAX_FILENAME + 1], outputFile[MAX_FILENAME + 1], studentsFolder[MAX_FILENAME + 1];

    readConfigFile(argv[1], studentsFolder, inputFile, outputFile);

    checkStudents(studentsFolder, inputFile, outputFile);

    return 0;
}


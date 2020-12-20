// Шараковский Юрий М8О-206Б-19
// Лабораторная работа №2
// Вариант: 21

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <stdio.h>

#define BUFSIZE 256
#define SYSEOF 0
#define FAILURE -1
#define SUCCESS 0
#define TRUE 1
#define FALSE 0


int readString(int fd, char* str, int maxsize) {
    char ch;
    int rbytes = 0;
    int ret;
    if ((ret = read(fd, &ch, 1)) < 1)
        return SYSEOF;

    while ((ch == ' ') || (ch == '\n') || (ch == '\t'))
        if (read(fd, &ch, 1) < 1)
            return SYSEOF;

    int i = 0;
    for (; i < maxsize - 1; i++) {
        str[i] = ch;
        rbytes++;
        if (read(fd, &ch, 1) < 1)
            return rbytes;
        if (ch == '\r')
            continue;
        if (ch == '\n')
            break;
    }
    str[++i] = '\0';
    return rbytes;
}

static inline void reverseString(char* dst, char* src, int size) {
    for (int i = size - 1; i >= 0; --i)
        dst[size - i - 1] = src[i];
    return;
}

int parent(int pipe1, int pipe2) {
    char buf[BUFSIZE];
    int size;
    int fd[] = { pipe1, pipe2 };
    int lever = 0;
    int nbytes;
    while ((size = readString(STDIN_FILENO, buf, BUFSIZE)) != SYSEOF) {
        if ((nbytes = write(fd[lever], &size, sizeof(size))) != sizeof(size))
            return FAILURE;
        int p = 0;
        while ((nbytes = write(fd[lever], buf + p, size - p)) != size - p) {
            p += nbytes;
            if (p == size)
                break;
            if (nbytes < 1)
                return FAILURE;
        }
        lever ^= 1;
    }
    return SUCCESS;
}

int child(int input, char filename[BUFSIZE]) {
    int output = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (output < 0)
        return FAILURE;
    char buf[BUFSIZE];
    char rev[BUFSIZE];
    char nl[1];
    nl[0] = '\n';
    int size = 0;
    int nbytes = 0;
    while (TRUE) {
        if ((nbytes = read(input, &size, sizeof(size))) != sizeof(size)) {
            if (nbytes == SYSEOF)
                break;
            return FAILURE;
        }
        if (read(input, buf, size) != size)
            return FAILURE;
        reverseString(rev, buf, size);
        if (write(output, rev, size) != size)
            return FAILURE;
        if (write(output, nl, sizeof(nl)) != sizeof(nl))
            return FAILURE;
    }
    close(output);
    return SUCCESS;
}

int main() {
    int ret;
    char file1[BUFSIZE], file2[BUFSIZE];
    readString(STDIN_FILENO, file1, BUFSIZE);
    readString(STDIN_FILENO, file2, BUFSIZE);

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) != 0)
        return FAILURE;
    if (pipe(pipe2) != 0)
        return FAILURE;

    int pid1 = fork();
    if (pid1 < 0) {
        return FAILURE;
    }
    else if (pid1 == 0) {
        close(pipe2[1]);
        close(pipe2[0]);
        close(pipe1[1]);
        ret = child(pipe1[0], file1);
        close(pipe1[0]);
    }
    else {
        int pid2 = fork();
        if (pid2 < 0) {
            return FAILURE;
        }
        else if (pid2 == 0) {
            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[1]);
            ret = child(pipe2[0], file2);
            close(pipe2[0]);
        }
        else {
            close(pipe1[0]);
            close(pipe2[0]);
            ret = parent(pipe1[1], pipe2[1]);
            close(pipe1[1]);
            close(pipe2[1]);
        }
    }

    return ret;
}
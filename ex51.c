// Omri Fridental 323869545
//

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <wait.h>

int fda[2];

char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return (buf);
}

int main() {
    
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    int notover = 1;
    int stat;

    pipe(fda);
    pid_t f = fork();
    if (f > 0) {
        // father:
        close(fda[0]);
        while (notover) {
            char c = getch();
            if (c != EOF && c != '\n') {
                char buf[1];
                switch (c) {
                    case 'q':
                        notover = 0;
                    case 'a':
                    case 'd':
                    case 's':
                    case 'w':
                        buf[0] = c;
                        int count = write(fda[1], buf, 1);
                        kill(f, SIGUSR2);
                        break;
                    default:
                        continue;

                }
            }
        }
        kill(f, SIGKILL);
        exit(0);

    } else if (f == 0) {
        // child
        // close write side:
        close(fda[1]);

        dup2(fda[0] ,0);
        char* file = strcat(cwd, "/draw.out");
        int r;
        char* prms[2] = {file, NULL};
        r = execvp(prms[0], prms);
        if (r == -1) {fprintf(stderr, "draw.out not found at:%s\n", file); exit(1); }

    } else {
        perror("error in syscall\n");
        exit(1);    
    }
}

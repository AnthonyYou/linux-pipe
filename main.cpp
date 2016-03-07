#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

using namespace std;

const int port = 6666;

int cmdFd[2];
int cmdResultFd[2];

void startChildProcess() {
    dup2(cmdFd[0], 0);
    dup2(cmdResultFd[1], 1);
    dup2(cmdResultFd[1], 2);
    close(cmdFd[0]);
    close(cmdFd[1]);
    close(cmdResultFd[0]);
    close(cmdResultFd[1]);
    execl("/bin/bash", "bash", 0);
}

bool startServer() {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htons(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1) {
        cerr << "failed to create socket" << endl;
        return false;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    if(bind(serverSocket, (sockaddr*)&serverAddr, sizeof(sockaddr)) == -1) {
        cerr << "failed to bind" << endl;
        return false;
    }

    if(listen(serverSocket, 5)) {
        cerr << "failed to listen" << endl;
        return false;
    }

    close(cmdFd[0]);
    close(cmdResultFd[1]);
    fcntl(cmdResultFd[0], F_SETFL, O_NONBLOCK);

    cout << "server start..." << endl;
    char buffer[1024];
    while(true) {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        int client = accept(serverSocket, (sockaddr*)&clientAddr, &len);
        if(client < 0) {
            cerr << "failed to accept" << endl;
            continue;
        }

        while(true) {
            len = recv(client, buffer, sizeof(buffer), 0);
            if(len < 0) {
                break;
            }

            write(1, "receive cmd : ", strlen("receive cmd : "));
            write(1, buffer, len);
            if(strncmp(buffer, "quit\n", len) == 0) {
                close(cmdFd[1]);
                close(cmdResultFd[0]);
                write(client, "frontier_quit", strlen("frontier_quit"));
                close(client);
                goto quit;
            } else if(strncmp(buffer, "\n", 1) == 0) {
                write(client, "\n", 1);
                continue;
            } else if(strncmp(buffer, "exit\n", len) == 0) {
                write(client, "frontier_quit", strlen("frontier_quit"));
                close(client);
                break;
            }
            write(cmdFd[1], buffer, len);

            usleep(100000);

            size_t size = read(cmdResultFd[0], buffer, sizeof(buffer));
            while(size > 0 && size <= sizeof(buffer)) {
                write(client, buffer, size);
                size = read(cmdResultFd[0], buffer, sizeof(buffer));
            }
            write(client, "\n", 1);
        }
    }
    quit:
    close(serverSocket);
    return true;
}

int main()
{
    if(pipe(cmdFd) < 0) {
        cerr << "failed to create pipe" << endl;
        return -1;
    }

    if(pipe(cmdResultFd) < 0) {
        cerr << "failed to create cmd result pipe" << endl;
        return -1;
    }

    pid_t pid = fork();
    if(pid < 0) {
        cerr << "failed to fork" << endl;
        return -1;
    } else if(pid == 0) { // child process
        startChildProcess();
    } else { // parent process
        startServer();
    }
    return 0;
}


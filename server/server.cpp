#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>

class thread_guard {
    public:
    ~thread_guard()
    {
        th.detach();
    }

    thread_guard(std::thread && _th)
    : th(std::move(_th))
    {
        if (!th.joinable()) {
            throw std::logic_error("no thread!\n");
        }
    }

    private:
    std::thread th;
};

class clientHandler {
    public:
    static void sendAll(const char * msg)
    {
        std::lock_guard<std::mutex> lg(mt);

        for (const int & i : s) {
            write(i, msg, strlen(msg));
        }
    }

    static void addClient(int clnt)
    {
        std::lock_guard<std::mutex> lg(mt);

        if (!s.contains(clnt)) {
            s.insert(clnt);
        }
    }

    static void closeAndRemoveClient(int clnt)
    {
        std::lock_guard<std::mutex> lg(mt);

        if (s.contains(clnt)) {
            s.extract(clnt);

            close(clnt);
        }
    }

    private:
    static std::set<int> s;
    static std::mutex mt;
};

std::set<int> clientHandler::s;
std::mutex clientHandler::mt;

void errorHandling(const char * err)
{
    std::cerr << err << '\n';

    exit(1);
}

void handleClient(int clnt)
{
    char buffer[256] {};

    while (true) {
        ssize_t readLen {read(clnt, buffer, sizeof buffer)};

        if (readLen == 0) {
            break;
        }

        if (readLen == -1) {
            errorHandling("read() error");
        }

        buffer[readLen] = '0';

        clientHandler::sendAll(buffer);

        memset(buffer, 0, sizeof buffer);
    }

    clientHandler::closeAndRemoveClient(clnt);
}

int main()
{
    int servSock {socket(PF_INET, SOCK_STREAM, 0)};

    if (servSock == -1) {
        errorHandling("socket() error");
    }

    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof servAddr);

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(9191);

    if (bind(servSock, (sockaddr*)&servAddr, sizeof servAddr) == -1) {
        errorHandling("bind() error");
    }

    if (listen(servSock, 10) == -1) {
        errorHandling("listen() error");
    }

    while (true) {
        sockaddr_in clntAddr;
        socklen_t sockLen {};

        int clntSock {accept(servSock, (sockaddr*)&clntAddr, &sockLen)};

        if (clntSock == -1) {
            errorHandling("accept() error");
        }

        clientHandler::addClient(clntSock);

        thread_guard tg {std::thread(handleClient, clntSock)};
    }

    close(servSock);

    return 0;
}
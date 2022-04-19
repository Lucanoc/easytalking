#include <cstdio>
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>

class thread_guard {
    public:
    ~thread_guard()
    {
        th.join();
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

void errorHandling(const char * cc)
{
	std::cerr << cc << '\n';

	exit(1);
}

void handleWrite(int clnt)
{
	char buffer[256] {};

	while (true) {
		fgets(buffer, sizeof buffer, stdin);

		if (!strcmp(buffer, "exit()\n")) {
			shutdown(clnt, SHUT_WR);

			break;
		}

		write(clnt, buffer, strlen(buffer) + 1);

		memset(buffer, 0, sizeof buffer);
	}
}

void handleRead(int clnt)
{
	char buffer[256] {};

	while (true) {
		ssize_t readLen {read(clnt, buffer, sizeof buffer)};

		if (readLen == 0) {
			close(clnt);

			break;
		}

		if (readLen == -1) {
			errorHandling("read() error");
		}

		fputs(buffer, stdout);

		memset(buffer, 0, sizeof buffer);
	}
}

int main()
{
	int sock {socket(PF_INET, SOCK_STREAM, 0)};

	if (sock == -1) {
		errorHandling("socket() error");
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9191);

	if (connect(sock, (sockaddr*)&addr, sizeof addr) == -1) {
		errorHandling("connect() error");
	}

	thread_guard tgRead {std::thread(handleRead, sock)};
	thread_guard  tgWrite {std::thread(handleWrite, sock)};

	return 0;
}
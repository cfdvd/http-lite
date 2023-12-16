#include "MyHttp.hpp"
#include "fmt/format.h"

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1) {
        perror("socket(AF_INET, SOCK_STREAM, 0)");
        exit(-1);
    }

    auto server_address = MyHttp::CreateSocketAddr(HTTP_PORT, INADDR_ANY);
    if(MyHttp::MyBind(server_socket, server_address) == -1) {
        return -1;
    }

    int ret = listen(server_socket, 128);
    if(ret == -1) {
        perror("listen(server_socket, 128)");
        exit(-1);
    }
    std::cout << "wait client connect...\n";

    while(true) {
        struct sockaddr_in client_address {};
        bzero(&client_address, sizeof(struct sockaddr_in));

        auto client_socket = MyHttp::MyAccept(server_socket, client_address);

        std::array<char, 32> client_ip {};
        fmt::print("client ip: {}, port: {}\n", inet_ntop(AF_INET, &client_address.sin_addr.s_addr, client_ip.data(), client_ip.size()), ntohs(client_address.sin_port));

        std::thread thread (MyHttp::AnalysisHTTP, client_socket);
        thread.detach();
    }
    close(server_socket);

    return 0;
}
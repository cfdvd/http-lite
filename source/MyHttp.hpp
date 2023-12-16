#ifndef __MY_HTTP_HPP
#define __MY_HTTP_HPP

#include <iostream>
#include <format>
#include <type_traits>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <optional>
#include <thread>
#include <regex>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

template <typename... T> inline __attribute__((__always_inline__)) void ignore_unused(T &&...) {}

namespace MyHttp {

#define CONCAT_INNER(x, y) x##y
#define CONCAT(x, y) CONCAT_INNER(x, y)
#define TOSTRING_INNER(x) #x
#define TOSTRING(x) TOSTRING_INNER(x)

#define HTTP_PORT 8080

#define DEBUG
#ifdef DEBUG
    constexpr static const int Debug = 1;
#endif

#define ANALYSIS_SEGMENT 0
#define ANALYSIS_QUERY 1

    using socket_type = int;
    using socketAddr_type = struct sockaddr_in;
    using port_type = int16_t;
    using ipv4_type = int32_t;

    using file_ptr = std::unique_ptr<FILE, int(*)(FILE *)>;
    auto CreateFilePoint(std::string_view fileName, std::string_view mode = "r") {
        return file_ptr(std::fopen(fileName.data(), mode.data()), std::fclose);
    }

    template <int Switch>
    std::optional<std::unordered_map<std::string, std::string> > AnalysisKeyValue(std::string_view str) {
        assert(!str.empty());
        std::unordered_map<std::string, std::string> ret {};

        std::regex keyValueReg;
        if constexpr (Switch){
            keyValueReg = R"(([^=&]*)={1}([^=&]*))";
        } else {
            keyValueReg = R"(([^:]*):?\s(([^:]*)|([\d.]*:?\d*))\n)";
        }
        std::cregex_iterator regIter (str.begin(), str.end(), keyValueReg);
        std::cregex_iterator regEnd {};
        for(; regIter != regEnd; ++regIter) {
            ret.try_emplace((*regIter)[1], (*regIter)[2]);
        }

        return {std::move(ret)};
    }

    std::string GetLine(int socket) {
        std::string ret {};
        int cnt = 0;

        char ch = '\0';
        for(;ch != '\n';) {
            cnt = read(socket, &ch, 1);
            if(ch == '\r') { continue; }
            if(cnt == 0) {
                std::cerr << "this socket has been closed !\n";
                return ret;
            }
            if(cnt == -1) {
                perror("read(socket, &ch, 1)");
                return "";
            }
            ret.push_back(ch);
        }
        if(*(ret.end() - 1) == '\n') {
            ret.pop_back();
        }

        return ret;
    }

    std::string FileToString(file_ptr &file) {
        char ch = '\0';
        std::string tmp {};
        tmp.clear();
        while((ch = fgetc(file.get())) != EOF) {
            tmp.push_back(ch);
        }
        return tmp;
    }

    struct SendHttpPacket {
        virtual int operator()(int socket, std::string_view fileName) {
            ignore_unused(socket, fileName);
            return 0;
        }

        static const std::string http_200_head;
        static const std::string http_404_head;
        static const std::string http_400_head;
        static const std::string http_500_head;
        static const std::string http_501_head;
    };
    const std::string SendHttpPacket::http_200_head {"HTTP/1.1 200 OK\r\nServer: Archer Server\r\nContent-Type: text/html;\r\nConnection: Close\r\nContent-Length: "};
    const std::string SendHttpPacket::http_404_head {"HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n"};
    const std::string SendHttpPacket::http_400_head {"HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n"};
    const std::string SendHttpPacket::http_500_head {"HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n"};
    const std::string SendHttpPacket::http_501_head {"HTTP/1.1 501 Method Not Implemented\r\nContent-Type: text/html\r\n\r\n"};

    struct SendHttp_200: public SendHttpPacket {
        int operator()(int socket, std::string_view fileName) override {
            file_ptr file = CreateFilePoint(fileName);
            if(file == nullptr) { return -1; }
            auto httpBody = FileToString(file);
            std::string head = SendHttpPacket::http_200_head + std::to_string(httpBody.size()) + "\r\n\r\n";

            write(socket, head.data(), head.size());
            write(socket, httpBody.data(), httpBody.size());

            return 0;
        }
        static SendHttp_200 Compile() { return SendHttp_200{}; }
    };

    template <int Code>
    struct SendHttp_Other: public SendHttpPacket {
        int operator()(int socket, std::string_view fileName) override {
            file_ptr file = CreateFilePoint(fileName);
            if(file == nullptr) { return -1; }
            auto httpBody = FileToString(file);

            if constexpr (Code == 400) {
                write(socket, SendHttpPacket::http_400_head.data(), SendHttpPacket::http_400_head.size());
            } else if constexpr(Code == 404) {
                write(socket, SendHttpPacket::http_404_head.data(), SendHttpPacket::http_404_head.size());
            } else if constexpr(Code == 500) {
                write(socket, SendHttpPacket::http_500_head.data(), SendHttpPacket::http_500_head.size());
            } else if constexpr(Code == 501) {
                write(socket, SendHttpPacket::http_501_head.data(), SendHttpPacket::http_501_head.size());
            }
            write(socket, httpBody.data(), httpBody.size());

            return 0;
        }
        static SendHttp_Other<Code> Compile() { return SendHttp_Other<Code>{}; }
    };

    socketAddr_type CreateSocketAddr(port_type port, ipv4_type ipv4Addr) {
        socketAddr_type server_address {};
        bzero(&server_address, sizeof(struct sockaddr_in));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = htonl(ipv4Addr);
        return server_address;
    }

    socket_type MyBind(socket_type socket, socketAddr_type &socketAddr) {
        socket_type ret = bind(socket, (struct sockaddr *)(&socketAddr), sizeof(socketAddr_type));
        if(ret == -1) {
            perror("bind(socket, (struct sockaddr *)(&socketAddr), sizeof(socketAddr_type))");
            return -1;
        }
        return ret;
    }

    socket_type MyAccept(socket_type socket, socketAddr_type &socketAddr) {
        uint32_t len = sizeof(socketAddr_type);
        socket_type ret = accept(socket, (struct sockaddr *)(&socketAddr), &len);
        if(ret == -1) {
            perror("accept(socket, (struct sockaddr *)(&socketAddr), &len)");
            return -1;
        }
        return ret;
    }


    void AnalysisHTTP(int socket) {
        std::string httpHead = GetLine(socket);
        std::regex headReg (R"(^(\w+)\s/(([^;?#]*)(;([^\?#]*))?(\?([^;#]*))?(#([^;\?]*))?)\sHTTP/((\d+)(.\d+)?)$)", std::regex::icase);
        std::cmatch what {};
        std::regex_match(httpHead.c_str(), httpHead.c_str() + httpHead.size(), what, headReg);

        if(what.empty()) {
            std::unique_ptr<SendHttpPacket> func = std::make_unique<SendHttp_Other<400> >(SendHttp_Other<400>::Compile());
            (*func)(socket, "../html/Http_400.html");
        }

        std::string httpMethod {what[1].str()};
        std::string httpUrl {what[2].str()};
        std::string httpUrlPath {what[3].str()};
        std::string httpUrlParam {what[5].str()};
        std::string httpUrlQuery {what[7].str()};
        std::string httpUrlFragment {what[9].str()};
        std::string httpVer {what[10].str()};

//        if(!httpUrlQuery.empty()) {
//            auto httpUrlQueryMap = AnalysisKeyValue<ANALYSIS_QUERY>(httpUrlQuery);
//        }

        std::string segment {};
        std::string tmp {};
        while(!((tmp = GetLine(socket)).empty())) {
            segment.append(tmp + "\n");
        }
//        if(!segment.empty()) {
//            auto segmentMap = AnalysisKeyValue<ANALYSIS_SEGMENT>(segment);
//        }

        if(httpMethod == "GET") {
            struct stat st {};
            bzero(&st, sizeof(struct stat));
            std::string path {"../html/" + httpUrlPath};
            int status = stat (path.data(), &st);

            if(S_ISREG(st.st_mode) && status != -1) {
                std::unique_ptr<SendHttpPacket> func = std::make_unique<SendHttp_200>(SendHttp_200::Compile());
                (*func)(socket, path);
            } else {
                std::unique_ptr<SendHttpPacket> func = std::make_unique<SendHttp_Other<404> >( SendHttp_Other<404>::Compile());
                (*func)(socket, "../html/Http_404.html");
            }
        } else {
            std::unique_ptr<SendHttpPacket> func = std::make_unique<SendHttp_Other<501> >(SendHttp_Other<501>::Compile());
            (*func)(socket, "../html/Http_501.html");
        }

        close(socket);
    }
}

#endif

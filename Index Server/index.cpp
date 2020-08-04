#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <mutex>

#define NUM_CLIENTS 50

std::map<std::string, std::string> clientMap;
std::mutex service_mutex;

void serviceConn(int sockfd)
{
    struct sockaddr_in clientIP;
    socklen_t b = sizeof(clientIP);

    char buffer[50];
    char fileName[23];
    char fileIP[20];
    int connfd, n;

    while (1)
    {
        service_mutex.lock();
        connfd = accept(sockfd, (struct sockaddr*)&clientIP, &b);
        n = read(connfd, buffer, sizeof(buffer));
        std::string message(buffer);
        if(message == "Registration"){
            std::cout << "[INFO] Registration Updated" << std::endl;
            do {
                n = read(connfd, buffer, sizeof(buffer));
                std::stringstream fileIP(buffer);
                std::string file, ip;
                std::getline(fileIP, file, ',');
                std::getline(fileIP, ip, ',');
                clientMap.emplace(file, ip);
            } while(n > 0);

            for(auto client: clientMap){
                std::cout << "[INFO] Registered : " << client.first << " : " << client.second << std::endl;
            }
        }
        else
        {
            std::string mapFile(buffer);
            std::string mapIP;
            mapIP = clientMap[mapFile];
            mapIP.copy(fileIP, mapIP.size() + 1);
            write (connfd, fileIP, 20);

            std::cout << "[INFO] Redirecting : " << mapFile << " : " << mapIP << " from Thread " << std::this_thread::get_id() << std::endl;
        }
        close(connfd);
        service_mutex.unlock();
    }

}

void bindSocket(int sockfd)
{
    struct sockaddr_in serverIP;

    memset(&serverIP, 0, sizeof(serverIP));
    serverIP.sin_family = AF_INET;
    serverIP.sin_addr.s_addr = INADDR_ANY;
    serverIP.sin_port = htons(8080);

    bind (sockfd, (struct sockaddr*)&serverIP, sizeof(serverIP));
    listen(sockfd, NUM_CLIENTS);
}

int main (int argc, char *argv[])
{
    int sockfd;
    std::thread serviceThreads[NUM_CLIENTS];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bindSocket(sockfd);

    std::cout << "[INFO] Registration Server Running" << std::endl;
    for(int i = 0; i < NUM_CLIENTS; i++)      serviceThreads[i] = std::thread(serviceConn, sockfd);
    for(int i = 0; i < NUM_CLIENTS; i++)      serviceThreads[i].join();

    close(sockfd);
}
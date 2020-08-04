#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <netdb.h>
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#define BUF_SIZE 1025
#define NO_CLIENTS 50

std::mutex service_mutex;

/* Register the files with the Indexing Server */
void registerFiles(int sockfd, char* directory, char* currServer){
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (directory)) != NULL) {
    write(sockfd, "Registration", 50);
    while ((ent = readdir (dir)) != NULL) {
      char buffer[50];
      std::string fileName(ent->d_name);
      std::string ip(currServer);
      std::string temp(directory);
      temp += fileName + "," + ip;
      strcpy(buffer, temp.c_str());
      write(sockfd, buffer, 50);
    }
    closedir (dir);
  }
}

/* Send the file to the socket */
void sendFile(int sockfd, char* file){
    char buffer[BUF_SIZE];
    int fd, n, len, end;
    fd = open (file, O_RDONLY);
    end = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    len = 0;
    do
    {
       n = read (fd, buffer, sizeof(buffer));
       write (sockfd, buffer, n);
       len += n;
    } while (len < end);

//    std::cout << "[INFO]" << len << "/" << end << std::endl;
    close(fd);
}

/* Receive the file from the socket */
void receiveFile(int sockfd, char* file)
{
    int fd, n, len = 0, cnt = 0;
    char buffer[BUF_SIZE];
    n = write (sockfd, file, strlen(file));

    fd = open (file, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR);
    do {
        n = read (sockfd, buffer, sizeof(buffer) - 1);
        len += n;
        if (cnt == 1024){
            std::cout << "=";
            cnt = 0;
        }
        fflush(stdout);
        write (fd, buffer, n);
        cnt++;
    } while (n > 0);

    std::cout << std::endl;
    close (fd);
}

void connectHost(int sockfd, char* hostName)
{
    struct sockaddr_in serverIP;
    struct hostent *h = gethostbyname(hostName);

    memset(&serverIP, 0, sizeof(serverIP));
    serverIP.sin_family = AF_INET;
    serverIP.sin_addr.s_addr = ((struct in_addr*)(h->h_addr))->s_addr;
    serverIP.sin_port = htons(8080);

    connect (sockfd, (struct sockaddr *)&serverIP, sizeof(serverIP));
}

void bindSocket(int sockfd)
{
    struct sockaddr_in serverIP;

    memset(&serverIP, 0, sizeof(serverIP));
    serverIP.sin_family = AF_INET;
    serverIP.sin_addr.s_addr = INADDR_ANY;
    serverIP.sin_port = htons(8080);

    bind (sockfd, (struct sockaddr*)&serverIP, sizeof(serverIP));
    listen(sockfd, NO_CLIENTS);
}

void serviceConn(int sockfd)
{
    struct sockaddr_in clientIP;
    socklen_t b = sizeof(clientIP);

    char fileName[23];
    int connfd, filelen;
    while (1)
    {
        service_mutex.lock();
        connfd = accept(sockfd, (struct sockaddr*)&clientIP, &b);
        filelen = read(connfd, fileName, sizeof(fileName));
        fileName[filelen] = 0;
  //      std::cout << fileName << ":" << "sent: " << " from " << std::this_thread::get_id() << std::endl;
        sendFile(connfd, fileName);
        close(connfd);
        service_mutex.unlock();
    }
}

void downloadfile(char* file, char* index){
    char clientIP[20];
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    connectHost(clientfd, index);
    write(clientfd, file, 23);
    read(clientfd, clientIP, 20);
    close (clientfd);

    std::cout << "[INFO] Downloading file from : " << std::string(clientIP) << std::endl;

    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    connectHost(clientfd, clientIP);
    receiveFile(clientfd, file);
    close(clientfd);
}

int main(int argc, char* argv[]){
    /*
    argv[1] = registration mode / server-client mode
    argv[2] = File Input / File Location
    argv[3] = Index Server IP Address
    argv[4] = Peer IP Address
    */

    int serverfd;
    char file[50];
    std::string fileName;
    std::thread connThreads[NO_CLIENTS];

    if(argc < 4 && std::string(argv[1]) != "i"){
        std::cout << "Invalid Registration Arguments" << std::endl;
        return -1;
    }

    if(std::string(argv[1]) == "r"){
        serverfd = socket(AF_INET, SOCK_STREAM, 0);
        connectHost(serverfd, argv[3]);
        registerFiles(serverfd, argv[2], argv[4]);
        close(serverfd);
        std::cout << "Registration Complete" << std::endl;
        return 0;
    }
    else {
        serverfd = socket(AF_INET, SOCK_STREAM, 0);
        bindSocket(serverfd);
        for(int i = 0; i < NO_CLIENTS; i++)        connThreads[i] = std::thread(serviceConn, serverfd);
        std::cout << "Waiting for all servers to be up for batch run" << std::endl;
        std::cin.get();
        if(std::string(argv[1]) == "b"){
            std::ifstream myFile(argv[2]);
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            while(std::getline(myFile, fileName)){
                strcpy(file, fileName.c_str());
                downloadfile(file, argv[3]);
            }
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;
        }
        else if (std::string(argv[1]) == "i"){
            while(1){
                std::cout << "Enter the file for download: ";
                std::cin >> fileName;
                strcpy(file, fileName.c_str());
                downloadfile(file, argv[2]);
            }
        }

        for(int i = 0; i < NO_CLIENTS; i++)        connThreads[i].join();
        close(serverfd);
    }
}
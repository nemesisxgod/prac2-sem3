#include <iostream>
#include <string>
#include <thread> // создание и управление потоками
#include <mutex>    // синхронизация потоков
#include <netinet/in.h>
#include <arpa/inet.h> // преобразование адресов между строками и сет. структурами
#include <unistd.h>
#include <sstream>
#include "header.h"

using namespace std;

mutex dbMutex; // защищает таблицу от одновременной модификации
mutex clientMutex; // защищает доступ к счетчику
int activeClientsCount = 0; 

void handleClient(int clientSocket, TableJson& jstab) {

    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break; 

        string command(buffer, bytesReceived);
        istringstream cmnd(command);
        string firstMessage;
        cmnd >> firstMessage;

        dbMutex.lock();
        try {
        if (firstMessage == "INSERT") {
            cout << "insertion is in progress" << endl;
            insert(command, jstab);
        } else if (firstMessage == "DELETE") {
            cout << "deletion is in progress" << endl;
            strDelete(command, jstab);
        } else if (firstMessage == "SELECT") {
            cout << "selection is in progress" << endl;
            select(command, jstab);
        } else if (firstMessage == "EXIT") {
            dbMutex.unlock();
            break;
        }
        else cout<<"unknown command"<<endl;
        } catch (const exception& e) {
            cerr << "error: " << e.what() << endl;
        }
        dbMutex.unlock();

        send(clientSocket, "the command was sent\n", 21, 0); 
    }

    cout << "client " << activeClientsCount << " disconnected"  << endl;

    close(clientSocket);

    lock_guard<mutex> lock(clientMutex); // уменьшает счетчик клиентов
    activeClientsCount--;

    if (activeClientsCount == 0) {
        cout << "server shuts down. \n";
        exit(0);
    }
}


int main() {
    TableJson jstab;
    parser(jstab);

    // создание сокета
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(7432);
    serverAddress.sin_addr.s_addr = INADDR_ANY; // принимает подкл на всех доступных интерфейсах

    // привязка сокета к адресу и порту
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
    listen(serverSocket, 3); // перревод в режим ожидания подключений

    cout << "server is running \n";

    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLength);

        if (clientSocket < 0) {
            cerr << "error connecting client \n";
            continue;
        }

        // определение айпи клиента
        string clientIp = inet_ntoa(clientAddress.sin_addr);
        cout << "client " << (activeClientsCount + 1) << " connected: " << clientIp << endl;

        lock_guard<mutex> lock(clientMutex);
        activeClientsCount++;

        // создает новый поток для клиента
        thread clientThread(handleClient, clientSocket, ref(jstab));
        clientThread.detach(); // отсоединение потока для независимого выполнения
    }

    close(serverSocket);
    return 0;
}
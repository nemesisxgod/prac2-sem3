#include <iostream>
#include <string>
#include <netinet/in.h>     // сетевые соединения
#include <unistd.h>         // системные вызовы

using namespace std;

int main() {
    // создаем клиентский сокет
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0); // возвращает дескриптор сокета
    sockaddr_in serverAddress; // параметры адреса сервера
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(7432);
    serverAddress.sin_addr.s_addr = INADDR_ANY; // сервер на любом локальном IP

    // установка соединения с сервом
    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    string command;
    while (true) {
        cout << "waiting for the command: ";
        getline(cin, command);

        if (command.empty()) 
            continue;

        // отправка команды серву
        send(clientSocket, command.c_str(), command.size(), 0);

        if (command == "EXIT") 
            break;

        char buffer[1024]; // для приема ответа от серва (1024 байта)
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0); // возвращает кол-во байт
        if (bytesReceived > 0) {
            cout << "server: " << string(buffer, bytesReceived) << endl; // выводит то, что ответил серв
        }
    }
    close(clientSocket);
    return 0;
}
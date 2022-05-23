#pragma once

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>

#pragma warning(disable: 4996)
SOCKET ConnectedSocket = INVALID_SOCKET;
void ClientHandler() {
	int msg_size;
	while (true) {
		int result=recv(ConnectedSocket, (char*)&msg_size, sizeof(int), NULL);
		if (result < 1) {
			std::cout << "I guess battlefield does not exist anymore";
			break;
		}
		char* msg = new char[msg_size + 1];
		msg[msg_size] = '\0';
		recv(ConnectedSocket, msg, msg_size, NULL);
		std::cout << msg << std::endl;
		delete[] msg;
	}
}

void GetServers() {
	int result;
	SOCKET updSocket = socket(AF_INET, SOCK_DGRAM, NULL);
	if (updSocket == INVALID_SOCKET) {
		std::cout << "socket error";
	}
	int optval = 1;
	result = setsockopt(updSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval));
	if (result == SOCKET_ERROR)
	{
		std::cout << "Error";
	}
	SOCKADDR_IN serv;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(8080);
	serv.sin_addr.s_addr = INADDR_BROADCAST;
	
	const char* message = "1232";
	result = sendto(updSocket, message, sizeof(message), 0, (sockaddr*)&serv, sizeof(serv));
	if (result == SOCKET_ERROR)
	{
		std::cout << "Sending broadcast error";
		return;
	}
	char receiveBuffer[256];
	int servLength = sizeof(serv);
	while (ConnectedSocket == INVALID_SOCKET)
	{
		result = recvfrom(updSocket, receiveBuffer, 255, NULL, (sockaddr*)&serv, &servLength);
		if (result == SOCKET_ERROR)
		{
			std::cout << "Receiving message is failed";
			return;
		}
		if (strcmp(receiveBuffer, "123") == 0) {
			char serveradr[100];
			PCSTR ip = inet_ntop(serv.sin_family, &serv.sin_addr.S_un.S_addr, (PSTR)&serveradr, sizeof(serveradr));
				std::cout << "Server ip: " << ip << std::endl;
		}
	}
}

void StartClient(std::string adress) {
	ADDRINFO hints;
	ADDRINFO* addrResult = NULL;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int result = getaddrinfo(adress.c_str(), "1111", &hints, &addrResult);
	if (result != 0)
	{
		std::cout << "Get addrindo failed with error" << result << std::endl;
		WSACleanup();
		exit(1);
	}

	ConnectedSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ConnectedSocket == INVALID_SOCKET) {
		std::cout << "Socket creation failed" << result << std::endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		exit(1);
	}

	result = connect(ConnectedSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (result == SOCKET_ERROR) {
		std::cout << "Error: failed connect to server.\n";
		closesocket(ConnectedSocket);
		ConnectedSocket = INVALID_SOCKET;
		freeaddrinfo(addrResult);
		WSACleanup();
		return;
	}
	std::cout << "Connected to Server!\n";
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

	std::string msg1;
	while (true) {
		std::cin.clear();
		std::getline(std::cin, msg1);
		int num;
		try
		{
			num = stoi(msg1);
		}
		catch (const std::exception&)
		{
			std::cout << "Input value have to be int number \n";
			continue;
		}
		if (num < 1) {
			std::cout << "Wrong number \n";
			continue;
		}
		int msg_size = msg1.size();
		send(ConnectedSocket, (char*)&msg_size, sizeof(int), NULL);
		send(ConnectedSocket, msg1.c_str(), msg_size, NULL);
		Sleep(10);
	}
}
int main(int argc, char* argv[]) {
	WSAData wsaData;
	int result;

	WORD DLLVersion = MAKEWORD(2, 1); 
	result = WSAStartup(DLLVersion, &wsaData);
	if (result != 0) { 
		std::cout << "WAStartupp Error" << result << std::endl; 
		exit(1);
	}
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetServers, NULL, NULL, NULL);
	Sleep(20);
	std::cout << "Write chosen server ip: \n";
	std::string adress;
	std::getline(std::cin, adress);
	StartClient(adress);
	system("pause");
	return 0;
}

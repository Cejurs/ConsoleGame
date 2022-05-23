#pragma once

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <string>
#include <WS2tcpip.h>
#include <mutex>


#pragma warning(disable: 4996)
SOCKET Connections[100];
int Counter = 0;
std::mutex mutex;
int bossHP = 10000;
int maxBossHP = 10000;
bool isBleeding = false;
int bleedCount = 0;
int bossCounterAttackChance = 10;


void SendToCurrentPlayer(std::string message, int index)
{
	int size = message.length() + 1;
	send(Connections[index], (char*)&size, sizeof(int), NULL);
	send(Connections[index], message.c_str(), size, NULL);
}

void SendToAllPlayers(std::string message)
{
	int mes_size = message.length() + 1;
	for (int i = 0; i < Counter; i++) {
		if (Connections[i] != 0) {
			send(Connections[i], (char*)&mes_size, sizeof(int), NULL);
			send(Connections[i], message.c_str(), mes_size, NULL);
		}
	}
}

void Bleed() {
	if (isBleeding)
	{
		if (bleedCount <= 0)
		{
			isBleeding = false;
			return;
		}
		bossHP -= 10;
		bleedCount--;
		std::string message = "Boss is bleeding! BossHP: " + std::to_string(bossHP)+"/"+ std::to_string(maxBossHP);
		SendToAllPlayers(message);
	}
}
bool isEvased(int damage){
	int random = rand() % 100+1;
	int chance;
	if(damage>0 && damage<10){
		chance = 15;
	}
	if (damage >= 10 && damage < 30) {
		chance = 25;
	}
	if (damage >= 30 && damage < 50) {
		chance = 45;
	}
	if (damage >=50 && damage < 80) {
		chance = 65;
	}
	if (damage >= 80 && damage < 100) {
		chance = 85;
	}
	if (damage >= 100) {
		chance = 99;
	}
	if(random > chance) return false;
	return true;
}

bool isCounterAttacked(){
	int random = rand() % 100;
	if (random > bossCounterAttackChance) return false;
	return true;
}
void CreateNewBoss() {
	int basicCounter = 15;
	maxBossHP *= 1.05;
	bossHP = maxBossHP;
	isBleeding = false;
	bleedCount = 0;
	bossCounterAttackChance = basicCounter * (rand() % 200) / 100;
}

void ClientHandler(int index) {
	int msg_size;
	int playerhp = 100;
	int bleedingHitChance = rand() % 5 + 1;
	while (true) {
		if (playerhp <= 0)
		{
			std::string strForOthers = "Player " + std::to_string(Connections[index]) + " has been killed by boss";
			std::string strForThisPlayer = "Unfortunately you died";
			SendToAllPlayers(strForOthers);
			SendToCurrentPlayer(strForThisPlayer, index);
			closesocket(Connections[index]);
			Connections[index] = 0;
			break;
		}
		int bytw = recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
		if (bytw < 1) {
			std::string str = "Player " + std::to_string(Connections[index]) + " cowardly ran away from the battlefield";
			std::cout << Connections[index] << " " << " Disconnect\n";
			closesocket(Connections[index]);
			Connections[index] = 0;
			SendToAllPlayers(str);
			break;
		}
		else {
			char* msg = new char[msg_size + 1];
			msg[msg_size] = '\0';
			std::string indexstr = std::to_string(Connections[index]);
			int bytesReceive = recv(Connections[index], msg, msg_size, NULL);
			int damage = atoi(msg);
			mutex.lock();
			if (bossHP <= 0) CreateNewBoss();
			Bleed();
			if (isCounterAttacked())
			{
				playerhp -= damage;
				std::string str = "Player " + indexstr + " has been counter attacked by boss! PlayerH HP: " +std::to_string(playerhp)+"/"+std::to_string(100);
				SendToAllPlayers(str);
			}
			if (isEvased(damage))
			{
				std::string str = "Player " + indexstr + " missed!";
				SendToAllPlayers(str);
			}
			else {
				bossHP -= damage;
				int random = rand() % 100 + 1;
				if (random < bleedingHitChance)
				{
					isBleeding = true;
					bleedCount = rand() % 5 + 1;
				}
				std::string str = "Player " + indexstr + " hits boss! Boss HP: " + std::to_string(bossHP)+"/" + std::to_string(maxBossHP);
				SendToAllPlayers(str);
			}
			mutex.unlock();
		}
	}
}

void StartServer() {
	ADDRINFO hints;
	ADDRINFO* addrResult = NULL;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int result = getaddrinfo(NULL, "1111", &hints, &addrResult);
	if (result != 0)
	{
		std::cout << "Get addrinfo failed with error" << result << std::endl;
		WSACleanup();
		exit(1);
	}

	SOCKET ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		std::cout << "Socket creation failed" << result << std::endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		exit(1);
	}
	result = bind(ConnectSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (result == SOCKET_ERROR) {
		std::cout << "Error: binding socket failed.\n";
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		freeaddrinfo(addrResult);
		WSACleanup();
		return;
	}

	result = listen(ConnectSocket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		std::cout << "Error: listening socket failed.\n";
		closesocket(ConnectSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
		return;
	}
	std::cout << "Server is started. \n";
	std::cout << "Waiting for connection... \n";

	SOCKET newConnection;
	for (int i = 0; i < 100; i++) {
		newConnection = accept(ConnectSocket, NULL, NULL);

		if (newConnection == 0) {
			std::cout << "Error #2\n";
		}
		else {
			std::cout << "Client Connected! " << newConnection << "\n";
			std::string str = "Hello knight, other heroes need your help. Take your arms player " + std::to_string(newConnection)+"!";
			int msg_size = (int)str.size();
			send(newConnection, (char*)&msg_size, sizeof(int), NULL);
			send(newConnection, str.c_str(), msg_size, NULL);
			for (int i = 0; i < Counter; i++) {
				if (Connections[i] != 0) {
					std::string str = "Player " + std::to_string(newConnection) + " join to the fight";
					int mes_size = sizeof(str);
					send(Connections[i], (char*)&mes_size, sizeof(int), NULL);
					send(Connections[i], str.c_str(), sizeof(str), NULL);
				}
			}

			Connections[i] = newConnection;
			Counter++;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
		}
	}
}

void UDPRequestHandler()
{
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, NULL);
	if (udpSocket == INVALID_SOCKET) {
		std::cout << "UDPSocket init fail"<< std::endl;
		WSACleanup();
		exit(1);
	}

	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(8080);
	server.sin_addr.s_addr = INADDR_ANY;

	int result = bind(udpSocket, (LPSOCKADDR)&server, sizeof(server));
	if (result == SOCKET_ERROR) {
		std::cout << "UDPSocket bind is failed\n";
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
		WSACleanup();
		exit(1);
	}
	sockaddr_in client;
	int clientLength = sizeof(client);
	ZeroMemory(&client, clientLength);

	char buffer[1024];
	while (1)
	{
		ZeroMemory(buffer, 1024);

		int bytesReceive = recvfrom(udpSocket, buffer, 1024, 0,(sockaddr*)&client, &clientLength);

		if (bytesReceive == SOCKET_ERROR) {
			std::cout << "Error receiving from client" << WSAGetLastError()<<std::endl;
			continue;
		}

		char clientIP[256];
		ZeroMemory(clientIP, 256);

		inet_ntop(AF_INET, &client.sin_addr, clientIP, 256);
		std::cout << "Broadcast Message recv from " << clientIP << "\n" << std::endl;

		const char* message = "123";

		int bytesSend = sendto(udpSocket, message, sizeof(message), 0, (sockaddr*)&client, clientLength);
		if (bytesSend == SOCKET_ERROR) {
			std::cout << "Error sending to client" << WSAGetLastError() << std::endl;
		}


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
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)UDPRequestHandler, NULL, NULL, NULL);
	StartServer();
	system("pause");
	return 0;
}


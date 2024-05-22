#pragma once

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#define MAX_CONNECTIONS	10

struct Room {

	Room(std::string Name, std::string Password) {
		name = Name;
		password = Password;
	}

	std::string name;
	std::string password;
	std::vector<int> users;
};

enum Packet {
	pChatMessage,
	pServerMessage,
	pWarningMessage,
	pRegister,      // ����� ����� ��� �����������
	pLogin          // ����� ����� ��� �����
};

bool handleRegister(SOCKET clientSocket, std::string message);
bool handleLogin(SOCKET clientSocket, std::string message);
void setColor(const char* color);

std::vector<std::string> stringSplit(std::string str, std::string delimiter);

std::string getCurrentTime();

void sendPacket(SOCKET* socket, Packet* packetType, std::string* message);

void MessageHandler(int userID, std::string msg, int* roomID);

bool processPacket(int index, Packet packetType, int* roomID);

void clientHandler(int index);
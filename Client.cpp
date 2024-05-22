#include "Client.h"
#include <fstream>
std::string nickname = "user";
std::string login = "";
std::string password = "";
SOCKET Connection = INVALID_SOCKET;

//для сохранения никнейма
void saveNicknameToFile(const std::string& nickname) {
    std::ofstream file("nickname.txt");
    if (file.is_open()) {
        file << nickname;
        file.close();
    }
    else {
        setColor("red");
        printf("Error: Unable to save nickname to file.\n");
        setColor("white");
    }
}
//отображение никнейма
std::string loadNicknameFromFile() {
    std::ifstream file("nickname.txt");
    std::string nickname;
    if (file.is_open()) {
        std::getline(file, nickname);
        file.close();
    }
    return nickname;
}

void setColor(const char* color) {
    if (strcmp(color, "red") == 0)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
    else if (strcmp(color, "white") == 0)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    else if (strcmp(color, "blue") == 0)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    else if (strcmp(color, "green") == 0)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    return;
}

std::vector<std::string> stringSplit(std::string str, std::string delimiter) {
    size_t posStart = 0;
    size_t posEnd;
    std::vector<std::string> splitted;

    while ((posEnd = str.find(delimiter, posStart)) != std::string::npos) {
        std::string token = str.substr(posStart, posEnd - posStart);
        if (!token.empty()) // Ignoring empty substrings
            splitted.push_back(token);
        posStart = posEnd + delimiter.length();
    }
    std::string lastToken = str.substr(posStart);
    if (!lastToken.empty()) // Ignoring the last empty substring
        splitted.push_back(lastToken);

    return splitted;
}


std::string getCurrentTime() {
    std::time_t msgTime = std::time(0);
    char timeStr[50];
    std::strftime(timeStr, sizeof(timeStr), "%F, %T", std::localtime(&msgTime));

    return std::string(timeStr);
}

void addTimeNickname(std::string* word, std::string* message) {
    if (*word != "!exit" && *word != "!show" && *word != "!create" && *word != "!remove" && *word != "!open") {
        std::string time = getCurrentTime();
        printf("%s\n\n", time.c_str());
        *message += "\n" + time;
        *message += ", " + nickname;
        // Save the nickname to a file immediately after changing it
        saveNicknameToFile(nickname);
    }
    return;
}


bool connectToServer(std::string address, int port) {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        printf("WSAStartup error\n");
        return false;
    }

    SOCKADDR_IN addr;
    int sizeofAddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&addr, sizeofAddr) != 0) {
        printf("Connection error\n");
        return false;
    }
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)clientHandler, NULL, NULL, NULL);
    return true;
}

void sendPacket(SOCKET* socket, Packet* packetType, std::string* message) {
    int messageSize = (*message).size();
    send(*socket, (char*)&(*packetType), sizeof(Packet), NULL);
    send(*socket, (char*)&messageSize, sizeof(int), NULL);
    send(*socket, (*message).c_str(), messageSize, NULL);

    return;
}

bool processInstructions(std::vector<std::string> splittedMessage) {
    if (splittedMessage[0] == "!help" && splittedMessage.size() == 1) {
        setColor("blue");
        printf("Use command '!nickname N' to set your nickname to N\n");
        printf("         or '!nickname' to see your current nickname\n");
        printf("Use command '!connect A P' to connect server with address A and port P\n");
        printf("Use command '!disconnect' to disconnect server\n");
        printf("Use command '!register L P' to register with login L and password P\n");
        printf("Use command '!login L P' to login with login L and password P\n");
        printf("Use command '!create R P' to create room R with password P\n");
        printf("Use command '!remove R P' to remove room R with password P\n");
        printf("Use command '!open R P' to open room R with password P\n");
        printf("Use command '!exit' to exit room\n");
        printf("Use command '!show' to show all rooms\n\n");
        setColor("white");
        return true;
    }
    // Check if the user is logged in before executing the !register and !login commands
    if (((splittedMessage[0] == "!register" && splittedMessage.size() != 3) || (splittedMessage[0] == "!login" && splittedMessage.size() != 3)) && (login.empty() || password.empty())) {
        setColor("red");
        printf("You need to log in first.\n\n");
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!register" && splittedMessage.size() == 3) {
        std::string message = splittedMessage[1] + " " + splittedMessage[2];
        Packet packetType = pRegister;
        nickname = splittedMessage[1];
        sendPacket(&Connection, &packetType, &message);
        return true;
    }

    if (splittedMessage[0] == "!login" && splittedMessage.size() == 3) {
        login = splittedMessage[1];
        password = splittedMessage[2];
        std::string message = login + " " + password;
        Packet packetType = pLogin;
        sendPacket(&Connection, &packetType, &message);
        return true;
    }

    // check for registration
    if (login.empty() || password.empty()) {
        if (splittedMessage[0] != "!help" && splittedMessage[0] != "!register" && splittedMessage[0] != "!login") {
            setColor("red");
            printf("You need to log in first.\n\n");
            setColor("white");
            return true;
        }
    }

    if (splittedMessage[0] == "!help" && splittedMessage.size() != 1) {
        setColor("red");
        printf("Wrong number of parameters for the command \'!help\'\n\n");
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!connect" && splittedMessage.size() == 3) {
        if (Connection == INVALID_SOCKET) {
            connectToServer(splittedMessage[1], atoi(splittedMessage[2].c_str()));
            setColor("blue");
            printf("You were connected to the server\n\n");
        }
        else {
            setColor("red");
            printf("You are already connected to the server\n\n");
        }
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!nickname" && splittedMessage.size() == 1) {
        setColor("blue");
        printf("Your current nickname is \'%s\'\n\n", nickname.c_str());
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!nickname" && splittedMessage.size() == 2) {
        setColor("blue");
        printf("You are changing nickname: \'%s\' -> \'%s\'\n\n", nickname.c_str(), splittedMessage[1].c_str());
        nickname = splittedMessage[1];
        // Save the nickname to a file after changing it
        saveNicknameToFile(nickname);
        setColor("white");
        return true;
    }


    if (splittedMessage[0] == "!nickname" && splittedMessage.size() > 2) {
        setColor("red");
        printf("Wrong number of parameters for the command \'!nickname\'\n");
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!connect" && splittedMessage.size() != 3) {
        setColor("red");
        printf("Wrong number of parameters for the command \'!connect\'\n\n");
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!disconnect" && splittedMessage.size() == 1) {
        setColor("red");
        if (Connection == INVALID_SOCKET)
            printf("You are already disconnected from any server\n\n");
        else {
            closesocket(Connection);
            Connection = INVALID_SOCKET;
            printf("You were disconnected from the server\n\n");
        }
        setColor("white");
        return true;
    }

    if (splittedMessage[0] == "!disconnect" && splittedMessage.size() != 1) {
        setColor("red");
        printf("Wrong number of parameters for the command \'!disconnect\'\n\n");
        setColor("white");
        return true;
    }

    return false;
}

bool processPacket(Packet packetType) {
    switch (packetType) {
    case pChatMessage:
    {
        setColor("white");
        break;
    }
    case pServerMessage:
    {
        setColor("blue");
        break;
    }
    case pWarningMessage:
    {
        setColor("red");
        break;
    }
    default:
    {
        setColor("red");
        printf("RECEIVED AN UNRECOGNIZED TYPE OF PACKET.\n\n");
        setColor("white");
        return false;
    }
    }
    int msgSize;
    recv(Connection, (char*)&msgSize, sizeof(int), NULL);
    char* msg = new char[msgSize + 1];
    msg[msgSize] = '\0';
    recv(Connection, msg, msgSize, NULL);
    printf("%s\n\n", msg);
    setColor("green");
    delete[] msg;

    return true;
}

void clientHandler() {
    Packet packetType;
    while (1) {
        if (recv(Connection, (char*)&packetType, sizeof(Packet), NULL) < 0) {
            setColor("red");
            printf("SERVER DISCONNECT\n\n");
            setColor("white");
            closesocket(Connection);
            Connection = INVALID_SOCKET;
            exit(0);
        }
        if (!processPacket(packetType))
            break;
    }
    closesocket(Connection);
    Connection = INVALID_SOCKET;
    return;
}

int main(int argc, char* argv[]) {
    std::string message;

    // Nickname loading from the file
    nickname = loadNicknameFromFile();

    if (!connectToServer("127.0.0.1", 1111))
        return 1;

    setColor("blue");
    printf("Use command \'!help\' to get more information\n\n");
    setColor("white");

    while (1) {
        setColor("green");

        getline(std::cin, message);
        std::vector<std::string> splittedMessage = stringSplit(message, " ");

        if (processInstructions(splittedMessage))
            continue;

        addTimeNickname(&splittedMessage[0], &message);

        // Saving a nickname to a file when it is changed
        saveNicknameToFile(nickname);

        Packet packetType = pChatMessage;
        sendPacket(&Connection, &packetType, &message);
        Sleep(10);
    }
    return 0;
}

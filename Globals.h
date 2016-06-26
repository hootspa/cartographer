#ifndef GLOBAL_ENUMS_H
#define GLOBAL_ENUMS_H

//TODO: there are 16 colors
enum COLOR { red, yellow, green, purple, oragne, brown, pink };

//3 different chat modes allowed for each client
enum CLIENT_CHAT_MODE {
	ALL_PLAYERS,
	PROXIMITY,
	OFF
};

#endif

#ifndef GLOBALS_H
#define GLOBALS_H
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "stdafx.h"
#include <unordered_map>
#include "h2mod.h"

#include "TSUsers.h"
#include "TSClient.h"
#include "TSServer.h"
#include "H2ChatBoxCommands.h"


extern TSUsers* tsUsers;

extern std::unordered_map<XUID, int> xuidPlayerIndexMap;
extern std::unordered_map<char*, XUID> nameToXuidIndexMap;
extern std::unordered_map<char*, int> nameToPlayerIndexMap;

extern TSClient* client;
extern bool stopClient;

extern TSServer* server;
extern IN_ADDR clientMachineAddress;
extern bool stopServer;
//xnetcreatekey sets this to true
//xsessiondelete/end set this to false
extern bool isServer;

extern bool microphoneEnabled;

extern bool isLobby;

extern CLIENT_CHAT_MODE clientChatMode;

extern ChatBoxCommands* commands;

extern HANDLE *currentSessionHandle;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
int stripWhitespace(wchar_t *inputStr);
#endif
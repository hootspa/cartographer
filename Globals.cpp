#include "Globals.h"

TSUsers* tsUsers = new TSUsers(); 
std::unordered_map<XUID, int> xuidPlayerIndexMap;
std::unordered_map<char*, XUID> nameToXuidIndexMap;
std::unordered_map<char*, int> nameToPlayerIndexMap;
TSClient* client = NULL;
TSServer* server = NULL;
//TODO: only really works once the teamspeak client has opened the microphone up and validated it could, till then we just assume the microphone is enabled
//we need something a bit nicer for this
bool microphoneEnabled = true;
bool stopClient;
bool stopServer;
bool clientServerAddressSet;
//always assume client is server unless they join a game which is called before xsessioncreate
//which creates or connects to teh team speak server
bool isServer = true;
bool isLobby = true;

HANDLE *currentSessionHandle;

CHAT_MODE chatMode = ALL_PLAYERS;

ChatBoxCommands* commands = new ChatBoxCommands();

IN_ADDR clientServerAddress = {};
IN_ADDR clientMachineAddress = {};

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}
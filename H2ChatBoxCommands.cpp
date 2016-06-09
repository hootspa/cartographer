#include "Globals.h"

ChatBoxCommands::ChatBoxCommands() {}

void ChatBoxCommands::mute(char* name, bool ban) {
	//TODO: if server try to mute on the server (if we even can)
	//if we can't we can just move them to another channel or kick them from the channel
	//but kicking would require us to every minute or so check if we reconnect
	if (!isServer) {
		client->mute(name, ban);
	}
}

void ChatBoxCommands::unmute(char* name) {
	//TODO: unmute for server (if possible)
	if (!isServer) {
		client->unmute(name);
		//TODO: remove ban if exists
	}
}

void ChatBoxCommands::setChatMode(CHAT_MODE mode) {
	if (!isServer && mode == OFF) {
		//client asking to turn off voice completely
		chatMode = OFF;
		cleanupClientAndServer();
		return;
	}

	//the server is trying to change the chat mode
	chatMode = mode;
	//TODO: right now just changing the chat mode is as simple as changing this global variable
	//however once channels are introduced for safe-team-chat, we will need to add logic that
	//tears down the channels if we switch from team chat to all_players or proximity
	//TODO: need some sort of templated visitor here for enum
	//to operate on the old type and the new type
}

void ChatBoxCommands::kick(char* playerName, bool perm) {
	int playerIndex;
	if (isLobby) {
		//TODO: the nameToPlayerIndexMap is based on in game player indexes
		//it is not used while in game, so the indexes aren't right
		TRACE_GAME_N("Kicking from lobby with player name won't work yet..");
	}	else {
		playerIndex = nameToPlayerIndexMap[playerName];
		for (auto it = nameToPlayerIndexMap.begin(); it != nameToPlayerIndexMap.end(); ++it) {
			char* name = it->first;
			int index = it->second;
			
			TRACE_GAME_N("PlayerMap-PlayerName=%s, PlayerIndex=%d", name, index);
		}
		this->kick(nameToPlayerIndexMap[playerName], perm);
	}
}

void ChatBoxCommands::kick(int playerIndex, bool perm) {
	h2mod->kick_player(playerIndex);
	//TODO: write to file for permnanent bans
}

bool ChatBoxCommands::isNum(char *s) {
	int i = 0;
	while (s[i]) {
		//if there is a letter in a string then string is not a number
		if (isalpha(s[i])) {
			return false;
		}
		i++;
	}
	return true;
}

BOOL ChatBoxCommands::handle_command(std::string command) {
	//split by a space
	std::vector<std::string> splitCommands = split(command, ' ');
	if (splitCommands.size() != 0) {
		std::string firstCommand = splitCommands[0];
		std::transform(firstCommand.begin(), firstCommand.end(), firstCommand.begin(), ::tolower);
		if (firstCommand == "$" + KICK_STR) {
			if (splitCommands.size() != 3) {
				TRACE_GAME_N(MISSING_GAMERTAG_AND_BAN_MSG, KICK_STR.c_str(), KICK_STR.c_str());
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;

			if (isNum(cstr)) {
				kick(atoi(cstr), ban);
			}	else {
				kick(cstr, ban);
			}

			delete[] cstr;
		}	else if (firstCommand == "$ban") {
		}	else if (firstCommand == "$" + MUTE_STR) {
			if (splitCommands.size() != 3) {
				TRACE_GAME_N(MISSING_GAMERTAG_AND_BAN_MSG, MUTE_STR.c_str(), MUTE_STR.c_str());
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;

			mute(cstr, ban);
		}	else if (firstCommand == "$" + UNMUTE_STR) {
			if (splitCommands.size() != 3) {
				TRACE_GAME_N(MISSING_GAMERTAG_MSG, UNMUTE_STR, UNMUTE_STR);
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			unmute(cstr);
		}	else if (firstCommand == "$setChatMode") {
		}	else if (firstCommand == "$listPlayers") {
		}	else if (firstCommand == "$listBannedPlayers") {
		}
	}

	return true;
}
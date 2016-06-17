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
		h2mod->write_chat_literal(L"Kicking from lobby with player name won't work yet..");
	}
	else {
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
	if (isLobby) {
		//TODO: the nameToPlayerIndexMap is based on in game player indexes
		//it is not used while in game, so the indexes aren't right
		h2mod->write_chat_literal(L"Kicking from lobby with player index won't work yet..");
		return;
	}
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

void ChatBoxCommands::listBannedPlayers() {
	if (!isServer) {
		//TODO:
	}
	else {
		TRACE_GAME_N(LIST_BANNED_PLAYERS_VALIDATION_MSG);
	}
}

void ChatBoxCommands::listPlayers() {
	for (int i = 0; i < 16; i++) {
		char gamertag[32];
		h2mod->get_player_name2(i, gamertag, 32);
		if ((gamertag != NULL) && (gamertag[0] >= 0x20 && gamertag[0] <= 0x7E)) {
			wchar_t* unicodeGamertag = new wchar_t[64];
			mbstowcs(unicodeGamertag, gamertag, 64);
			//stripWhitespace(unicodeGamertag);

			std::wstringstream oss;
			oss << "Player " << i << ":" << unicodeGamertag << "/" << "";
			std::wstring playerLogLine = oss.str();
			h2mod->write_chat_dynamic(playerLogLine.c_str());
		}
	}
}

void ChatBoxCommands::ban(char* gamertag) {
	//TODO:
}

/*
* Handles the given string command
* Returns a bool indicating whether the command is a valid command or not
*/
BOOL ChatBoxCommands::handle_command(std::string command) {
	//split by a space
	std::vector<std::string> splitCommands = split(command, ' ');
	if (splitCommands.size() != 0) {
		std::string firstCommand = splitCommands[0];
		std::transform(firstCommand.begin(), firstCommand.end(), firstCommand.begin(), ::tolower);
		if (firstCommand == "$kick") {
			if (splitCommands.size() != 3) {
				TRACE_GAME_N(KICK_VALIDATION_MSG);
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;

			if (isNum(cstr)) {
				kick(atoi(cstr), ban);
			}
			else {
				kick(cstr, ban);
			}

			delete[] cstr;
		}
		else if (firstCommand == "$ban") {
			if (splitCommands.size() != 2) {
				TRACE_GAME_N(BAN_VALIDATION_MSG);
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			ban(cstr);
		}
		else if (firstCommand == "$mute") {
			if (splitCommands.size() != 3) {
				TRACE_GAME_N(MUTE_VALIDATION_MSG);
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;

			mute(cstr, ban);

			delete[] cstr;
		}
		else if (firstCommand == "$unmute") {
			if (splitCommands.size() != 2) {
				TRACE_GAME_N(UNMUTE_VALIDATION_MSG);
				return false;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			unmute(cstr);

			delete[] cstr;
		}
		else if (firstCommand == "$setchatmode") {
			//TODO:
		}
		else if (firstCommand == "$listplayers") {
			if (splitCommands.size() != 1) {
				TRACE_GAME_N(LIST_PLAYERS_VALIDATION_MSG);
				return false;
			}
			listPlayers();
		}
		else if (firstCommand == "$listbannedplayers") {
			listBannedPlayers();
		}

		char c = firstCommand.c_str()[0];
		if (c == '$') {
			//if we made it here, we passed any/all validation and executed the command
			return true;
		}
	}

	return false;
}
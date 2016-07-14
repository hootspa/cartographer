#include "Globals.h"

ChatBoxCommands::ChatBoxCommands() {
}

void ChatBoxCommands::mute(char* name, bool ban) {
	//TODO: if server try to mute on the server (if we even can)
	//if we can't we can just move them to another channel or kick them from the channel
	//but kicking would require us to every minute or so check if we reconnect
	if (!isServer) {
		client->mute(name, ban);
	}
}


void ChatBoxCommands::setVoiceActivationLevel(float activationLevel) {
	if (!isServer) {
		client->setVoiceActivationLevel(activationLevel);
	}
}void ChatBoxCommands::unmute(char* name) {
	//TODO: unmute for server (if possible)
	if (!isServer) {
		client->unmute(name);
		//TODO: remove ban if exists
	}
}

void ChatBoxCommands::setChatMode(CLIENT_CHAT_MODE mode) {
	clientChatMode = mode;
}

void ChatBoxCommands::kick(char* playerName, bool perm) {
	int playerIndex = nameToPlayerIndexMap[playerName];
	for (auto it = nameToPlayerIndexMap.begin(); it != nameToPlayerIndexMap.end(); ++it) {
		char* name = it->first;
		int index = it->second;

		TRACE_GAME_N("PlayerMap-PlayerName=%s, PlayerIndex=%d", name, index);
	}
	this->kick(nameToPlayerIndexMap[playerName], perm);
}

void ChatBoxCommands::kick(int playerIndex, bool perm) {
	h2mod->kick_player(playerIndex);
	//TODO: write to file for permnanent bans
}

bool ChatBoxCommands::isNum(const char *s) {
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


/*
void ChatBoxCommands::listBannedXuids() {
	//TODo: go to ban file
}

void ChatBoxCommands::listBannedIps() {
	//TODo: go to ban file
}
*/
void ChatBoxCommands::listBannedPlayers() {
	//TODo: go to ban file
}


void ChatBoxCommands::printDistance(int player1, int player2) {
	float distance = h2mod->get_distance(player1, player2);
	std::wstringstream oss;
	oss << "Distance from player index " << player1 << " to player index " << player2 << " is " << distance << " units";
	h2mod->write_inner_chat_dynamic(oss.str().c_str());
}void ChatBoxCommands::listPlayers() {
	//TODO: we have to iterate all 16 player spots, since when people leave a game, other people in game don't occupy their spot
	for (int i = 0; i < 15; i++) {
		char gamertag[32];
		if (isLobby) {
			h2mod->get_player_name2(i, gamertag, 32);
		}
		else {
			h2mod->get_player_name(i, gamertag, 32);
		}
		if ((gamertag != NULL) && (gamertag[0] >= 0x20 && gamertag[0] <= 0x7E)) {
			wchar_t* unicodeGamertag = new wchar_t[64];
			mbstowcs(unicodeGamertag, gamertag, 64);

			const char* hostnameOrIP = inet_ntoa(h2mod->get_player_ip(i));

			bool resetDynamicBase = false;
			if (i != nameToPlayerIndexMap[gamertag]) {
				resetDynamicBase = true;
			}
			float xPos = h2mod->get_player_x(i, resetDynamicBase);
			float yPos = h2mod->get_player_y(i, resetDynamicBase);
			float zPos = h2mod->get_player_z(i, resetDynamicBase);

			std::wstringstream oss;
			oss << "Player " << i << ":" << unicodeGamertag << "/" << hostnameOrIP << "/x=" << xPos << ",y=" << yPos << ",z=" << zPos;
			std::wstring playerLogLine = oss.str();
			h2mod->write_inner_chat_dynamic(playerLogLine.c_str());
		}
	}
}

void ChatBoxCommands::ban(char* gamertag) {
	//first kick the player from the game
	kick(gamertag, true);

	int playerIndex = nameToPlayerIndexMap[gamertag];
	IN_ADDR playerIp = h2mod->get_player_ip(playerIndex);
	XUID playerXuid = nameToXuidIndexMap[gamertag];

	//then ban the player based on all the unique properties of the player
	BanUtility::banPlayer(gamertag, playerIp, playerXuid);}

/*
* Handles the given string command
* Returns a bool indicating whether the command is a valid command or not
*/
void ChatBoxCommands::handle_command(std::string command) {
	//split by a space
	std::vector<std::string> splitCommands = split(command, ' ');
	if (splitCommands.size() != 0) {
		std::string firstCommand = splitCommands[0];
		std::transform(firstCommand.begin(), firstCommand.end(), firstCommand.begin(), ::tolower);
		if (firstCommand == "$kick") {
			if (splitCommands.size() != 3) {
				h2mod->write_inner_chat_dynamic(L"Invalid kick command, usage - $kick (GAMERTAG or PLAYER_INDEX) BAN_FLAG(true/false)");
				return;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;

			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"Only the server can kick players");
				return;
			}

			if (isLobby) {
				//TODO: the nameToPlayerIndexMap is based on in game player indexes
				//it is not used while in game, so the indexes aren't right
				h2mod->write_inner_chat_dynamic(L"Kicking from lobby with player index or name won't work yet..");
				return;
			}

			if (isNum(cstr)) {
				kick(atoi(cstr), ban);
			}
			else {
				kick(cstr, ban);
			}

			delete[] cstr;
		} else if (firstCommand == "$ban") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid ban command, usage - $ban GAMERTAG");
				return;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"Only the server can ban players");
				return;
			}

			if (isLobby) {
				//TODO: the nameToPlayerIndexMap is based on in game player indexes
				//it is not used while in game, so the indexes aren't right
				h2mod->write_inner_chat_dynamic(L"Banning from lobby with player index or name won't work yet..");
				return;
			}
			ban(cstr);
		}
		else if (firstCommand == "$mute") {
			if (splitCommands.size() != 3) {
				h2mod->write_inner_chat_dynamic(L"Invalid mute command, usage - $mute GAMERTAG BAN_FLAG(true/false)");
				return;
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
				h2mod->write_inner_chat_dynamic(L"Invalid mute command, usage - $unmute GAMERTAG");
				return;
			}
			std::string firstArg = splitCommands[1];
			char *cstr = new char[firstArg.length() + 1];
			strcpy(cstr, firstArg.c_str());

			unmute(cstr);

			delete[] cstr;
		}
		else if (firstCommand == "$setchatmode") {
			//TODO: use command socket to set chat modes for client
		}
		else if (firstCommand == "$listplayers") {
			if (splitCommands.size() != 1) {
				h2mod->write_inner_chat_dynamic(L"Invalid listPlayers command, usage - $listPlayers");
				return;
			}
			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"listPlayers can only be used on the server");
				return;
			}
			listPlayers();
		}
		else if (firstCommand == "$listbannedplayers") {
			if (splitCommands.size() != 1) {
				h2mod->write_inner_chat_dynamic(L"Invalid listBannedPlayers command, usage - $listBannedPlayers");
				return;
			}
			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"listBannedPlayers can only be used on the server");
				return;
			}
			listBannedPlayers();
		} else if (firstCommand == "$printdistance") {
			if (splitCommands.size() != 3) {
				h2mod->write_inner_chat_dynamic(L"Invalid $printDistance command, usage - $printDistance playerIndex1 playerIndex2");
				return;
			}

			/*Don't commit this, turn back on later
			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"printDistance can only be used on the server");
				return;
			}*/

			std::string firstArg = splitCommands[1];
			if (!isNum(firstArg.c_str())) {
				h2mod->write_inner_chat_dynamic(L"playerIndex1 is not a number");
				return;
			}
			int player1 = atoi(firstArg.c_str());
			std::string secondArg = splitCommands[2];
			if (!isNum(secondArg.c_str())) {
				h2mod->write_inner_chat_dynamic(L"playerIndex2 is not a number");
				return;
			}
			int player2 = atoi(secondArg.c_str());
			printDistance(player1, player2);
		}	else if (firstCommand == "$setvoiceactivationlevel") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid $setVoiceActivationLevel command, usage - $setVoiceActivationLevel activationLevel(-50 to 50)");
				return;
			}

			std::string firstArg = splitCommands[1];
			if (!isNum(firstArg.c_str())) {
				h2mod->write_inner_chat_dynamic(L"activation level is not a number");
				return;
			}
			float activationLevel = ::atof(firstArg.c_str());
			if (activationLevel < MIN_VOICE_ACTIVATION_LEVEL || activationLevel > MAX_VOICE_ACTIVATION_LEVEL) {
				h2mod->write_inner_chat_dynamic(L"activation level is not between -50 and 50, default is -45");
			}

			setVoiceActivationLevel(activationLevel);
		}
	}
}

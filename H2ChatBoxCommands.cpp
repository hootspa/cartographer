#include "Globals.h"
#include <Shlwapi.h>

ChatBoxCommands::ChatBoxCommands() {
}

void ChatBoxCommands::mute(const char* name, bool ban) {
	//TODO: if server try to mute on the server (if we even can)
	//if we can't we can just move them to another channel or kick them from the channel
	//but kicking would require us to every minute or so check if we reconnect
	if (client != NULL) {
		client->mute(name, ban);
	}
}

void ChatBoxCommands::setVoiceActivationLevel(float activationLevel) {
	if (client != NULL) {
		client->setVoiceActivationLevel(activationLevel);
	}
}

void ChatBoxCommands::unmute(const char* name) {
	//TODO: unmute for server (if possible)
	if (client != NULL) {
		client->unmute(name);
		//TODO: remove ban if exists
	}
}

void ChatBoxCommands::setChatMode(CLIENT_CHAT_MODE mode) {
	clientChatMode = mode;
}

void ChatBoxCommands::kick(const char* playerName) {
	H2Player& player = players->getPlayer(playerName);
	int peerIndex = player.peer;

	if (peerIndex == -1) {
		h2mod->write_inner_chat_dynamic(L"Could not find player index");
		return;
	}

	TRACE_GAME_N("About to kick player index %d for player name %s", peerIndex, playerName);
	h2mod->kick_player(peerIndex);
}

void ChatBoxCommands::kick(int playerIndex) {
	h2mod->kick_player(playerIndex);
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

void ChatBoxCommands::listBannedPlayers() {
	std::vector<std::string> stringList;

	BanUtility::getInstance().fillBannedPlayerDisplayStrList(stringList);

	std::vector<std::string>::const_iterator iterator;
	for (iterator = stringList.begin(); iterator != stringList.end(); ++iterator) {
		std::string displayItem = *iterator;
		std::wstring ws;
		ws.assign(displayItem.begin(), displayItem.end());
		h2mod->write_inner_chat_dynamic(ws.c_str());
	}
}

void ChatBoxCommands::printDistance(int player1, int player2) {
	float distance = h2mod->get_distance(player1, player2);
	std::wstringstream oss;
	oss << "Distance from player index " << player1 << " to player index " << player2 << " is " << distance << " units";
	h2mod->write_inner_chat_dynamic(oss.str().c_str());
}

void ChatBoxCommands::listPlayers() {
	TRACE("Team play on: %d", h2mod->is_team_play());
	players->print();
}

void ChatBoxCommands::ban(const char* gamertag) {
	//first get all the unique properties about the player
	H2Player& player = players->getPlayer(gamertag);
	int playerIndex = player.index;
	XUID playerXuid = player.index;
	IN_ADDR playerIp = h2mod->get_player_ip(playerIndex);

	//second kick the player from the game
	kick(gamertag);

	//finally ban the player based on all the unique properties of the player
	BanUtility::getInstance().banPlayer(gamertag, playerIp, playerXuid);
}

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
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid kick command, usage - $kick (GAMERTAG or PLAYER_INDEX)");
				return;
			}
			std::string firstArg = splitCommands[1];
			try {
				if (!isServer) {
					//if clients use kick function, they will be kicked automatically by the game
					h2mod->write_inner_chat_dynamic(L"Only the server can kick players");
				}	else {
					if (isNum(firstArg.c_str())) {
						kick(atoi(firstArg.c_str()));
					}	else {
						kick(firstArg.c_str());
					}
				}
			}
			catch (...) {
				TRACE("Error trying to kick");
			}
		}
		else if (firstCommand == "$ban") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid ban command, usage - $ban GAMERTAG");
				return;
			}

			//TODO: enable when ban is ready
			h2mod->write_inner_chat_dynamic(L"not ready yet");
			return;

			std::string firstArg = splitCommands[1];

			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"Only the server can ban players");
			}
			else {
				ban(firstArg.c_str());
			}
		}
		else if (firstCommand == "$mute") {
			if (splitCommands.size() < 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid mute command, usage - $mute GAMERTAG");
				return;
			}
			std::string firstArg = splitCommands[1];

			/*
			std::string secondArg = splitCommands[2];
			bool ban = secondArg == "true" ? true : false;*/

			try {
				mute(firstArg.c_str(), false);
			}
			catch (...) {
				TRACE("Error trying to mute");
			}
		}
		else if (firstCommand == "$unmute") {
			if (splitCommands.size() < 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid mute command, usage - $unmute GAMERTAG");
				return;
			}
			std::string firstArg = splitCommands[1];
			try {
				unmute(firstArg.c_str());
			}	catch (...) {
				TRACE("Error trying to unmute");
			}
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

			//TODO: enable when ban is ready
			h2mod->write_inner_chat_dynamic(L"not ready yet");
			return;

			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"listBannedPlayers can only be used on the server");
				return;
			}
			listBannedPlayers();
		}
		else if (firstCommand == "$printdistance") {
			if (splitCommands.size() != 3) {
				h2mod->write_inner_chat_dynamic(L"Invalid $printDistance command, usage - $printDistance playerIndex1 playerIndex2");
				return;
			}

			//TODO: enable when ready
			h2mod->write_inner_chat_dynamic(L"not ready yet");
			return;

			if (!isServer) {
				h2mod->write_inner_chat_dynamic(L"printDistance can only be used on the server");
				return;
			}

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
		}
		else if (firstCommand == "$setvoiceactivationlevel") {
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
				h2mod->write_inner_chat_dynamic(L"activation level is not between -50 and 50, default is -25.0f");
			}

			setVoiceActivationLevel(activationLevel);
		}
		else if (firstCommand == "$unban") {
			//TODO: unbanning would involve being able to unban by name, or address, or xuid
			//TODO: check for unban all, and unban everyone

			//TODO: enable when ban is ready
			h2mod->write_inner_chat_dynamic(L"not ready yet");
			return;
		}
		else if (firstCommand == "$getplayerindex") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid $getplayerindex command, usage - $getplayerindex player_name");
				return;
			}
			std::string firstArg = splitCommands[1];
			H2Player& player = players->getPlayer(firstArg.c_str());

			std::wstringstream oss;
			oss << "Player " << firstArg.c_str() << " index is = " << player.index;
			h2mod->write_inner_chat_dynamic(oss.str().c_str());
		}
		else if (firstCommand == "$getpeerindex") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid $getpeerindex command, usage - $getplayerindex player_name");
				return;
			}
			std::string firstArg = splitCommands[1];
			H2Player& player = players->getPlayer(firstArg.c_str());

			std::wstringstream oss;
			oss << "Peer " << firstArg.c_str() << " index is = " << player.peer;
			h2mod->write_inner_chat_dynamic(oss.str().c_str());
		}
		else if (firstCommand == "$muteall") {
			if (splitCommands.size() != 1) {
				h2mod->write_inner_chat_dynamic(L"Invalid command, usage - $muteAll");
				return;
			}

			for (int i = 0; i < 15; i++) {
				H2Player& player = players->getPlayer(i);

				if (player.index != -1) {
					//TODO: fix
					//mute(player.name.c_str(), true);
				}
			}
		}
		else if (firstCommand == "$unmuteall") {
			if (splitCommands.size() != 1) {
				h2mod->write_inner_chat_dynamic(L"Invalid command, usage - $unmuteAll");
				return;
			}

			for (int i = 0; i < 15; i++) {
				H2Player& player = players->getPlayer(i);

				if (player.index != -1) {
					//TODO: fix
					//unmute(player.name.c_str());
				}
			}
		}
		else if (firstCommand == "$reloadmaps") {
			if (splitCommands.size() != 1) {
				h2mod->write_inner_chat_dynamic(L"Invalid command, usage - $reloadMaps");
				return;
			}

			mapManager->reloadMaps();
		}
		else if (firstCommand == "$printmemory") {

			//InstancesCount<H2Player>::print();
		}
		else if (firstCommand == "$spawn") {
			if (splitCommands.size() != 2) {
				h2mod->write_inner_chat_dynamic(L"Invalid command, usage $spawn ");
				return;
			}

			std::string firstArg = splitCommands[1];

			printf("string object_datum = %s", firstArg.c_str());

			unsigned int object_datum = strtoul(firstArg.c_str(), NULL, 0);
			printf("object_datum: %08X\n", object_datum);
			TRACE_GAME("object_datum = %08X", object_datum);

			char* nObject = new char[0xC4];
			DWORD dwBack;
			VirtualProtect(nObject, 0xC4, PAGE_EXECUTE_READWRITE, &dwBack);

			if (object_datum)
			{
				unsigned int player_datum = h2mod->get_unit_datum_from_player_index(0);
				call_object_placement_data_new(nObject, object_datum, player_datum, 0);

				*(float*)(nObject + 0x1C) = h2mod->get_player_x(0, true);
				*(float*)(nObject + 0x20) = h2mod->get_player_y(0, true);
				*(float*)(nObject + 0x24) = (h2mod->get_player_z(0, true) + 5.0f);

				unsigned int object_gamestate_datum = call_object_new(nObject);
				call_add_object_to_sync(object_gamestate_datum);


			}
		}
	}
}
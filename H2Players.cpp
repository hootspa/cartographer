#include "Globals.h"

void H2Players::initPeerHostData(wchar_t* gamertag, XUID id) {
	//we build this entire data structure based on the network updates sent back and forth
	//between clients/server, but when you are the peer host you actually don't send any network data
	//till someone joins so the events we populate data don't happen till someone joins. The adverse effect is this 
	//data structure won't be available immediately (so anything using this structure won't find anything), so we 
	//workaround it here by pushing the hosts player info into the structure manually
	updatePlayerId(0, id, 0);
	updatePlayerNameAndTeam(0, gamertag, h2mod->get_local_team_index());
}

H2Player& H2Players::getPlayer(std::string playerName) {
	std::wstring ww(playerName.begin(), playerName.end());
	return getPlayer(ww);
}

H2Player& H2Players::getPlayer(std::wstring playerName) {
	return *nameToPlayersMap[playerName];
}

H2Player& H2Players::getPlayer(int playerIndex) {
	return *indexToPlayersMap[playerIndex];
}

H2Player& H2Players::getPlayer(XUID playerXuid) {
	return *indexToPlayersMap[xuidToPlayerIndexMap[playerXuid]];
}

void H2Players::print() {
	for (auto it = indexToPlayersMap.begin(); it != indexToPlayersMap.end(); ++it) {
		H2Player* player = it->second;
		if (player != NULL) {

			const char* hostnameOrIP = inet_ntoa(h2mod->get_player_ip(player->index));
			/*
			TODO:
			bool resetDynamicBase = false;
			if (i != nameToPlayerIndexMap[gamertag]) {
			resetDynamicBase = true;
			}

			float xPos = h2mod->get_player_x(i, resetDynamicBase);
			float yPos = h2mod->get_player_y(i, resetDynamicBase);
			float zPos = h2mod->get_player_z(i, resetDynamicBase);*/

			std::wstringstream oss;
			oss << "Player Index:" << player->index 
					<< ",Peer Index: " << player->peer 
					<< ",Name: " << player->name.c_str() 
					<< ",Ip: " << hostnameOrIP;// << "/x=" << xPos << ",y=" << yPos << ",z=" << zPos;
			std::wstring playerLogLine = oss.str();
			h2mod->write_inner_chat_dynamic(playerLogLine.c_str());
			TRACE_GAME_N("%s", playerLogLine.c_str());
		}
	}
}

void H2Players::getPlayersForTeam(H2Player players[], int team) {
	//TODO:
}

void H2Players::updatePlayerId(int playerIndex, XUID playerXuid, int peerIndex) {
	H2Player* possiblePlayer = indexToPlayersMap[playerIndex];
	//-1 = empty struct in this case
	if (possiblePlayer != NULL && possiblePlayer->index != -1) {
		if (possiblePlayer->xuid != playerXuid) {
			TRACE_GAME("Player Membership Update - Player id's different for player index %d", playerIndex);
			possiblePlayer->xuid = playerXuid;
		}
	}	else {
		//new player
		H2Player* player = new H2Player();
		player->index = playerIndex;
		player->xuid = playerXuid;
		player->peer = peerIndex;

		indexToPlayersMap[playerIndex] = player;
		peersToPlayersMap[peerIndex] = player;
		xuidToPlayerIndexMap[playerXuid] = playerIndex;
	}
}

void H2Players::updatePlayerNameAndTeam(int playerIndex, wchar_t*& playerName, int playerTeam) {
	H2Player* possiblePlayer = indexToPlayersMap[playerIndex];

	//-1 = empty struct in this case
	if (possiblePlayer != NULL && possiblePlayer->index != -1) {
		//there is already a player cached for this index, check if anything has changed, and update accordingly
		if (wcscmp(possiblePlayer->name.c_str(), playerName) != 0) {
			TRACE_GAME("Player Membership Update - Player index %d name not the same oldName=%s,newName=%s", 
				playerIndex, possiblePlayer->name.c_str(), playerName);
			//set new name
			//old name goes out scope and the wstring should clean it up
			possiblePlayer->name = std::wstring(playerName);
			nameToPlayersMap[possiblePlayer->name] = possiblePlayer;
		}

		if (possiblePlayer->team != playerTeam) {
			TRACE_GAME_N("Player Membership Update - Player index %d team change oldTeam=%d,newTeam=%d",
				playerIndex, possiblePlayer->team, playerTeam);
			possiblePlayer->team = playerTeam;
			//TODO: fire off team change event
		}
	}	else {
		//new player
		H2Player* player = new H2Player();
		player->index = playerIndex;
		player->name = std::wstring(playerName);
		player->team = playerTeam;

		indexToPlayersMap[playerIndex] = player;
		nameToPlayersMap[player->name] = player;
	}
}

void H2Players::clear() {
	for (auto it = indexToPlayersMap.begin(); it != indexToPlayersMap.end(); ++it) {
		delete it->second;
	}
	xuidToPlayerIndexMap.clear();
	indexToPlayersMap.clear();
	nameToPlayersMap.clear();
	peersToPlayersMap.clear();
}

bool H2Players::playersEqual(H2Player player1, H2Player player2) {
	return player1.index == player2.index
		&& player1.name == player2.name
		&& player1.xuid == player2.xuid;
}

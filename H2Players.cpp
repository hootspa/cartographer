#include "Globals.h"

void H2Players::initPeerHostData(char* gamertag, XUID id) {
	//we build this entire data structure based on the network updates sent back and forth
	//between clients/server, but when you are the peer host you actually don't send any network data
	//till someone joins so the events we populate data don't happen till someone joins. The adverse effect is this 
	//data structure won't be available immediately (so anything using this structure won't find anything), so we 
	//workaround it here by pushing the hosts player info into the structure manually
	updatePlayerId(0, id, 0);
	updatePlayerNameAndTeam(0, gamertag, h2mod->get_local_team_index());
}

H2Player H2Players::getPlayer(std::string playerName) {
	return nameToPlayersMap[playerName];
}

H2Player H2Players::getPlayer(int playerIndex) {
	return indexToPlayersMap[playerIndex];
}

H2Player H2Players::getPlayer(XUID playerXuid) {
	return indexToPlayersMap[xuidToPlayerIndexMap[playerXuid]];
}

void H2Players::getPlayersForTeam(H2Player players[], int team) {
	//TODO:
}

void H2Players::updatePlayerId(int playerIndex, XUID playerXuid, int peerIndex) {
	H2Player possiblePlayer = indexToPlayersMap[playerIndex];
	//-1 = empty struct in this case
	if (possiblePlayer.index != -1) {
		if (possiblePlayer.xuid != playerXuid) {
			TRACE("Player Membership Update - Player id's different for player index %d", playerIndex);
			possiblePlayer.xuid = playerXuid;
		}
	}	else {
		//new player
		H2Player player = H2Player();
		player.index = playerIndex;
		player.xuid = playerXuid;
		player.peer = peerIndex;

		indexToPlayersMap[playerIndex] = player;
		peersToPlayersMap[peerIndex] = player;
		xuidToPlayerIndexMap[playerXuid] = playerIndex;
	}
}

void H2Players::updatePlayerNameAndTeam(int playerIndex, std::string playerName, int playerTeam) {
	H2Player possiblePlayer = indexToPlayersMap[playerIndex];

	//-1 = empty struct in this case
	if (possiblePlayer.index != -1) {
		//there is already a player cached for this index, check if anything has changed, and update accordingly
		if (possiblePlayer.name != playerName) {
			TRACE_GAME_N("Player Membership Update - Player index %d name not the same oldName=%s,newName=%s", 
				playerIndex, possiblePlayer.name.c_str(), playerName.c_str());
			//set new name
			possiblePlayer.name = playerName;
		}

		if (possiblePlayer.team != playerTeam) {
			TRACE_GAME_N("Player Membership Update - Player index %d team change oldTeam=%d,newTeam=%d",
				playerIndex, possiblePlayer.team, playerTeam);
			possiblePlayer.team = playerTeam;
			//TODO: fire off team change event
		}
	}	else {
		//new player
		H2Player player = H2Player();
		player.index = playerIndex;
		player.name = playerName;
		player.team = playerTeam;

		indexToPlayersMap[playerIndex] = player;
		nameToPlayersMap[playerName] = player;
	}
}

void H2Players::clear() {
	xuidToPlayerIndexMap.clear();
	indexToPlayersMap.clear();
	nameToPlayersMap.clear();
}

bool H2Players::playersEqual(H2Player player1, H2Player player2) {
	return player1.index == player2.index
		&& player1.name == player2.name
		&& player1.xuid == player2.xuid;
}

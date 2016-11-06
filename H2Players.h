#ifndef PLAYERS_H
#define PLAYERS_H

//TODO: add peer address
class H2Player {
public:
	std::wstring name;
	int index = -1;
	int peer;
	int team;
	XUID xuid;

	H2Player() {}
};

class H2Players {
public:
	H2Player& getPlayer(std::string playerName);
	H2Player& getPlayer(std::wstring playerName);
	H2Player& getPlayer(int playerIndex);
	H2Player& getPlayer(XUID playerXuid);
	void print();
	void getPlayersForTeam(H2Player players[], int team);
	void updatePlayerId(int playerIndex, XUID playerId, int peerIndex);
	void updatePlayerNameAndTeam(int playerIndex, wchar_t*& playerName, int playerTeam);
	void clear();
	void initPeerHostData(wchar_t* arr, XUID id);

private:
	bool playersEqual(H2Player player1, H2Player player2);

	std::unordered_map<XUID, int> xuidToPlayerIndexMap;
	std::unordered_map<int, H2Player*> indexToPlayersMap;
	std::unordered_map<int, H2Player*> peersToPlayersMap;
	std::unordered_map<std::wstring, H2Player*> nameToPlayersMap;
	std::unordered_map<int, H2Player*[]> teamToPlayersMap;
};

#endif
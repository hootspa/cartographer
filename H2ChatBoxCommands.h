#ifndef CHATBOX_COMMANDS_H
#define CHATBOX_COMMANDS_H

class ChatBoxCommands {
public:
	ChatBoxCommands();
	void listPlayers();
	void listBannedPlayers();
	void mute(char*, bool);
	void unmute(char*);
	void ban(char*);
	void kick(char*, bool);
	void kick(int, bool);
	void setChatMode(CHAT_MODE);
	BOOL handle_command(std::string);
private:
	bool ChatBoxCommands::isNum(char *s);
	std::string SET_CHAT_MOD_STR = "setChatMode";
	std::string LIST_BANNED_PLAYERS_STR = "listBannedPlayers";
	std::string LIST_PLAYERS_STR = "listPlayers";
	std::string UNMUTE_STR = "unmute";
	std::string MUTE_STR = "mute";
	std::string BAN_STR = "ban";
	std::string KICK_STR = "kick";
	char* MISSING_GAMERTAG_MSG = "No gamertag provided for %s or too many/not enough args provided, usage - $%s GAMERTAG";
	char* MISSING_GAMERTAG_AND_BAN_MSG = "No gamertag/ban flag provided for %s or too many/not enough args provided, usage - $%s GAMERTAG BAN(true/false)";
};

#endif
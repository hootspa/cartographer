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
	char* SET_CHAT_MODE_VALIDATION_MSG = "Invalid setChatMode command, can only be used on server, usage - $setChatMode 0-3 (0=everyone,1=proximity,2=teamonly,3=off)";
	char* SET_CHANNEL_CODEC_VALIDATION_MSG = "Invalid setChannelCodec command, can only be used on server, usage - $setChannelCodec 0-5 (5=best, 0=worst)";
	char* SET_CHANNEL_QUALITY_VALIDATION_MSG = "Invalid setChannelQuality command, can only be used on server, usage - $setChannelQuality 0-10 (10=best, 0=worst)";
	char* LIST_BANNED_PLAYERS_VALIDATION_MSG = "Invalid listBannedPlayers command, can only be used on server, usage - $listBannedPlayers";
	char* LIST_PLAYERS_VALIDATION_MSG = "Invalid listPlayers command, usage - $listPlayers";
	char* BAN_VALIDATION_MSG = "Invalid ban command, can only be used on server, usage - $ban GAMERTAG";
	char* MUTE_VALIDATION_MSG = "Invalid mute command, usage - $mute GAMERTAG BAN_FLAG(true/false)";
	char* UNMUTE_VALIDATION_MSG = "Invalid mute command, usage - $unmute GAMERTAG";
	char* KICK_VALIDATION_MSG = "Invalid kick command, can only be used on server, usage - $kick (GAMERTAG or PLAYER_INDEX) BAN_FLAG(true/false)";
};

#endif
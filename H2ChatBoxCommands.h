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
	void setChatMode(CLIENT_CHAT_MODE);
	BOOL handle_command(std::string);
private:
	bool ChatBoxCommands::isNum(char *s);

	char* SET_CHAT_MODE_VALIDATION_MSG = "Invalid setChatMode command, can only be used on server, usage - $setChatMode 0-3 (0=everyone,1=proximity,2=teamonly,3=off)";
	char* SET_CHANNEL_CODEC_VALIDATION_MSG = "Invalid setChannelCodec command, can only be used on server, usage - $setChannelCodec 0-5 (5=best, 0=worst)";
	char* SET_CHANNEL_QUALITY_VALIDATION_MSG = "Invalid setChannelQuality command, can only be used on server, usage - $setChannelQuality 0-10 (10=best, 0=worst)";
};

#endif
#include "public_definitions.h"
#include "public_errors.h"
#include "clientlib_publicdefinitions.h"
#include "clientlib.h"

#include "Globals.h"

const float TSClient::MAX_CLIENT_VOLUME_MODIFIER = 20.0f;
const float TSClient::MIN_CLIENT_VOLUME_MODIFIER = -60.0f;

TSClient::TSClient(bool log) {
	TRACE("Client started");
	initializeCallbacks(log);
	TRACE("Client lib initialized");
}

char* TSClient::programPath(char* programInvocation) {
	char* path;
	char* end;
	int length;
	char pathsep;

	if (programInvocation == NULL) return strdup("");

#ifdef _WIN32
	pathsep = '\\';
#else
	pathsep = '/';
#endif

	end = strrchr(programInvocation, pathsep);
	if (!end) return strdup("");

	length = (end - programInvocation) + 2;
	path = (char*)malloc(length);
	strncpy(path, programInvocation, length - 1);
	path[length - 1] = 0;

	return path;
}

void TSClient::initializeCallbacks(bool log) {
	char * path;
	struct ClientUIFunctions funcs;

	memset(&funcs, 0, sizeof(struct ClientUIFunctions));

	funcs.onConnectStatusChangeEvent = onConnectStatusChangeEvent;
	funcs.onClientMoveEvent = onClientMoveEvent;
	funcs.onClientMoveTimeoutEvent = onClientMoveTimeoutEvent;
	funcs.onTalkStatusChangeEvent = onTalkStatusChangeEvent;
	funcs.onServerErrorEvent = onServerErrorEvent;
	funcs.onUserLoggingMessageEvent = onUserLoggingMessageEvent;
	
	//soundbackends better be in your /halo2 directory
	path = programPath("soundbackends");
	int logTypes;

	if (log) {
		logTypes = LogType_FILE | LogType_CONSOLE | LogType_USERLOGGING;
	}	else {
		logTypes = LogType_NONE;
	}
	error = ts3client_initClientLib(&funcs, NULL, logTypes, NULL, path);
	free(path);

	if (error != ERROR_ok) {
		char* errormsg;
		if (ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
			TRACE_GAME_N("Error initialzing client: %s\n", errormsg);
			ts3client_freeMemory(errormsg);
		}
	}

	/* Create a new client identity */
	if ((error = ts3client_createIdentity(&identity)) != ERROR_ok) {
		TRACE("Error creating identity: %d\n", error);
		return;
	}
}

void TSClient::onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
	//TODO: what to do during critical errors? nothing?
	TRACE_GAME_N("[%d]User log message: %s", logLevel, logMessage);
}

void TSClient::connect() {
	/* Spawn a new server connection handler using the default port and store the server ID */
	if ((error = ts3client_spawnNewServerConnectionHandler(0, &scHandlerID)) != ERROR_ok) {
		TRACE("Error spawning server connection handler: %d\n", error);
	}

	this->openMicrophone();
	this->openPlayback();

	char *version;
	const char* hostnameOrIP = inet_ntoa(serverAddress);
	TRACE_GAME_N("TeamSpeak::hostname: %s", hostnameOrIP);
	TRACE("TeamSpeak::port: %d", port);
	TRACE_GAME_N("TeamSpeak::nickname: %s", nickname);
	//TODO: do we need a secret per session?
	if ((error = ts3client_startConnection(scHandlerID, identity, hostnameOrIP, port, nickname, NULL, "", "secret")) != ERROR_ok) {
		TRACE("Error connecting to server: %d\n", error);
		return;
	}

	/* Query and print client lib version */
	if ((error = ts3client_getClientLibVersion(&version)) != ERROR_ok) {
		TRACE("Failed to get clientlib version: %d\n", error);
		return;
	}
	TRACE_GAME_N("Client lib version: %s", version);
	ts3client_freeMemory(version);  /* Release dynamically allocated memory */
	version = NULL;
}

void TSClient::openPlayback() {
	/* Open default playback device */
	/* Passing empty string for mode and NULL or empty string for device will open the default device */
	if ((error = ts3client_openPlaybackDevice(scHandlerID, "", NULL)) != ERROR_ok) {
		TRACE("Error opening playback device: %d\n", error);
	}
	if ((error = ts3client_setPlaybackConfigValue(scHandlerID, "volume_modifier", "20")) != ERROR_ok) {
		TRACE("Error setting playback config value: %d\n", error);
	}
}

void TSClient::openMicrophone() {
	/* Open default capture device */
	/* Passing empty string for mode and NULL or empty string for device will open the default device */
	if ((error = ts3client_openCaptureDevice(scHandlerID, "", NULL)) != ERROR_ok) {
		microphoneEnabled = false;
		TRACE("Error opening capture device: %d\n", error);
	}
}

void TSClient::disconnect() {
	/* Disconnect from server */
	if ((error = ts3client_stopConnection(scHandlerID, "leaving")) != ERROR_ok) {
		TRACE("Error stopping connection: %d\n", error);
	}	else {
		TRACE_GAME_N("Client disconnecting from the server %s", inet_ntoa(serverAddress));
	}

	//TODO: guess we need to wait a lil fro the connection to stop, why?
	Sleep(200);

	/* Destroy server connection handler */
	if ((error = ts3client_destroyServerConnectionHandler(scHandlerID)) != ERROR_ok) {
		TRACE("Error destroying clientlib: %d\n", error);
	}
}

/*
* Callback for connection status change.
* Connection status switches through the states STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED and STATUS_CONNECTION_ESTABLISHED.
*
* Parameters:
*   serverConnectionHandlerID - Server connection handler ID
*   newStatus                 - New connection status, see the enum ConnectStatus in clientlib_publicdefinitions.h
*   errorNumber               - Error code. Should be zero when connecting or actively disconnection.
*                               Contains error state when losing connection.
*/
void TSClient::onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
	TRACE("Connect status changed: %llu %d %u", (unsigned long long)serverConnectionHandlerID, newStatus, errorNumber);
	/* Failed to connect ? */
	if (newStatus == STATUS_DISCONNECTED && errorNumber == ERROR_failed_connection_initialisation) {
		TRACE_GAME_N("Looks like there is no server running.");
	}
}

/*
* Called when a client joins, leaves or moves to another channel.
*
* Parameters:
*   serverConnectionHandlerID - Server connection handler ID
*   clientID                  - ID of the moved client
*   oldChannelID              - ID of the old channel left by the client
*   newChannelID              - ID of the new channel joined by the client
*   visibility                - Visibility of the moved client. See the enum Visibility in clientlib_publicdefinitions.h
*                               Values: ENTER_VISIBILITY, RETAIN_VISIBILITY, LEAVE_VISIBILITY
*/
void TSClient::onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
	TRACE_GAME_N("ClientID %u moves from channel %llu to %llu with message %s", clientID, (unsigned long long)oldChannelID, (unsigned long long)newChannelID, moveMessage);
}

/*
* Called when a client drops his connection.
*
* Parameters:
*   serverConnectionHandlerID - Server connection handler ID
*   clientID                  - ID of the moved client
*   oldChannelID              - ID of the channel the leaving client was previously member of
*   newChannelID              - 0, as client is leaving
*   visibility                - Always LEAVE_VISIBILITY
*   timeoutMessage            - Optional message giving the reason for the timeout
*/
void TSClient::onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
	TRACE_GAME_N("ClientID %u timeouts with message %s", clientID, timeoutMessage);
}

/* 
 * Prints the current client volume 
 */
void TSClient::printCurrentClientVolume(anyID teamspeakClientID) {
	int volume;
	if (ts3client_getClientVariableAsInt(scHandlerID, teamspeakClientID, CLIENT_VOLUME_MODIFICATOR, &volume)) {
		TRACE("error getting client volume");
	}
	else {
		TRACE("current client volume=%d", volume);
	}
}

int TSClient::getClientVolume(anyID teamspeakClientID) {
	int volume;
	if (ts3client_getClientVariableAsInt(scHandlerID, teamspeakClientID, CLIENT_VOLUME_MODIFICATOR, &volume)) {
		TRACE("error getting client volume");
	}
	else {
		TRACE("current client volume=%d", volume);
	}
	return volume;
}

/*
 * Sets the volume for the given client id
 */
void TSClient::setClientVolume(anyID teamspeakClientID, float volume) {
	int error;
	if ((error = ts3client_setClientVolumeModifier(scHandlerID, teamspeakClientID, volume)) != ERROR_ok) {
		TRACE("error turning off client volume: %d\n", error);
	}
}

anyID TSClient::getClientId(char* name) {
	if (!this->nameToTeamSpeakClientIdMap[name]) {
		//goto the client lib and get all the client ids and populate the map
		//TODO: when we figure out how to increase player count, this would need the configurable player count value
		anyID* currentClients = new anyID[16];
		if (ts3client_getClientList(scHandlerID, &currentClients) != ERROR_ok) {
			//might as well wipe the map, since we are fetching all the clients now
			//TODO: probably want to not do this and just remove people the map on certain events
			this->nameToTeamSpeakClientIdMap.clear();
			for (int i = 0; i < 100; i++) {
				anyID clientID = currentClients[i];
				char* clientName;
				/* Query client nickname from ID */
				if (ts3client_getClientVariableAsString(scHandlerID, clientID, CLIENT_NICKNAME, &clientName) != ERROR_ok) {
					TRACE("error trying to client name for mute");
				}
			}
		}
		ts3client_freeMemory(currentClients);
	}
	return this->nameToTeamSpeakClientIdMap[name];
}

/*
 * Mutes the ts client associated with the given name
 */
void TSClient::mute(char* name, bool permanent) {
	int error;
	anyID clientsToMute[1];
	clientsToMute[0] = this->getClientId(name);
	if ((error = ts3client_requestMuteClients(scHandlerID, clientsToMute, NULL)) != ERROR_ok) {
		TRACE("error turning muting clients: %d\n", error);
	}

	if (permanent) {
		//TODO: write out gamertag (maybe even ip and mac addr) to a voice ban file, so these players automatically get banned in any server
		//this client is in with voice enabled
	}
}

/*
 * Unmutes the ts client associated with the given name
 */
void TSClient::unmute(char* name) {
	int error;
	anyID clientsToUnmute[1];
	clientsToUnmute[0] = this->getClientId(name);
	if ((error = ts3client_requestUnmuteClients(scHandlerID, clientsToUnmute, NULL)) != ERROR_ok) {
		TRACE("error turning unmuting clients: %d\n", error);
	}

	//TODO: check if they were permanently banned and remove their entry from the file
}

/*
* This event is called when a client starts or stops talking.
*
* Parameters:
*   serverConnectionHandlerID - Server connection handler ID
*   status                    - 1 if client starts talking, 0 if client stops talking
*   isReceivedWhisper         - 1 if this event was caused by whispering, 0 if caused by normal talking
*   clientID                  - ID of the client who announced the talk status change
*/
void TSClient::onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID teamspeakClientID) {
	char* name;
	/* Query client nickname from ID */
	if (ts3client_getClientVariableAsString(serverConnectionHandlerID, teamspeakClientID, CLIENT_NICKNAME, &name) != ERROR_ok)
		return;

	XUID remoteId = _atoi64(name);
	XUID clientId = _atoi64(client->nickname);
	if (status == STATUS_TALKING) {
		client->nameToTeamSpeakClientIdMap[name] = teamspeakClientID;
		TRACE_GAME_N("Client \"%s\" starts talking.", name);

		client->printCurrentClientVolume(teamspeakClientID);
		if (isLobby) {
			//in the lobby everyone can hear anyone
			//TODO: move into its own method
			client->setClientVolume(teamspeakClientID, MAX_CLIENT_VOLUME_MODIFIER);
			tsUsers->userStartedTalking(remoteId);
		}
		else {
			//TODO: move into its own method
			int remotePlayerIndex = xuidPlayerIndexMap[remoteId];
			BYTE remotePlayerTeamIndex = h2mod->get_team_id(remotePlayerIndex);
			TRACE("Client-%d remotePlayerTeamIndex:%u, xuid:%llu", remotePlayerIndex, remotePlayerTeamIndex, remoteId);

			int clientPlayerIndex = xuidPlayerIndexMap[clientId];
			BYTE clientPlayerTeamIndex = h2mod->get_team_id(clientPlayerIndex);
			TRACE("Client-%d clientPlayerTeamIndex:%u, xuid:%llu", clientPlayerIndex, clientPlayerTeamIndex, clientId);
			if (clientChatMode == PROXIMITY) {
				//client only chat mode (can hear everyone based on distance)
				if (!isLobby) {
					//TODO: mute based on distance
					float remoteX, remoteY, remoteZ;
					float clientX, clientY, clientZ;

					remoteX = h2mod->get_player_x(remotePlayerIndex);
					remoteY = h2mod->get_player_y(remotePlayerIndex);
					remoteZ = h2mod->get_player_z(remotePlayerIndex);

					clientX = h2mod->get_player_x(clientPlayerIndex);
					clientY = h2mod->get_player_y(clientPlayerIndex);
					clientZ = h2mod->get_player_z(clientPlayerIndex);

					double distance = abs(sqrt((pow((clientX - remoteX), 2)) + (pow((clientY - remoteY), 2) + (pow((clientZ - remoteZ), 2)))));

					//TODO: clip distance into a range between max/min client volume
				}
				else {
					//in the lobby we hear everyone
					client->setClientVolume(teamspeakClientID, MAX_CLIENT_VOLUME_MODIFIER);
					tsUsers->userStartedTalking(remoteId);
				}
			}
			else if (clientChatMode == ALL_PLAYERS) {
				//hear everybody all the time
				client->setClientVolume(teamspeakClientID, MAX_CLIENT_VOLUME_MODIFIER);
				tsUsers->userStartedTalking(remoteId);
			}
			else if (clientChatMode == OFF) {
				//don't listen to anyone
				client->setClientVolume(teamspeakClientID, MIN_CLIENT_VOLUME_MODIFIER);
				tsUsers->userStartedTalking(remoteId);
			}
		}
	}
	else {
		TRACE_GAME_N("Client \"%s\" stops talking.", name);
		tsUsers->userStoppedTalking(remoteId);
	}
	ts3client_freeMemory(name);  /* Release dynamically allocated memory only if function succeeded */
}

void TSClient::onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	TRACE_GAME_N("Error for server %llu: %s %s", (unsigned long long)serverConnectionHandlerID, errorMessage, extraMessage);
}

void TSClient::setServerAddress(IN_ADDR address) {
	this->serverAddress = address;
}

void TSClient::setServerPort(unsigned int port) {
	this->port = port;
}

void TSClient::setNickname(char* nickname) {
	this->nickname = nickname;
}

void TSClient::chat() {
	connect();

	//make sure this is false before we start
	stopClient = false;

	while (!stopClient) {}

	disconnect();

	//since this is now true, we reset it for future server runs
	stopClient = false;
}

void TSClient::startChatting() {
	std::thread* clientConnectionThread = new std::thread(&TSClient::chat, this);
}

TSClient::~TSClient() {
	ts3client_freeMemory(identity);  /* Release dynamically allocated memory */
	identity = NULL;

	delete nickname;
}
#ifndef MAP_DOWNLOAD_H
#define MAP_DOWNLOAD_H



class MapManager {
public:
	MapManager();
	void reloadMaps();
	void startMapDownload();
	void searchForMap();
	void resetMapDownloadUrl(); 
	void setCustomLobbyMessage(std::wstring newStatus);
	std::wstring getCustomLobbyMessage();
	void setMapDownloadUrl(std::string url);
	void requestMapDownloadUrl(SOCKET comm_socket, SOCKADDR_IN SenderAddr);
	BOOL hasCheckedMapAlready(std::wstring mapName);

private:
	bool hasMap(std::wstring mapName);
	bool isZipFile(std::string path);
	std::string getFileName(const std::string& s);
	BOOL downloadMap(std::string url, std::wstring mapName);

	std::wstring currentMap;
	std::string mapDownloadUrl;

	std::wstring customLobbyMessage;

	std::set<std::wstring> checkedMaps;
	BOOL alreadyCheckingForMaps;
};

#endif
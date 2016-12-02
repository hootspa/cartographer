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
	void setMapDownloadType(std::string type);
	void requestMapDownloadUrl(SOCKET comm_socket, SOCKADDR_IN SenderAddr);
	bool canDownload(); 

	bool downloadFromUrl();
	bool downloadFromExternal();
	bool downloadFromHost();

private:
	std::wstring getCurrentMapName();
	bool hasMap(std::wstring mapName); 
	void unzipArchive(std::wstring localPath, std::wstring mapsDir);
	BOOL downloadMap(std::wstring mapName);

	std::wstring currentMap;
	std::string mapDownloadUrl;
	std::string mapDownloadType;

	std::wstring customLobbyMessage;

	std::set<std::wstring> checkedMaps;
};

#endif
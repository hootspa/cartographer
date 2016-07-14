#ifndef BAN_H
#define BAN_H

class BanUtility {
public:
	static bool isBannedAddress(IN_ADDR possibleBannedAddress);
	static bool isBannedGamerTag(wchar_t* gamertag);
	static bool isBannedXuid(XUID id);
	static void banPlayer(char* gamertag, IN_ADDR ipAddress, XUID xboxId);
private:
	static BanUtility& getInstance() {
		static BanUtility instance;
		// Guaranteed to be destroyed and Instantiated on first use.
		return instance;
	}
	BanUtility() {};
	BanUtility(BanUtility const&);     // Don't Implement
	void operator=(BanUtility const&); // Don't implement
};
#endif
#include <Windows.h>
#include <WinInet.h>
#include "json.hpp"
#include "H2MOD_ServerList.h"

#pragma comment (lib, "Wininet.lib")
#pragma comment (lib, "urlmon.lib")
#define POST 1
#define GET 0


/*
typedef struct _XLOCATOR_SEARCHRESULT {
ULONGLONG serverID;                     // the ID of dedicated server
DWORD dwServerType;                     // see XLOCATOR_SERVERTYPE_PUBLIC, etc
XNADDR serverAddress;                   // the address dedicated server
XNKID xnkid;
XNKEY xnkey;
DWORD dwMaxPublicSlots;
DWORD dwMaxPrivateSlots;
DWORD dwFilledPublicSlots;
DWORD dwFilledPrivateSlots;
DWORD cProperties;                      // number of custom properties.
PXUSER_PROPERTY pProperties;            // an array of custom properties.
} XLOCATOR_SEARCHRESULT, *PXLOCATOR_SEARCHRESULT;
*/
void Request(int Method, LPCSTR Host, LPCSTR url, LPCSTR header, LPSTR data)
{
	try {
		//Retrieve default http user agent
		char httpUseragent[512];
		DWORD szhttpUserAgent = sizeof(httpUseragent);
		ObtainUserAgentString(0, httpUseragent, &szhttpUserAgent);

		char m[5];

		if (Method == GET)
			strcpy(m, "GET");
		else
			strcpy(m, "POST");

		
		HINTERNET internet = InternetOpenA(httpUseragent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (internet != NULL)
		{

			HINTERNET connect = InternetConnectA(internet, Host, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
			if (connect != NULL)
			{
				HINTERNET request = HttpOpenRequestA(connect, m, url, "HTTP/1.1", NULL, NULL,
					INTERNET_FLAG_HYPERLINK | INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
					INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
					INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
					INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
					INTERNET_FLAG_NO_AUTH |
					INTERNET_FLAG_NO_CACHE_WRITE |
					INTERNET_FLAG_NO_UI |
					INTERNET_FLAG_PRAGMA_NOCACHE |
					INTERNET_FLAG_RELOAD, NULL);

				if (request != NULL)
				{
					int datalen = 0;
					if (data != NULL) datalen = strlen(data);
					int headerlen = 0;
					if (header != NULL) headerlen = strlen(header);

					HttpSendRequestA(request, header, headerlen, data, datalen);

					DWORD dwBytesAvailable;
					while (InternetQueryDataAvailable(request, &dwBytesAvailable, 0, 0))
					{
						BYTE *pMessageBody = (BYTE *)malloc(dwBytesAvailable + 1);

						DWORD dwBytesRead;
						BOOL bResult = InternetReadFile(request, pMessageBody,
							dwBytesAvailable, &dwBytesRead);
						if (!bResult)
						{
							//fprintf(stderr, "InternetReadFile failed, error = %d (0x%x)\n",
							//	GetLastError(), GetLastError());
							break;
						}

						if (dwBytesRead == 0)
							break;	// End of File.

						pMessageBody[dwBytesRead] = '\0';
						
						json11::Json json;
						std::string json_err;
						json = json.parse((char*)pMessageBody, json_err);
					
						
					//	char MsgBox[255];
					//	sprintf(MsgBox, "dwMaxPublicSlots: %i", json["dwMaxPublicSlots"].int_value());

					//MessageBoxA(NULL, MsgBox,  "Test Json", MB_OK);





						//printf("%s", pMessageBody);
						free(pMessageBody);
					}

					InternetCloseHandle(request);
				}
			}
			InternetCloseHandle(connect);
		}
		InternetCloseHandle(internet);
	}
	catch (...) {}
}

void GetServerList()
{

	char URL[1024];
	char* geturi = "/server_list.php";

	wsprintfA(URL, geturi);
	Request(GET, "cartographer.online", URL, NULL, NULL);
}

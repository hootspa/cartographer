#pragma once

class ResponseHandler {
public:
	virtual void handleResponse(char* pMessageBody);
};

void Request(int Method, LPCSTR Host, LPCSTR url, LPCSTR header, LPSTR data, ResponseHandler* handler);
void GetServerList();
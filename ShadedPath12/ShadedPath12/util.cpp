#include "stdafx.h"

/*#include <winerror.h>
#include "errors.h"
// Convert DirectX error codes to exceptions.

error_desc* getErrorDesc(DWORD nErrorCode) {
	long i = 0;
	for each (error_desc error in all_errors)	{
		if (error.e == nErrorCode) return &all_errors[i];
		i++;
	}
	return nullptr;
}

char* iGetLastErrorText(DWORD nErrorCode)
{
	char* msg;
	// Ask Windows to prepare a standard message for a GetLastError() code:
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, nErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
	// Return the message
	if (!msg)
		return("Unknown error");
	else
		return(msg);
}
*/


/*
VOID WINAPI MyDXUTOutputDebugStringW(LPCWSTR strMsg, ...)
{
	WCHAR strBuffer[512];

	va_list args;
	va_start(args, strMsg);
	vswprintf_s(strBuffer, 512, strMsg, args);
	strBuffer[511] = L'\0';
	va_end(args);

	OutputDebugString(strBuffer);
}

wchar_t* string2wstring(std::string text)
{
	static wchar_t wstring[5000];
	MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, wstring, 5000);
	return wstring;
}

void Log(std::string text)
{
	if (text.length() > 500) {
		int len = text.length();
		while (len > 500) {
			std::string sub = text.substr(0, 500);
			text = text.substr(500, text.length() - 500);
			Log(sub);
			len -= 500;
		}
		if (len > 0) Log(text);
		return;
	}
#ifdef UNICODE
	MyDXUTOutputDebugStringW(string2wstring(text));
#else
	DXUTOutputDebugStr(text.c_str());
#endif
#ifdef LOG_STDOUT
	cout << text;
#endif
}

*/

wchar_t* string2wstring(std::string text)
{
	static wchar_t wstring[5000];
	MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, wstring, 5000);
	return wstring;
}

// allow parsing like this: vector<string> topics = split(commandline, ' ');
// discards empty strings which occur when multiple delimiter chars are ina row
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (item.size() > 0) {
			elems.push_back(item);
		}
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

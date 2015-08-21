#pragma once
class XAppBase
{
public:
	XAppBase();
	~XAppBase();

	virtual void init();
	//virtual void resize();
	virtual void update();
	virtual void draw();
	virtual string getWindowTitle() = 0;

protected:
	string myClass;

};

class XApp
{
public:
	XApp();
	~XApp();

	void init();
	void resize();
	void update();
	void draw();
	void report();
	void registerApp(string name, XAppBase*);
	XAppBase* getApp(string appName);
	void setRunningApp(string appName);
	unordered_map<string, string> parameters;
	void parseCommandLine(string commandline);
	bool getBoolParam(string key, bool default_value = false);
	int getIntParam(string key, int default_value = 0);

	void setHWND(HWND h);
	void resize(int width, int height);
	HWND getHWND();
	string commandline;
	HWND hwnd = 0;
	bool ovrRendering;   // use split screen ovr rendering
	int requestWidth, requestHeight;

private:
	unordered_map<string, XAppBase *> appMap;
	ComPtr<IDXGISwapChain3> swapChain;
	bool initialized = false;
	string appName;
};

// reference to global instance:
XApp& xapp();
void xappDestroy();
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

	void registerApp(string name, XAppBase*);

private:
	unordered_map<string, XAppBase *> appMap;
};

// reference to global instance:
XApp& xapp();
void xappDestroy();
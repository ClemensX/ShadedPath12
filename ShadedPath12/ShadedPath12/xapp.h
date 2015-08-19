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


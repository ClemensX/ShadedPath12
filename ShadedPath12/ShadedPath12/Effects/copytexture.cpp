#include "stdafx.h"

void WorkerCopyTextureCommand::perform()
{
	//Log("perform() copy texture command" << endl);
	TextureInfo *tex = xapp->textureStore.getTexture(textureName);
	assert(tex->available);
}

void CopyTextureEffect::init()
{
	xapp = XApp::getInstance();
	assert(xapp->inInitPhase() == true);
	setThreadCount(xapp->getMaxThreadCount());
}

void CopyTextureEffect::setThreadCount(int max)
{
	assert(xapp != nullptr);
	assert(xapp->inInitPhase() == true);
	worker.resize(max);
}

void CopyTextureEffect::draw(string texName)
{
	assert(xapp->inInitPhase() == false);
	// get ref to current command: (here just the frame number, may be more complicated in other effects)
	int index = xapp->getCurrentBackBufferIndex();
	WorkerCopyTextureCommand *c = &worker.at(index);
	c->type = CommandType::WorkerCopyTexture;
	c->textureName = texName;
	c->xapp = xapp;
	//c->commandDetails = c;
	xapp->workerQueue.push(c);
}


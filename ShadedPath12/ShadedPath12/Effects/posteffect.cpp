#include "stdafx.h"

void PostEffect::init()
{
	auto frameCount = xapp().FrameCount;
	Log("PostEffect init for " << frameCount << " frames called" << endl);

	// create render textures for post effects:
	DXGI_SWAP_CHAIN_DESC1 scd;
	xapp().swapChain->GetDesc1(&scd);
	Log("" << scd.Format << endl);
}

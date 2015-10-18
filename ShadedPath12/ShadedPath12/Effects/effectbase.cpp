#include "stdafx.h"

// Wait for pending GPU work to complete.
void EffectBase::WaitForGpu()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// Schedule a Signal command in the queue.
	ThrowIfFailed(xapp().commandQueue->Signal(fence.Get(), fenceValues[frameIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	fenceValues[frameIndex]++;
	//Log("fence frame " << frameIndex << endl);
}


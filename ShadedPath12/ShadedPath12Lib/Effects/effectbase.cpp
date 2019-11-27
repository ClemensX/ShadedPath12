#include "stdafx.h"
//#include "effectbase.h"

void EffectBase::createSyncPoint(FrameResourceSimple &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void EffectBase::waitForSyncPoint(FrameResourceSimple & f)
{
	//	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT64 completed = f.fence->GetCompletedValue();
	//Log("ev start " << frameIndex << " " << completed << " " << f.fenceValue << endl);
	if (completed == -1) {
		Error(L"fence.GetCompletedValue breakdown");
	}
	if (completed > 100000) {
		//Log("ev MAX " << completed << " " << f.fenceValue << endl);
	}
	if (completed <= f.fenceValue)
	{
		WaitForSingleObject(f.fenceEvent, INFINITE);
	}
	else {
		//Log("ev " << completed << " " << f.fenceValue << endl);
	}
}


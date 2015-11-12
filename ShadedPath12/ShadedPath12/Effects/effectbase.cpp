#include "stdafx.h"

void EffectBase::createSyncPoint(FrameResource &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void EffectBase::waitForSyncPoint(FrameResource & f)
{
	UINT64 completed = f.fence->GetCompletedValue();
	if (completed < f.fenceValue)
	{
		WaitForSingleObject(f.fenceEvent, INFINITE);
	}
}

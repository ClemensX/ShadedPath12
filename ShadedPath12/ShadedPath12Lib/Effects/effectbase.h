class GlobalEffect;
class EffectBase {
protected:
public:
	static void createSyncPoint(FrameResourceSimple &f, ComPtr<ID3D12CommandQueue> queue);
	static void waitForSyncPoint(FrameResourceSimple &f);
};


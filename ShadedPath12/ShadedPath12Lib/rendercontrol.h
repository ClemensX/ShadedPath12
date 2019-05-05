class RenderControl {
public:
	// init, called from xapp.init()
	void init(XApp *xapp);
	// get next finished Frame from render engine
	// frame resource has to be in state D3D12_RESOURCE_STATE_COPY_SOURCE
	ID3D12Resource * getNextFrame();
private:
	ResourceStateHelper * resourceStateHelper = nullptr;
};
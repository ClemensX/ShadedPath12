#pragma once
class Frame
{
public:
	Frame() {
		LogF("Frame c'tor\n");
	};
	virtual ~Frame();
};

class FrameBuffer
{
public:
	FrameBuffer() {}
	void resize(size_t size) {
		assert(frames.size() == 0);
		frames.resize(size);
	}
private:
	vector<Frame> frames;
};
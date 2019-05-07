#pragma once
class Frame
{
public:
	Frame() {
		LogF("Frame c'tor\n");
	};
	virtual ~Frame();
	long long absFrameNumber = 0LL;
};

// Frame storage. Get access in a round-robin manner. 
// It is application duty to never use more Frames at the same time than is has size
class FrameBuffer
{
public:
	FrameBuffer() {}
	void resize(size_t size) {
		assert(frames.size() == 0);  // prevent setting size more than once
		frames.resize(size);
		next_free = 0;
	}

	size_t getNextFameIndex() {
		size_t ret = next_free++;
		if (next_free >= frames.size()) {
			next_free = 0;
		}
		return ret;
	}

	Frame* getNextFrame() {
		size_t i = getNextFameIndex();
		return &frames[i];
	}
private:
	vector<Frame> frames;
	size_t next_free = 0;
};
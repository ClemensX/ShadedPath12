#pragma once
class Frame
{
public:
	Frame() {
		LogF("Frame c'tor\n");
	};
	virtual ~Frame();
	long long absFrameNumber = 0LL;
	boolean inUse = false;
};

// Frame storage. Get access in a round-robin manner. 
// getting next free slot will block if all frames are in use
class FrameBuffer
{
public:
	FrameBuffer() {}
	void resize(size_t size) {
		assert(frames.size() == 0);  // prevent setting size more than once
		frames.resize(size);
		free_slots = size;
	}

	// get next free slot, block if currently no free slot until one is available
	// returning null means shutdown
	Frame* getNextFrameSlot() {
		unique_lock<mutex> lock(monitorMutex);
		size_t slot = getFreeFameIndex();
		while (slot == -1) {
			cond.wait_for(lock, chrono::milliseconds(3000));
			LogF("FrameBuffer wait suspended\n");
			if (in_shutdown) {
				LogF("FrameBuffer shutdown in getNextFrame\n");
				return nullptr;
			}
		}
		Frame *frame = &frames[slot];
		frame->inUse = true;
		free_slots--;
		LogF("Get Frame, free: " << free_slots << endl);
		return frame;
	}

	// return frame, any resources should be already detached so fram is ready to be overwritten by next usage
	void returnFrame(Frame* frame) {
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			throw "FrameBuffer shutdown in returnFrame";
		}
		frame->inUse = false;
		frame->absFrameNumber = -1LL;
		free_slots++;
		LogF("return Frame, free: " << free_slots << endl);
		cond.notify_one();
	}

	size_t currentlyFreeSlots() {
		return free_slots;
	}

	void shutdown() {
		in_shutdown = true;
		cond.notify_all();
	}

private:
	// get next free slot index, -1 if nothing free
	// will always start with search at index 0: Should be fast enough for real-time usage with 2 or 3 frames
	// no synchronization: has to be done from caller
	size_t getFreeFameIndex() {
		for (size_t i = 0; i < frames.size(); i++) {
			if (!frames[i].inUse) {
				return i;
			}
		}
		return -1;
	}

	vector<Frame> frames;
	size_t free_slots = 0;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};
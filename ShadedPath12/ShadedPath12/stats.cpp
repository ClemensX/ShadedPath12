#include "stdafx.h"

Stats::Stats()
{
}

void Stats::startUpdate(GameTime &gameTime)
{
	long long absFrameCount = xapp->getAbsFrameCount();
	if (absFrameCountStartGathering <= absFrameCount && absFrameCount < absFrameCountStartGathering + numFramesGathered) {
		LARGE_INTEGER qwTime;
		QueryPerformanceCounter(&qwTime);
		LONGLONG now = qwTime.QuadPart;
		started[absFrameCount - absFrameCountStartGathering] = now;
	}
}

void Stats::startDraw(GameTime &gameTime)
{
}

void Stats::endUpdate(GameTime &gameTime)
{
}

void Stats::endDraw(GameTime &gameTime)
{
	long long absFrameCount = xapp->getAbsFrameCount();
	if (absFrameCountStartGathering <= absFrameCount && absFrameCount < absFrameCountStartGathering + numFramesGathered) {
		LARGE_INTEGER qwTime;
		QueryPerformanceCounter(&qwTime);
		LONGLONG now = qwTime.QuadPart;
		ended[absFrameCount - absFrameCountStartGathering] = now;
	}
}

Stats::~Stats()
{
	//unsigned int hwc = thread::hardware_concurrency;
	Log(" HW concurrency: " << thread::hardware_concurrency() << endl);
	for (const auto &si : statTopics) {
		const StatTopic *st = &si.second;
		Log("" << getInfo(si.first));
	}
	//for (int i = 0; i < numFramesGathered; i++) {
	//	Log("stat frame " << absFrameCountStartGathering + i << " " << started[i] - started[0] << " " << ended[i] - started[0] << endl);
	//}
}

long long Stats::getNow() {
	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	LONGLONG now = qwTime.QuadPart;
	return now;
}

wstring Stats::getInfo(string name)
{
	LARGE_INTEGER qwFrequency;
	QueryPerformanceFrequency(&qwFrequency);
	StatTopic *t = get(name);
	if (t->called == 0L) {
		t->called = 1;
	}
	long long average = t->cumulated / t->called;
	// convert average to micro seconds
	average *= 1000000;
	average /= qwFrequency.QuadPart;
	// total time in ms:
	long long total = t->cumulated * 1000;
	total /= qwFrequency.QuadPart;
	wstringstream s;
	s << "average " << s2w(name) << " microseconds(10^-6): " << average << " total [ms]: " << total << " called: " << t->called << endl;
	return s.str();
}

void Stats::start(string topic)
{
	StatTopic *t = &statTopics[topic];
	t->curStart = getNow();
}

void Stats::end(string topic)
{
	StatTopic *t = &statTopics[topic];
	long long duration = getNow() - t->curStart;
	t->cumulated += duration;
	t->called++;
}

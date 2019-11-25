#pragma once
class GameTime
{
public:
	GameTime();
	~GameTime();
	enum GametimePredefs { GAMEDAY_1_MINUTE = 24 * 60, GAMEDAY_20_SECONDS = 24 * 60 * 3, GAMEDAY_10_SECONDS = 24 * 60 * 6 };
private:
	LONGLONG now;
	LONGLONG last;
	LONGLONG ticks_per_sec;
	LONGLONG start;
	LONGLONG ticks_per_game_day;
	LONGLONG gameday;
	LARGE_INTEGER qwTime;
	double timeOfDay;
	double timeAbs;
	double timeRel;
	double timeDelta;
public:
	// advances time, should be called once for every frame
	// not thread save - be sure to call from synchronized method (usually after presenting)
	void advanceTime();

	// returns time of day as double in range 0..24
	double getTimeOfDay();

	// get absolute number of hours (and fractions)
	// NEVER use time values as float instead of double: precision is not enough and you will get same time value for actually different times
	double getTimeAbs();

	// get absolute number of seconds (and fractions)
	// NEVER user time values as float: precision is not enough and you will get same time value for actually different times
	double getTimeAbsSeconds();

	// get relative number of seconds (and fractions) since timer creation
	// NEVER user time values as float: precision is not enough and you will get same time value for actually different times
	double getTimeRelSeconds();

	// get delta in seconds since last time
	double getDeltaTime();

	// get real time
	LONGLONG getRealTime();

	// get ticks per second
	LONGLONG getTicksPerSec();

	// get duration in seconds
	double getSecondsBetween(LONGLONG &past, LONGLONG &present);

	// set how much faster a game day passes, 1 == real time, 24*60 is a one minute day
	// init needs be called before any other time method
	void init(LONGLONG gamedayFactor);
};

class ThemedTimer {
	struct TimerEntry {
		long long time = 0;
		bool used = false;
	};
	struct TimerDesc {
		vector<TimerEntry> entries;
		int numSlots; // number of allocated TimerEntry slots
		int pos = 0;  // next input position
	};

public:
	//singleton:
	static ThemedTimer* getInstance() {
		static ThemedTimer singleton;
		return &singleton;
	};

	void create(string name, int slots) {
		TimerDesc td;
		for (int i = 0; i < slots; i++) {
			TimerEntry e;
			td.entries.push_back(e);
		}
		td.numSlots = slots;
		td.pos = 0;
		timerMap[name] = td;
	};

	void add(string name, long long value) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		TimerEntry &t = td.entries[td.pos];
		t.time = value;
		t.used = true;
		td.pos++;
		if (td.pos >= td.numSlots) {
			td.pos = 0;
		}
	};

	int usedSlots(string name) {
		if (!check_name(name)) return -1;
		auto& td = timerMap[name];
		int count = 0;
		for (auto& t : td.entries) {
			if (t.used) count++;
		}
		return count;
	}

	void logEntries(string name) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		// we start at next insert position, then log whole buffer from here to getnewest entry last
		for (int i = td.pos; i <= td.pos + td.numSlots; i++ ) {
			int checkPos = i % td.numSlots;
			auto& t = td.entries[checkPos];
			if (t.used) {
				LogF("" << t.time << endl);
			}
		}
	}

private:
	bool check_name(string name) {
		if (timerMap.count(name) <= 0) {
			// theme not found
			LogF("tried to access themed timer that does not exist: " << name.c_str() << endl);
			return false;
		}
		return true;
	}
	unordered_map<string, TimerDesc> timerMap;
	ThemedTimer() {};								// prevent creation outside this class
	ThemedTimer(const ThemedTimer&);				// prevent creation via copy-constructor
	ThemedTimer& operator = (const ThemedTimer&);	// prevent instance copies
};

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


#include "stdafx.h"


GameTime::GameTime()
{
}


GameTime::~GameTime()
{
}


// advances time, should be called once for every frame
// NEVER user time values as float: precision is not enough and you will get same time value for actually different times
void GameTime::advanceTime()
{
	static double lastTimeAbsD = 0;
	//static float lastTimeAbsF = 0;
	LONGLONG last_backup = last;
	last = now;
	QueryPerformanceCounter(&qwTime);
	now = qwTime.QuadPart;
	LONGLONG now_days_stripped = now % ticks_per_game_day;  // strip days off gametime
	timeOfDay = ((double)now_days_stripped) / ticks_per_game_day * 24;
	//timeOfDay = (double)now_days_stripped / ticks_per_game_day * 24;
	//timeAbs = ((double)now - start) / ticks_per_game_day * 24;
	//timeAbs /= 3600; // go from seconds to hours
	timeAbs = (double)now / ticks_per_game_day * 24;
	timeDelta = ((double)(now - last_backup)) / ticks_per_sec;
	if (timeDelta < 0)
		timeDelta = 0.0f;
	//Log("time day abs delta " << timeOfDay << " " << timeAbs << " " << timeDelta << "\n");
	//Log(" time dbl " << setprecision(20) << timeAbs << endl);
	//float tf = (float)timeAbs;
	assert(lastTimeAbsD != timeAbs);
	lastTimeAbsD = timeAbs;
	//assert(lastTimeAbsF != ((float)timeAbs));
	//lastTimeAbsF = (float)timeAbs;
	//Log(" time f " << setprecision(20) << tf << endl);
}


// returns time of day as double in range 0..24
double GameTime::getTimeOfDay()
{
	return (double)timeOfDay;
}


// get absolute number of hours (and fractions)
double GameTime::getTimeAbs()
{
	return timeAbs;
}

// get absolute number of seconds (and fractions)
double GameTime::getTimeAbsSeconds()
{
	return timeAbs * 3600.0;
}

// get number of seconds since last advanceTime()
double GameTime::getDeltaTime()
{
	return timeDelta;
}

LONGLONG GameTime::getRealTime()
{
	return now;
}

LONGLONG GameTime::getTicksPerSec()
{
	return ticks_per_sec;
}

// get duration in seconds
double GameTime::getSecondsBetween(LONGLONG &past, LONGLONG &present)
{
	double t1 = ((double)past) / ((double)ticks_per_sec);
	double t2 = ((double)present) / ((double)ticks_per_sec);
	double seconds = t2 - t1;
	return abs(seconds);
}

// set how much faster a game day passes, 1 == real time, 24*60 is a one minute day
void GameTime::init(LONGLONG gamedayFactor)
{
	QueryPerformanceCounter(&qwTime);
	start = qwTime.QuadPart;
	QueryPerformanceFrequency(&qwTime);
	ticks_per_sec = qwTime.QuadPart;
	ticks_per_game_day = (24 * 60 * 60) / gamedayFactor * ticks_per_sec;
	advanceTime();  // initializes proper initialization
}


#include "stdafx.h"


GameTime::GameTime()
{
}


GameTime::~GameTime()
{
}


// advances time, should be called once for every frame
void GameTime::advanceTime()
{
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
}


// returns time of day as float in range 0..24
float GameTime::getTimeOfDay()
{
	return (float)timeOfDay;
}


// get number of hours (and fractions) since game timer creation
float GameTime::getTimeAbs()
{
	return (float)timeAbs;
}

// get number of seconds since last advanceTime()
float GameTime::getDeltaTime()
{
	return (float)timeDelta;
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
float GameTime::getSecondsBetween(LONGLONG &past, LONGLONG &present)
{
	double t1 = ((double)past) / ((double)ticks_per_sec);
	double t2 = ((double)present) / ((double)ticks_per_sec);
	float seconds = (float)(t2 - t1);
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


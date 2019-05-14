// PerfTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include "emptyframes.h"
#include "simple2dframe.h"
#include "PerfTest.h"

// TODO now orchestrate frame creation in multiple tests


int main()
{
	//EmptyFrames emptyFrames;
	//emptyFrames.runTest();
	Simple2dFrame simple2dFrame;
	simple2dFrame.runTest();
}


// pre_proc.h : specify preprocessor directives here, not in project configuration
// this helps keeping one set of directives for all sub-projects
//

#pragma once

#define _SVR_
#if !defined(_DEBUG)
#define _DISABLE_GPU_DEBUG_
#endif

// disable all update threads: for application and in all effects:
//#define DISABLE_UPDATE_THREADS

// disable for final build, adds some perf penalty and need pix dll at runtime:
#define USE_PIX
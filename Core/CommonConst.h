// CommonConst.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _COMMON_CONST_H_
#define _COMMON_CONST_H_

enum
{
	DEF_THREAD_GRAPHIC = 0,//not a real thread
	DEF_THREAD_INPUT,
	DEF_THREAD_LOADER_0,
	DEF_THREAD_LOADER_1,
	DEF_THREAD_LOADER_2,
	DEF_THREAD_LOADER_3,
	NUM_DEF_THREAD,

	DEF_THREAD_RESERVE_END = 0x80
};

enum
{
	COMMON_EVENT_THREAD_END = 0xfffffffe,
	COMMON_EVENT_SHUTDOWN = 0xffffffff
};

enum//always use LOCKER_CUSTOM_ID_START as lockers start id, and define it continuous
{
	LOCKER_MAIN = 0,

	LOCKER_CUSTOM_ID_START,
};

#endif
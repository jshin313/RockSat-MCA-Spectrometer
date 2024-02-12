// struct definition for packet zero from MCA
//
//    Copyright (C) 2022-2023 CapeSym, Inc.
//
// Assumes float is always 32-bit, e.g. in gcc compiler on Linux
//

#pragma once
#include <stdint.h>

typedef struct								// to be returned in response to cmd[0,0]
{
	float cps;								// 32-bit count rate of most recent acquisition interval
	float totalCount;						// sum of all spectrum data in all channels
	float totalPulseTime;					// total time in seconds inside pulses
	uint32_t usPerInterval;					// duration of most recent interval in microseconds
	uint32_t totalIntervals;				// total number of intervals used to acquire data
	uint32_t capemcaId;						// the USBD id number defined in usbd_desc.c
	uint32_t detectors;						// number of detectors in array
	uint32_t cpiArray;						// cpi across all detectors for most recent interval
	uint32_t countInRangeArray;				// count in channel range across all detectors
	float xDirection;						// source direction based on counts in range vector
	float yDirection;
	float zDirection;
	uint32_t reserved[4];
} PACKET0_TYPE;								// sizeof(PACKET0_TYPE) = 64 bytes

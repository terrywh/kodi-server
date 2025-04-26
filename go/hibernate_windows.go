package main

// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>

import "C"

func KeepBackend() {
	// C.SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)
	C.SetThreadExecutionState(0x80000000 | 0x00000001 | 0x00000040)
}

func KeepDisplay() {
	// C.SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED)
	C.SetThreadExecutionState(0x80000000 | 0x00000001 | 0x00000002 | 0x00000040)
}

func Restore() {
	// C.SetThreadExecutionState(ES_CONTINUOUS)
	C.SetThreadExecutionState(0x80000000)
}
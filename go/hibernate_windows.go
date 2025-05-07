package main

import (
	"context"
	"syscall"
	"time"
)

var (
	kernel32                = syscall.NewLazyDLL("kernel32.dll")
	SetThreadExecutionState = kernel32.NewProc("SetThreadExecutionState")
)

type HibernateController struct {
}

func (hc *HibernateController) KeepAwake(ctx context.Context) {
	ticker := time.NewTicker(15 * time.Second)
	for {
		select {
		case <-ticker.C:
			// SetThreadExecutionState(ES_SYSTEM_REQUIRED)
			SetThreadExecutionState.Call(0x00000001)
		case <-ctx.Done():
			break
		}
	}
}

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
KEEPING:
	for {
		select {
		case <-ticker.C:
			// 保持清醒不休眠
			SetThreadExecutionState.Call(0x80000000 | 0x00000001 | 0x00000040)
		case <-ctx.Done():
			break KEEPING
		}
	}
	// 恢复
	SetThreadExecutionState.Call(0x80000000)
}

package main

import (
	"fmt"
	"path/filepath"
	"strings"
)

func FormatSize(size int64) string {
	if fsize := float64(size); fsize > 1.1*1024*1024*1024 {
		return fmt.Sprintf("%.1fG", fsize/1024/1024/1024)
	} else if fsize > 1.1*1024*1024 {
		return fmt.Sprintf("%.1fM", fsize/1024/1024)
	} else {
		return fmt.Sprintf("%.1fK", fsize/1024)
	}
}

func ParseType(path string) string {
	switch ext := filepath.Ext(path); ext {
	case ".mp4":
		fallthrough
	case ".mkv":
		fallthrough
	case ".webm":
		return fmt.Sprintf("video/%s", ext[1:])
	case ".css":
		return "text/stylesheet"
	case ".js":
		return "text/javascript"
	case ".json":
		return "text/json"
	case ".yaml":
		return "text/yaml"
	case ".toml":
		return "text/toml"
	case ".html":
		return "text/html"
	case ".php":
		fallthrough
	case ".cpp":
		fallthrough
	case ".c":
		fallthrough
	case ".h":
		fallthrough
	case ".go":
		fallthrough
	case ".rs":
		return "text/plain"
	case ".flac":
		fallthrough
	case ".wma":
		fallthrough
	case ".mp3":
		fallthrough
	case ".wav":
		fallthrough
	case ".ogg":
		return fmt.Sprintf("audio/%s", ext[1:])
	case ".gif":
		fallthrough
	case ".bmp":
		fallthrough
	case ".ico":
		fallthrough
	case ".jpg":
		fallthrough
	case ".jpeg":
		fallthrough
	case ".heic":
		fallthrough
	case ".webp":
		return fmt.Sprintf("image/%s", ext[1:])
	case ".txt":
		return "text/plain"
	case ".md":
		return "text/markdown"
	default:
		if strings.HasSuffix(path, "/") {
			return ""
		} else {
			return "application/octet-stream"
		}
	}
}

func ParseIcon(path string) string {
	switch filepath.Ext(path) {
	case ".mp4":
		fallthrough
	case ".mkv":
		fallthrough
	case ".webm":
		return `<i class="bi bi-film"></i>`
	case ".css":
		fallthrough
	case ".js":
		fallthrough
	case ".json":
		fallthrough
	case ".yaml":
		fallthrough
	case ".toml":
		fallthrough
	case ".html":
		fallthrough
	case ".php":
		fallthrough
	case ".cpp":
		fallthrough
	case ".h":
		fallthrough
	case ".c":
		fallthrough
	case ".go":
		fallthrough
	case ".rs":
		return `<i class="bi bi-file-code"></i>`
	case ".flac":
		fallthrough
	case ".wma":
		fallthrough
	case ".mp3":
		fallthrough
	case ".wav":
		fallthrough
	case ".ogg":
		return `<i class="bi bi-file-music"></i>`
	case ".gif":
		fallthrough
	case ".bmp":
		fallthrough
	case ".jpg":
		fallthrough
	case ".jpeg":
		fallthrough
	case ".heic":
		fallthrough
	case ".webp":
		return `<i class="bi bi-file-image"></i>`
	case ".txt":
		fallthrough
	case ".md":
		return `<i class="bi bi-file-text"></i>`
	default:
		if strings.HasSuffix(path, "/") {
			return `<i class="bi bi-folder"></i>`
		} else {
			return ""
		}
	}
}

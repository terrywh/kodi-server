package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

func IgnoreOnError(err error, args ...any) {
	if err == nil || strings.HasSuffix(err.Error(), ": connection reset by peer") || strings.HasSuffix(err.Error(), ": broken pipe") {
		return
	}
	log.Println(args...)
}

func main() {
	var err error
	err = http.ListenAndServe(":3000", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		path := filepath.Join("./", r.URL.Path)
		stat, err := os.Stat(path)
		if err != nil {
			w.WriteHeader(404)
			return
		}

		log.Println(r.Method, r.URL.Path)

		if r.Method == "GET" && stat.IsDir() {
			fmt.Fprint(w, "<table>\n")
			fmt.Fprint(w, `<tr><td><a href="dde-introduction.mp4">dde-introduction.mp4</a><td><td>2025-02-08 20:23</td><td>114.8M</td>\n`)
			fmt.Fprint(w, "</table>\n")
		} else if r.Method == "HEAD" && !stat.IsDir() {
			w.Header().Set("Content-Length", fmt.Sprint(stat.Size()))
			w.Header().Set("Content-Type", "video/mp4")
			w.Write(nil)
		} else if r.Method == "GET" && !stat.IsDir() {
			file, err := os.Open(path)
			if err != nil {
				w.WriteHeader(403)
				return
			}
			defer file.Close()
			fileRange := r.Header.Get("Range")

			w.Header().Set("Content-Type", "video/mp4")
			if len(fileRange) < 6 {

				w.WriteHeader(200)
				// _, err = io.Copy(w, io.NewSectionReader(file, 0, 256*1024))
				_, err = io.Copy(w, file)
				IgnoreOnError(err, 200, err)
				return
			}
			ranges := strings.SplitN(fileRange[6:], "-", 2)
			start, _ := strconv.ParseInt(ranges[0], 10, 64)
			end, _ := strconv.ParseInt(ranges[1], 10, 64)

			if end == 0 {
				end = stat.Size() - 1
			}

			log.Println("ranges", start, end)

			w.Header().Set("Accept-Ranges", "bytes")
			w.Header().Set("Content-Range", fmt.Sprintf("bytes %d-%d/%d", start, end, stat.Size()))
			w.Header().Set("Content-Length", fmt.Sprint(end-start+1))
			w.WriteHeader(206)
			_, err = io.Copy(w, io.NewSectionReader(file, start, end-start+1))
			IgnoreOnError(err, 200, err)
		} else {
			w.WriteHeader(404)
			return
		}
	}))
	if err != nil {
		log.Fatal(err)
	}
}

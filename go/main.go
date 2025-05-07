package main

import (
	"context"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
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
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	hc := HibernateController{}
	go hc.KeepAwake(ctx)

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

			entries, err := os.ReadDir(path)
			if err != nil {
				w.WriteHeader(500)
				return
			}
			fmt.Fprintf(w, `<html>
<head>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css" crossorigin="anonymous">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.4.1/dist/css/bootstrap.min.css" crossorigin="anonymous">
</head>
<body>
<div class="container">

<div class="row pt-3 pb-2"><div class="col-12">
    <h5>当前路径：<code>%s</code></h5>
</div></div>

<div class="row"><div class="col-12">

<table class="table">
	<thead>
		<tr>
			<th>名称</th>
            <th style="width: 14em;">时间</th>
            <th style="width: 5em;">大小</th>
		</tr>
	</thead>
	<tbody>
		<tr><td><i class="bi bi-box-arrow-in-up"></i> <a href="../">上一层</a></td><td>&nbsp;</td><td>&nbsp;</td></tr>
`, path)
			for _, entry := range entries {
				if strings.HasPrefix(entry.Name(), ".") || entry.Name() == "node_modules" {
					continue
				}
				fpath := filepath.Join(path, entry.Name())
				flink := url.PathEscape(entry.Name())

				stat, err := os.Stat(fpath)
				if err != nil {
					continue
				}

				if stat.IsDir() {
					flink = fmt.Sprintf("%s/", flink)
				}

				fmt.Fprint(w, "\t\t<tr>")
				fmt.Fprintf(w, "\t\t\t<td>%s <a href=\"%s\">%s</a></td>\n", ParseIcon(flink), flink, entry.Name())
				if stat.IsDir() {
					fmt.Fprintf(w, "\t\t\t<td> - </td>\n")
					fmt.Fprintf(w, "\t\t\t<td> - </td>\n")
				} else {
					fmt.Fprintf(w, "\t\t\t<td class=\"text-secondary\"><i class=\"bi bi-calendar2-day\"></i> %s</td>\n", stat.ModTime().Local().Format("2006-01-02 15:04"))
					fmt.Fprintf(w, "\t\t\t<td class=\"text-secondary\">%s</td>\n", FormatSize(stat.Size()))
				}
				fmt.Fprint(w, "\t\t</tr>\n")
			}
			fmt.Fprint(w, `
	</tbody>
</table>

</div></div>

</div>
</body>
</html>`)
		} else if r.Method == "HEAD" && !stat.IsDir() {
			w.Header().Set("Content-Length", fmt.Sprint(stat.Size()))
			w.Header().Set("Content-Type", ParseType(path))
			w.Write(nil)
		} else if r.Method == "GET" && !stat.IsDir() {
			file, err := os.Open(path)
			if err != nil {
				w.WriteHeader(403)
				return
			}
			defer file.Close()
			fileRange := r.Header.Get("Range")

			w.Header().Set("Content-Type", ParseType(path))
			if len(fileRange) < 6 {
				w.WriteHeader(200)
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

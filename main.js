import { serve } from "bun";
import { readdir } from "node:fs/promises";
import { basename } from "node:path";
import { join } from "node:path/posix";

/**
 * 
 * @param {Number} size 
 * @returns {string}
 */
function formatSize(size) {
    if (size > 1.1 * 1024 * 1024 * 1024) {
        return `${(size / 1024 / 1024 / 1024).toFixed(1)}G`;
    } else if (size > 1.1 * 1024 * 1024) {
        return `${(size / 1024 / 1024).toFixed(1)}M`;
    } else {
        return `${(size / 1024).toFixed(1)}K`;
    }
}

/**
 * 
 * @param {Date} date 
 * @returns {string}
 */
function formatDate(date) {
    return `${date.getFullYear().toString().padStart(4, '0')}-${(date.getMonth()+1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')} ${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}`;
}


class SourceBuilder {
    /**
     * @type string
     */
    #html
    constructor() {
        this.reset();
    }

    /**
     * 
     * @param {*} path 
     * @param {*} name 
     * @param {Date} date 
     * @param {*} size 
     */
    file(path, name, date, size) {
        this.#html += `<tr>
            <td><a href="${encodeURIComponent(path)}">${name}</a></td>
            <td>${formatDate(date)}</td>
            <td>${formatSize(size)}</td>
        </tr>
        `;
    }

    reset() {
        this.#html = `<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.4.1/dist/css/bootstrap.min.css" integrity="sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh" crossorigin="anonymous">
<table class="table">
    <thead>
        <tr>
            <td>名称</td>
            <td>时间</td>
            <td>大小</td>
        </tr>
    </thead>
    <tbody>
        `;
    }

    done() {
        this.#html += `</tbody>
</table>`
        const r = this.#html;
        this.reset();
        return r;
    }

}

serve({
    async fetch(req) {
        const url = new URL(req.url);
        console.info(new Date(), req.method, url.pathname);
        if (url.pathname == "/") {
            const sb = new SourceBuilder();
            const files = await readdir("./");
            for (const file of files) {
                const f = Bun.file(file);
                const stats = await f.stat();
                sb.file(file, basename(file), stats.ctime, stats.size);
            }
            return new Response(sb.done(), {
                headers: {
                    "content-type": "text/html; charset=utf-8",
                },
            });
        } else {
            const range = req.headers.get("Range")
            const f = Bun.file(join("./", url.pathname));
            const exists = await f.exists();

            if (range != null) {
                const [start, end ] = req.headers.get("Range").split("=").at(-1).split("-").map((v) => {
                    return v ? Number(v) : Number(f.size)
                }); // [0, 100]
                console.log("RANGE", req.headers.get("Range"), " => ", start, end)
                return new Response(f.slice(start, end), {
                    headers: {
                        "content-range": `bytes ${start}-${end}/${f.size}`,
                        "content-type": f.type,
                    }
                });
            } else if (req.method == "HEAD") {
                return new Response(null, {
                    headers: {
                        "content-type": exists ? f.type : "text/plain",
                        "content-size": exists ? f.size.toString() : "0",
                    }
                })
            } else if (exists) {
                console.log("FILE")
                return new Response(f);
            } else {
                console.log("NNNN")
                return new Response(null, {status: 404});
            }
        }
    },
    // port: 3000,
})
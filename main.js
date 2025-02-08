import { serve } from "bun";
import { readdir } from "node:fs/promises";
import { basename } from "node:path";
import { extname, join } from "node:path/posix";

/**
 * 
 * @param {Number} size 
 * @returns {string}
 */
function formatSize(size) {
    if (size === null) {
        return "-";
    } else if (size > 1.1 * 1024 * 1024 * 1024) {
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
    if (!date) return "-";
    return `${date.getFullYear().toString().padStart(4, '0')}-${(date.getMonth()+1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')} ${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}`;
}

/**
 * 
 * @param {Request} req 
 * @param {import("bun").BunFile} file
 * @returns {Array<number>}
 */
function parseRange(req, file) {
    const range = req.headers.get("Range");
    return range ? range.split("=").at(-1).split("-").map((v) => {
        return v ? Number(v) : null;
    }) : [null, null];
}

function fetchIcon(path) {
    switch (extname(path)) {
    case ".mp4":
    case ".mkv":
    case ".webm":
        return `<i class="bi bi-film"></i>`;
    case ".css":
    case ".js":
    case ".json":
    case ".yaml":
    case ".toml":
    case ".html":
    case ".php":
    case ".cpp":
    case ".go":
    case ".rs":
        return `<i class="bi bi-file-code"></i>`;
    case ".flac":
    case ".wma":
    case ".mp3":
    case ".wav":
    case ".ogg":
        return `<i class="bi bi-file-music"></i>`;
    case ".gif":
    case ".bmp":
    case ".jpg":
    case ".jpeg":
    case ".heic":
    case ".webp":
        return `<i class="bi bi-file-image"></i>`;
    case ".txt":
    case ".md":
        return `<i class="bi bi-file-text"></i>`;
    default:
        if (path == "../") return `<i class="bi bi-box-arrow-in-up"></i>`
        else if (path.endsWith("/")) return `<i class="bi bi-folder"></i>`;
        else return "";
    };
    
}

class SourceBuilder {
    /**
     * @type string
     */
    #html
    constructor() {
        this.#start();
    }

    #start() {
        this.#html = `<html>
<head>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css" crossorigin="anonymous">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.4.1/dist/css/bootstrap.min.css" crossorigin="anonymous">
</head>
<body>`;
    }

    #end() {
        this.#html += `
</body>
</html>`;
    }
    /**
     * 
     * @param {*} path 
     * @param {*} name 
     * @param {Date} date 
     * @param {*} size 
     */
    file(path, name, date, size) {
        this.#html += `<tr>`;
        this.#html += `<td>${fetchIcon(path)} <a href="${path}">${name}</a></td>`;
        if (date) {
            this.#html += `<td class="text-secondary"><i class="bi bi-calendar2-day"></i> ${formatDate(date)}</td>`;
        } else {
            this.#html += `<td> - </td>`;
        }
        this.#html += `<td class="text-secondary">${formatSize(size)}</td>`;
        this.#html += `</tr>`;
    }

    async directory(path) {
        this.#directorStart(path);
        if (path != "./") {
            this.file("../", `上一层`, null, null);
        }
        const entries = await readdir(path);
        for (const entry of entries) {
            if (entry.startsWith(".")) continue;

            const filepath = join(path, entry)
            const filelink = encodeURIComponent(entry);

            const file = Bun.file(filepath);
            const stat = await file.stat();
            this.file(stat.isDirectory() ? `${filelink}/` : `${filelink}`, basename(entry), stat.mtime, stat.isDirectory() ? null : stat.size);
        }
        this.#directorEnd();
    }

    #directorStart(path) {
        this.#html += `
<div class="container">
<div class="row pt-3 pb-2"><div class="col-12">
    <h5>当前路径：<code>${path == "./" ? "/" : "/" + path}</code></h5>
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
        `;
    }

    #directorEnd() {
        this.#html += `</tbody>
</table>
</div></div>
</div>`
    }

    done() {
        this.#end();
        const r = this.#html;
        this.#start();
        return r;
    }

}

const route = new Map();

function handleFile(method, handler) {
    route.set(`${method}:file`, handler);
}

function handleDirectory(method, handler) {
    route.set(`${method}:directory`, handler);
}

function handleNotFound(req, kind) {
    if (req.method == "HEAD") {
        return new Response(null, {status: 404});
    }
    return new Response(`${kind} not found`, {status: 404});
}

async function handle(req) {
    const url = new URL(req.url);
    const path = url.pathname.split("/").map(decodeURIComponent).join("/");
    const file = Bun.file(join("./", path));
    console.info(new Date(), req.method, path);

    let stat;
    try {
        stat = await file.stat();
    } catch(ex) {
        return handleNotFound(req, "resource");
    }
    
    const handler = route.get(`${req.method}:${stat.isDirectory()?"directory":stat.isFile()?"file":"unknown"}`);
    if (!handler) {
        return handleNotFound(req, "handler");
    }

    return await handler(file, stat);
}


handleFile("HEAD", async function(file, stat) {
    return new Response(file.slice(0, 0));
});

handleFile("GET", async function(file, stat) {
    if (extname(file.name) == "") {
        return new Response(file, {
            headers: {
                "content-type": "text/plain",
            },
        });
    } 
    let [start, end] = parseRange(req, file);
    if (start === null || start == 0 && end == null) {
        return new Response(file);
    }
    if (end == null || end >= file.size - 1) {
        return new Response(file.slice(start), {status: 206});
    }
    return new Response(file.slice(start, end + 1), {
        status: 206,
        headers: {
            "accept-ranges": "bytes",
            "content-range": `bytes ${start}-${end}/${file.size}`,
            "content-type": file.type,
        }
    });
});

handleDirectory("GET", async function(file, stat) {
    const builder = new SourceBuilder();
    await builder.directory(file.name);
    return new Response(builder.done(), {
        headers: {
            "content-type": "text/html; charset=utf-8",
        },
    });
});

handleDirectory("HEAD", async function(file, stat) {
    return new Response(null, {status: 405});
})

serve({
    async fetch(req) {
        return await handle(req);
    },
    // port: 3000,
})
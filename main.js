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
        if (path != "./") {
            this.file("../", `上一层`, null, null);
        }
        const entries = await readdir(path);
        for (const entry of entries) {
            const filepath = join(path, entry)
            const filelink = encodeURIComponent(entry);

            const file = Bun.file(filepath);
            const stat = await file.stat();
            this.file(stat.isDirectory() ? `${filelink}/` : `${filelink}`, basename(entry), stat.mtime, stat.isDirectory() ? null : stat.size);
        }
    }

    reset() {
        this.#html = `
        <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css" crossorigin="anonymous">
        <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.4.1/dist/css/bootstrap.min.css" crossorigin="anonymous">
<table class="table">
    <thead>
        <tr>
            <th>名称</th>
            <th>时间</th>
            <th>大小</th>
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
        const path = url.pathname.split("/").map(decodeURIComponent).join("/");
        const file = Bun.file(join("./", path));

        console.info(new Date(), req.method, path);

        let stat;
        try {
            stat = await file.stat();
        } catch(ex) {
            return new Response(null, {status: 404});
        }
        
        if (stat.isDirectory()) {
            const builder = new SourceBuilder();
            await builder.directory(file.name);
            return new Response(builder.done(), {
                headers: {
                    "content-type": "text/html; charset=utf-8",
                },
            });
        } else if (req.method == "HEAD") {
            return new Response(f.slice(0, 0));
        } else if (extname(file.name) == "") {
            return new Response(file, {
                headers: {
                    "content-type": "text/plain",
                },
            });
        } else {
            let [start, end] = parseRange(req, file);
            if (start === null) {
                return new Response(file);
            }
            if (end == null) end = Math.min(file.size, start + 16 * 1024 *  1024);
            return new Response(file.slice(start, end), {
                status: 206,
                headers: {
                    "content-range": `bytes ${start}-${end-1}/${file.size}`,
                    "content-type": file.type,
                }
            });
        }
    },
    // port: 3000,
})
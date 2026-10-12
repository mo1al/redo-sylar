/* script.js —— 主题切换 + 滚动渐显动画
 * 主题的初始设置在页面 <head> 内联脚本中完成（避免闪烁），
 * 这里只负责「点击按钮切换」和「滚动渐显」。 */
(function () {
    "use strict";

    /* ---------- 主题切换 ---------- */
    var toggle = document.getElementById("theme-toggle");
    if (toggle) {
        toggle.addEventListener("click", function () {
            var root = document.documentElement;
            var next = root.getAttribute("data-theme") === "dark" ? "light" : "dark";
            root.setAttribute("data-theme", next);
            try {
                localStorage.setItem("theme", next);
            } catch (e) {
                /* localStorage 不可用（隐私模式等）时静默忽略 */
            }
        });
    }

    /* ---------- 滚动渐显 ---------- */
    var revealEls = document.querySelectorAll(".reveal");
    if (revealEls.length && "IntersectionObserver" in window) {
        var io = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    entry.target.classList.add("in");
                    io.unobserve(entry.target);
                }
            });
        }, { threshold: 0.12, rootMargin: "0px 0px -40px 0px" });

        revealEls.forEach(function (el) { io.observe(el); });
    } else {
        // 降级：直接全部显示
        revealEls.forEach(function (el) { el.classList.add("in"); });
    }
})();

/* ---------- 文章页：阅读进度 + 目录 ---------- */
(function () {
    "use strict";
    var postBody = document.querySelector(".post-body");
    var toc = document.getElementById("toc");
    var progressBar = document.getElementById("progress-bar");

    /* 阅读进度条 */
    if (progressBar) {
        var updateProgress = function () {
            var el = document.documentElement;
            var total = el.scrollHeight - el.clientHeight;
            var pct = total > 0 ? (window.scrollY / total) * 100 : 0;
            progressBar.style.height = pct + "%";
        };
        window.addEventListener("scroll", updateProgress, { passive: true });
        updateProgress();
    }

    /* 目录生成 + 滚动高亮 */
    if (postBody && toc) {
        var headings = postBody.querySelectorAll("h2, h3");
        if (headings.length) {
            var links = [];
            headings.forEach(function (h, i) {
                var id = "toc-" + (i + 1);
                h.id = id;
                var a = document.createElement("a");
                a.href = "#" + id;
                a.className = "toc-level-" + h.tagName.charAt(1);
                a.textContent = h.textContent;
                toc.appendChild(a);
                links.push(a);
            });

            var onScrollSpy = function () {
                var current = null;
                headings.forEach(function (h) {
                    if (h.getBoundingClientRect().top <= 120) current = h;
                });
                links.forEach(function (a) {
                    a.classList.toggle("active", !!current && a.getAttribute("href") === "#" + current.id);
                });
            };
            window.addEventListener("scroll", onScrollSpy, { passive: true });
            onScrollSpy();
        } else {
            toc.style.display = "none";
        }
    }

    /* 目录切换按钮：桌面默认展开，移动端默认收起 */
    var toggle = document.getElementById("toc-toggle");
    if (toggle && toc) {
        var tocOpen = window.matchMedia("(min-width: 1240px)").matches;
        var syncToc = function () { toc.classList.toggle("open", tocOpen); };
        syncToc();
        toggle.addEventListener("click", function () {
            tocOpen = !tocOpen;
            syncToc();
        });
        toc.addEventListener("click", function (e) {
            if (e.target && e.target.tagName === "A" && window.innerWidth < 1240) {
                tocOpen = false;
                syncToc();
            }
        });
    }
})();

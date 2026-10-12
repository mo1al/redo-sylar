#include"service.h"
#include"log.h"
#include"config.h"
#include<filesystem>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cctype>
#include <system_error>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cmark.h>
namespace fs = std::filesystem;
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static mo1al::ConfigVar<std::string>::ptr g_datafile = mo1al::Config::LookUp<std::string>("data_file_config","./data/");
static mo1al::ConfigVar<std::string>::ptr g_staticfile = mo1al::Config::LookUp<std::string>("static_file_config","./static/");

// ============================================================
// 个人信息 
// ============================================================
struct SiteInfo {
    std::string name   = "mo1al";                                    // 昵称 
    std::string slogan = "C++";                              // 顶部徽标文字
    std::string bio    = "我是一名大三的软件工程的学生,因为看到网络上C++知识点还是比较分散,所以在从分享C++的一些问题 积少成多成为一个不错的C++分享地。"; // 一句话介绍
    std::string github = "https://github.com/mo1al";
    std::string qq     = "2362422632";                                // QQ 号
    std::string qq_qr  = "/static/qq.png";                            // QQ 二维码图片路径
    std::string avatar = "/static/mo1al.jpg";                                          // 头像图片路径（如 "/static/avatar.jpg"），留空则用姓名首字母
};
static const SiteInfo g_site;

namespace mo1al
{
namespace http
{
// ---------- 通用工具 ----------

// URL 解码：%XX -> 原始字节
static std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            std::istringstream is(str.substr(i + 1, 2));
            int value = 0;
            is >> std::hex;
            if (is >> value) {
                result += static_cast<char>(value);
                i += 2;
                continue;
            }
        }
        result += str[i];
    }
    return result;
}

// URL 编码：非保留字符原样输出，其余做百分号编码（用于拼链接）
static std::string urlEncode(const std::string& str) {
    std::ostringstream esc;
    esc.fill('0');
    for (unsigned char c : str) {
        if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') {
            esc << static_cast<char>(c);
        } else {
            esc << '%' << std::uppercase << std::hex
                << std::setw(2) << static_cast<int>(c)
                << std::nouppercase << std::dec;
        }
    }
    return esc.str();
}

// HTML 转义，防 XSS
static std::string htmlEscape(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:  out += c;
        }
    }
    return out;
}

// 根据扩展名返回 MIME 类型
static std::string getMimeType(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    if (ext == ".css")   return "text/css; charset=utf-8";
    if (ext == ".js")    return "application/javascript; charset=utf-8";
    if (ext == ".html")  return "text/html; charset=utf-8";
    if (ext == ".svg")   return "image/svg+xml";
    if (ext == ".png")   return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")   return "image/gif";
    if (ext == ".ico")   return "image/x-icon";
    if (ext == ".woff")  return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    return "application/octet-stream";
}

// 分类目录名 -> 展示标题（未列出的目录名原样使用）
static std::string categoryTitle(const std::string& dir) {
    static const std::unordered_map<std::string, std::string> titles = {
        {"cpp", "C++"},           {"c", "C 语言"},        {"stl", "STL"},
        {"os", "操作系统"},        {"linux", "Linux"},
        {"network", "计算机网络"}, {"http", "HTTP"},
        {"db", "数据库"},          {"database", "数据库"},
        {"algorithm", "算法"},     {"ds", "数据结构"},     {"data-structure", "数据结构"},
        {"design-pattern", "设计模式"}, {"concurrency", "并发编程"},
    };
    auto it = titles.find(dir);
    return it == titles.end() ? dir : it->second;
}

// ---------- Markdown 表格支持（cmark 不含 GFM 表格，这里预处理成 HTML） ----------
static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

// 是否为表格行（以 | 开头，且含多个 |）
static bool isTableRow(const std::string& line) {
    std::string t = trim(line);
    return !t.empty() && t[0] == '|' && t.find('|', 1) != std::string::npos;
}

// 是否为表格分隔行（仅由 | - : 空格 组成，且至少一个 -）
static bool isDelimRow(const std::string& line) {
    std::string t = trim(line);
    if (t.empty()) return false;
    bool hasDash = false;
    for (char c : t) {
        if (c == '-') hasDash = true;
        else if (c != '|' && c != ':' && c != ' ' && c != '\t') return false;
    }
    return hasDash;
}

// 按 | 拆分一行单元格（处理 \| 转义，去掉首尾空 cell）
static std::vector<std::string> splitRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.size() && line[i + 1] == '|') {
            cur += '|';
            i++;
            continue;
        }
        if (c == '|') { cells.push_back(cur); cur.clear(); }
        else cur += c;
    }
    cells.push_back(cur);
    for (auto& c : cells) c = trim(c);
    if (!cells.empty() && cells.front().empty()) cells.erase(cells.begin());
    if (!cells.empty() && cells.back().empty()) cells.pop_back();
    return cells;
}

// 渲染单元格内的内联 markdown（去掉 cmark 包裹的 <p>）
static std::string renderInline(const std::string& text) {
    if (text.empty()) return "";
    char* html = cmark_markdown_to_html(text.c_str(), text.size(), CMARK_OPT_UNSAFE);
    if (!html) return htmlEscape(text);
    std::string s(html);
    free(html);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.size() >= 3 && s.compare(0, 3, "<p>") == 0) s = s.substr(3);
    if (s.size() >= 4 && s.compare(s.size() - 4, 4, "</p>") == 0) s = s.substr(0, s.size() - 4);
    return s;
}

// 把 GFM 表格转换成 HTML 表格，其余内容保持原样
static std::string convertTables(const std::string& md) {
    std::vector<std::string> lines;
    std::istringstream in(md);
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);

    std::string out;
    size_t i = 0;
    while (i < lines.size()) {
        if (isTableRow(lines[i]) && i + 1 < lines.size() && isDelimRow(lines[i + 1])) {
            std::vector<std::string> header = splitRow(lines[i]);
            std::vector<std::string> aligns;
            for (const auto& c : splitRow(lines[i + 1])) {
                bool left = !c.empty() && c.front() == ':';
                bool right = !c.empty() && c.back() == ':';
                if (left && right) aligns.push_back("center");
                else if (right) aligns.push_back("right");
                else aligns.push_back("left");
            }
            i += 2;
            std::vector<std::vector<std::string>> body;
            while (i < lines.size() && isTableRow(lines[i])) {
                body.push_back(splitRow(lines[i]));
                i++;
            }
            auto cellAttr = [&](size_t col) -> std::string {
                if (col >= aligns.size() || aligns[col] == "left") return "";
                return std::string(" style=\"text-align:") + aligns[col] + "\"";
            };
            out += "<table>\n<thead>\n<tr>\n";
            for (size_t c = 0; c < header.size(); ++c) {
                out += "<th" + cellAttr(c) + ">" + renderInline(header[c]) + "</th>\n";
            }
            out += "</tr>\n</thead>\n<tbody>\n";
            for (const auto& row : body) {
                out += "<tr>\n";
                for (size_t c = 0; c < row.size(); ++c) {
                    out += "<td" + cellAttr(c) + ">" + renderInline(row[c]) + "</td>\n";
                }
                out += "</tr>\n";
            }
            out += "</tbody>\n</table>\n";
            continue;
        }
        out += lines[i] + "\n";
        i++;
    }
    return out;
}

// ---------- 数据模型与扫描 ----------
struct Question { std::string name; std::string file; bool is_md; };
struct Category { std::string dir; std::string title; std::vector<Question> questions; };

// 扫描 data/ 下的子目录，每个子目录 = 一个分类，目录内的 .md/.txt = 一个问题
static std::vector<Category> scanCategories() {
    std::vector<Category> cats;
    std::error_code ec;
    fs::directory_iterator it(g_datafile->getValue(), ec);
    if (ec) return cats;

    for (const auto& entry : it) {
        if (!entry.is_directory()) continue;
        std::string dir = entry.path().filename().string();
        if (dir.empty() || dir[0] == '.') continue;   // 跳过隐藏目录

        Category cat;
        cat.dir = dir;
        cat.title = categoryTitle(dir);

        std::error_code ec2;
        fs::directory_iterator fit(entry.path(), ec2);
        if (ec2) continue;
        for (const auto& fe : fit) {
            if (!fe.is_regular_file()) continue;
            std::string ext = fe.path().extension().string();
            if (ext != ".md" && ext != ".txt") continue;
            Question q;
            q.name = fe.path().stem().string();
            q.file = fe.path().string();
            q.is_md = (ext == ".md");
            cat.questions.push_back(q);
        }
        std::sort(cat.questions.begin(), cat.questions.end(),
                  [](const Question& a, const Question& b){ return a.name < b.name; });
        if (!cat.questions.empty()) {
            cats.push_back(std::move(cat));
        }
    }
    std::sort(cats.begin(), cats.end(),
              [](const Category& a, const Category& b){ return a.dir < b.dir; });
    return cats;
}

// ---------- 图标 ----------
static const char* iconSun() {
    return "<svg class=\"icon-sun\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
           "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
           "<circle cx=\"12\" cy=\"12\" r=\"4\"/><path d=\"M12 2v2M12 20v2M4.93 4.93l1.41 1.41"
           "M17.66 17.66l1.41 1.41M2 12h2M20 12h2M4.93 19.07l1.41-1.41M17.66 6.34l1.41-1.41\"/></svg>";
}
static const char* iconMoon() {
    return "<svg class=\"icon-moon\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
           "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" aria-hidden=\"true\">"
           "<path d=\"M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z\"/></svg>";
}
static const char* iconGithub() {
    return "<svg viewBox=\"0 0 16 16\" width=\"24\" height=\"24\" fill=\"currentColor\" aria-hidden=\"true\">"
           "<path d=\"M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 "
           "0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53"
           ".63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 "
           "0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27s1.36.09 2 .27"
           "c1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95"
           ".29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0 0 16 8c0-4.42-3.58-8-8-8z\"/></svg>";
}

// ---------- 页面外壳 ----------

// 品牌首字母（用作头像占位）
static std::string brandMark() {
    if (g_site.name.empty()) return "S";
    return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(g_site.name[0]))));
}

static std::string renderNavbar() {
    std::string s;
    s += "<header class=\"navbar\"><div class=\"container navbar-inner\">";
    s += "<a class=\"brand\" href=\"/\"><span>" + htmlEscape(g_site.name) + "</span><span class=\"brand-dot\">.blog</span></a>";
    s += "<div class=\"navbar-right\"><nav class=\"nav-links\">";
    s += "<a href=\"/\">首页</a><a href=\"/cpp\">C++</a>";
    s += "</nav>";
    s += "<button class=\"theme-toggle\" id=\"theme-toggle\" aria-label=\"切换深浅色\" title=\"切换深浅色\">";
    s += iconSun();
    s += iconMoon();
    s += "</button></div></div></header>";
    return s;
}

static std::string renderFooter() {
    std::string s;
    s += "<footer class=\"site-footer\"><div class=\"container\">";
    s += "© 2026 " + htmlEscape(g_site.name) + " · 基于C++的开源项目sylar框架重写版构建";
    s += "</div></footer>";
    return s;
}

// 统一页面外壳
static std::string pageShell(const std::string& title, const std::string& body, bool isHome) {
    std::string s;
    s += "<!DOCTYPE html><html lang=\"zh-CN\" data-theme=\"dark\"><head>";
    s += "<meta charset=\"utf-8\">";
    s += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    s += "<meta name=\"description\" content=\"" + htmlEscape(g_site.bio) + "\">";
    s += "<title>" + htmlEscape(title) + "</title>";
    s += "<link rel=\"stylesheet\" href=\"/static/style.css\">";
    // 主题初始化（内联，避免首屏闪烁）
    s += "<script>(function(){var t;try{t=localStorage.getItem('theme');}catch(e){}"
         "if(!t)t=matchMedia('(prefers-color-scheme: dark)').matches?'dark':'light';"
         "document.documentElement.setAttribute('data-theme',t);})();</script>";
    s += "</head><body>";
    s += "<div class=\"bg-orbs\" aria-hidden=\"true\"><span class=\"orb orb-1\"></span>"
         "<span class=\"orb orb-2\"></span><span class=\"orb orb-3\"></span></div>";
    s += "<div class=\"grid-overlay\" aria-hidden=\"true\"></div>";
    s += renderNavbar();
    s += "<main class=\"container" + std::string(isHome ? "" : " no-hero") + "\">";
    s += body;
    s += "</main>";
    s += renderFooter();
    s += "<script src=\"/static/script.js\" defer></script>";
    s += "</body></html>";
    return s;
}

// ---------- 首页各区块 ----------
static std::string renderHero() {
    std::string s;
    s += "<section class=\"hero\"><div class=\"container hero-inner\">";
    if (g_site.avatar.empty()) {
        s += "<div class=\"avatar\">" + brandMark() + "</div>";
    } else {
        s += "<div class=\"avatar avatar-img\"><img src=\"" + htmlEscape(g_site.avatar) + "\" alt=\"头像\"></div>";
    }
    s += "<span class=\"hero-badge\">" + htmlEscape(g_site.slogan) + "</span>";
    s += "<h1 class=\"hero-name\">" + htmlEscape(g_site.name) + "</h1>";
    s += "<p class=\"hero-bio\">" + htmlEscape(g_site.bio) + "</p>";
    s += "<div class=\"hero-actions\">";
    s += "<a class=\"btn btn-primary\" href=\"/cpp\">浏览 C++ 知识点 <span>→</span></a>";
    s += "</div>";
    s += "</div></section>";
    return s;
}

static std::string renderCpp() {
    std::vector<Category> cats = scanCategories();

    std::string s;
    s += "<section class=\"cpp-head reveal\">";
    s += "<h2>C++ 知识点</h2>";
    s += "<p>分享 C++ 学习中的知识点与心得，点击查看详情。</p>";
    s += "</section>";

    if (cats.empty()) {
        s += "<div class=\"empty-hint\">暂无内容 —— 在 data/ 目录下按分类建子目录（如 cpp/、os/），"
             "每个 .md 文件即一篇知识点。</div>";
        return s;
    }

    s += "<div id=\"cpp\">";
    for (size_t i = 0; i < cats.size(); ++i) {
        const Category& cat = cats[i];
        s += "<section class=\"cat-section reveal\">";
        s += "<div class=\"cat-header\"><h2 class=\"cat-title\"><span class=\"cat-dot\"></span>"
             + htmlEscape(cat.title) + "</h2>";
        s += "<span class=\"cat-count\">" + std::to_string(cat.questions.size()) + " 篇</span></div>";
        s += "<div class=\"q-grid\">";
        for (size_t j = 0; j < cat.questions.size(); ++j) {
            const Question& q = cat.questions[j];
            std::string href = "/post/" + urlEncode(cat.dir) + "/" + urlEncode(q.name);
            std::ostringstream num;
            num << std::setfill('0') << std::setw(2) << (j + 1);
            s += "<a class=\"q-card\" href=\"" + href + "\">";
            s += "<span class=\"q-num\">" + num.str() + "</span>";
            s += "<span class=\"q-title\">" + htmlEscape(q.name) + "</span>";
            s += "<span class=\"q-arrow\">→</span></a>";
        }
        s += "</div></section>";
    }
    s += "</div>";
    return s;
}

static std::string renderContact() {
    std::string s;
    s += "<section class=\"contact reveal\" id=\"contact\">";
    s += "<h2 class=\"section-title\">Contact</h2>";
    s += "<div class=\"contact-grid\">";
    // GitHub
    s += "<a class=\"contact-card\" href=\"" + htmlEscape(g_site.github) + "\" target=\"_blank\" rel=\"noopener\">";
    s += "<span class=\"contact-icon\">" + std::string(iconGithub()) + "</span>";
    s += "<span class=\"contact-meta\"><span class=\"contact-label\">GitHub</span>";
    s += "<span class=\"contact-value\">@" + htmlEscape(g_site.name) + "</span></span></a>";
    // QQ
    s += "<a class=\"contact-card\" href=\"/qq\">";
    s += "<span class=\"contact-icon\">🐧</span>";
    s += "<span class=\"contact-meta\"><span class=\"contact-label\">QQ</span>";
    s += "<span class=\"contact-value\">" + htmlEscape(g_site.qq) + "</span></span></a>";
    s += "</div></section>";
    return s;
}

// ---------- Servlet ----------

// 首页：个人介绍 + 联系方式
ServiceServlet::ServiceServlet(const std::string&name)
    :Servlet("blog service"),m_name(name)
{
}
int32_t ServiceServlet::handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)
{
    std::string body = renderHero() + renderContact();
    response->setStatus(HttpStatus::OK);
    response->setHeader("Content-Type", "text/html; charset=utf-8");
    response->setBody(pageShell(g_site.name + " · " + g_site.slogan, body, true));
    return 0;
}

// C++ 知识点列表页：按分类分组展示所有文章
CppServlet::CppServlet(const std::string&name)
    : Servlet("cpp service"),m_name(name)
{
}
int32_t CppServlet::handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)
{
    response->setStatus(HttpStatus::OK);
    response->setHeader("Content-Type", "text/html; charset=utf-8");
    response->setBody(pageShell("C++ 知识点 · " + g_site.name, renderCpp(), false));
    return 0;
}

// QQ 二维码页
QqServlet::QqServlet(const std::string&name)
    : Servlet("qq service"),m_name(name)
{
}
int32_t QqServlet::handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)
{
    std::string s;
    s += "<article class=\"post-content\">";
    s += "<nav class=\"crumbs\"><a href=\"/\">首页</a><span class=\"sep\">/</span>";
    s += "<span class=\"cur\">QQ</span></nav>";
    s += "<h1 class=\"post-title-h1\">QQ</h1>";
    s += "<div class=\"qr-box\"><img src=\"" + htmlEscape(g_site.qq_qr) + "\" alt=\"QQ 二维码\"></div>";
    s += "<p class=\"qr-desc\">QQ C++ 分享群<span class=\"num\"></span></p>";
    s += "</article>";
    response->setStatus(HttpStatus::OK);
    response->setHeader("Content-Type", "text/html; charset=utf-8");
    response->setBody(pageShell("QQ · " + g_site.name, s, false));
    return 0;
}

// 具体文章：/post/<分类>/<题目>
ArticleServlet::ArticleServlet(const std::string&name)
    : Servlet("article service"),m_name(name)
{
}
int32_t ArticleServlet::handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)
{
    const std::string prefix = "/post/";
    std::string path = request->getPath();
    if (path.compare(0, prefix.size(), prefix) != 0) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    std::string rest = path.substr(prefix.size());       // "分类/题目"
    size_t slash = rest.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 >= rest.size()) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    std::string cat = urlDecode(rest.substr(0, slash));
    std::string name = urlDecode(rest.substr(slash + 1));

    // 防目录穿越 / 非法输入
    if (cat.empty() || name.empty() ||
        cat.find("..") != std::string::npos || name.find("..") != std::string::npos ||
        cat.find('/') != std::string::npos || cat == "." || cat == ".." ||
        name == "." || name == "..") {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }

    std::string md_file = g_datafile->getValue() + cat + "/" + name + ".md";
    std::string txt_file = g_datafile->getValue() + cat + "/" + name + ".txt";
    std::string filename;
    bool is_markdown = false;
    if (fs::exists(md_file)) {
        filename = md_file;
        is_markdown = true;
    } else if (fs::exists(txt_file)) {
        filename = txt_file;
    } else {
        response->setStatus(HttpStatus::NOT_FOUND);
        response->setHeader("Content-Type", "text/html; charset=utf-8");
        response->setBody(pageShell("404 · " + g_site.name,
            "<article class=\"post-content\"><h1 class=\"post-title-h1\">404</h1>"
            "<p>文章不存在。</p><p><a href=\"/\">← 返回首页</a></p></article>", false));
        return 0;
    }

    std::ifstream ifs(filename);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    if (content.empty() && !ifs) {
        response->setStatus(HttpStatus::NOT_FOUND);
        response->setHeader("Content-Type", "text/html; charset=utf-8");
        response->setBody(pageShell("404 · " + g_site.name,
            "<article class=\"post-content\"><h1 class=\"post-title-h1\">404</h1>"
            "<p>文章读取失败。</p><p><a href=\"/\">← 返回首页</a></p></article>", false));
        return 0;
    }

    std::string body;
    if (is_markdown) {
        std::string converted = convertTables(content);
        char* md_html = cmark_markdown_to_html(converted.c_str(), converted.size(), CMARK_OPT_UNSAFE);
        if (md_html) {
            body = md_html;
            free(md_html);
        } else {
            body = htmlEscape(content);
        }
    } else {
        body = "<pre class=\"plain\">" + htmlEscape(content) + "</pre>";
    }

    std::string s;
    s += "<div class=\"reading-progress\"><div class=\"reading-progress-bar\" id=\"progress-bar\"></div></div>";
    s += "<article class=\"post-content\">";
    s += "<nav class=\"crumbs\"><a href=\"/\">首页</a><span class=\"sep\">/</span>";
    s += "<a href=\"/cpp\">C++</a><span class=\"sep\">/</span>";
    s += "<span class=\"cur\">" + htmlEscape(categoryTitle(cat)) + "</span></nav>";
    s += "<h1 class=\"post-title-h1\">" + htmlEscape(name) + "</h1>";
    s += "<div class=\"post-meta\"><span class=\"tag\">" + htmlEscape(categoryTitle(cat)) + "</span></div>";
    s += "<div class=\"post-body\">" + body + "</div>";
    s += "</article>";
    s += "<aside class=\"toc\" id=\"toc\"><div class=\"toc-title\">目录</div></aside>";
    s += "<button class=\"toc-toggle\" id=\"toc-toggle\" aria-label=\"目录\">☰ 目录</button>";

    response->setStatus(HttpStatus::OK);
    response->setHeader("Content-Type", "text/html; charset=utf-8");
    response->setBody(pageShell(name + " · " + g_site.name, s, false));
    return 0;
}

// 静态资源
StaticFileServlet::StaticFileServlet(const std::string&name)
    : Servlet("static file"),m_name(name)
{
}
int32_t StaticFileServlet::handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)
{
    std::string path = request->getPath();
    const std::string prefix = "/static/";
    if (path.compare(0, prefix.size(), prefix) != 0) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    std::string rel = path.substr(prefix.size());
    if (rel.empty() || rel.find("..") != std::string::npos) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    std::string filename = g_staticfile->getValue() + rel;
    if (!fs::exists(filename) || !fs::is_regular_file(filename)) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    std::ifstream ifs(filename, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    if (content.empty() && !ifs) {
        response->setStatus(HttpStatus::NOT_FOUND);
        return 0;
    }
    response->setStatus(HttpStatus::OK);
    response->setHeader("Content-Type", getMimeType(filename));
    response->setBody(content);
    return 0;
}
} // http
} // mo1al

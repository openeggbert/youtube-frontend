/*
 * MIT License
 * Copyright (c) 2024-2025 Robert Vokac
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "YoutubeVideoHtml.h"
#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace std;
namespace fs = std::filesystem;

// Escapes a value for safe use as a single shell argument when pasted into a
// terminal (used only for the copy-paste ffmpeg command shown to the user).
static std::string escapeForShell(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    for (char c : input) {
        switch (c) {
        case ' ':
            out += "\\ ";
            break;
        case '(':
            out += "\\(";
            break;
        case ')':
            out += "\\)";
            break;
        case '#':
            out += "\\#";
            break;
        case '&':
            out += "\\&";
            break;
        case ';':
            out += "\\;";
            break;
        case '|':
            out += "\\|";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\'':
            out += "\\'";
            break;
        case '$':
            out += "\\$";
            break;
        case '`':
            out += "\\`";
            break;
        case '\\':
            out += "\\\\";
            break;
        default:
            out += c;
        }
    }
    return out;
}

static std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << hex << uppercase;

    for (unsigned char c : value) {
        // Safe characters remain the same
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << setw(2) << int(c);
        }
    }
    return escaped.str();
}

// Returns the first UTF-8 codepoint of s (as a byte sequence), uppercased if
// it is a plain ASCII letter. Used for the little avatar-initial bubble next
// to each comment.
static std::string firstUtf8Char(const std::string& s) {
    if (s.empty())
        return "?";

    unsigned char c0 = static_cast<unsigned char>(s[0]);
    size_t len = 1;
    if ((c0 & 0x80) == 0x00) len = 1;
    else if ((c0 & 0xE0) == 0xC0) len = 2;
    else if ((c0 & 0xF0) == 0xE0) len = 3;
    else if ((c0 & 0xF8) == 0xF0) len = 4;
    len = std::min(len, s.size());

    std::string ch = s.substr(0, len);
    if (len == 1 && std::islower(static_cast<unsigned char>(ch[0])))
        ch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(ch[0])));
    return ch;
}

static std::string shortHash(const std::string& hash) {
    return hash.size() > 12 ? hash.substr(0, 6) + "…" + hash.substr(hash.size() - 6) : hash;
}

YoutubeVideoHtml::YoutubeVideoHtml(
    const YoutubeVideo& youtubeVideo,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& archiveBoxArchiveDirectory,
    long countOfVideosInChannel
) {
    std::ostringstream html;

    const std::string titleEsc = Utils::escapeHtml(youtubeVideo.title);
    const std::string finalUrl = "https://www.youtube.com/watch?v=" + youtubeVideo.id;

    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" type="image/x-icon" href="../favicon.ico" sizes="16x16">
<link rel="stylesheet" href="../assets/style.css">
<title>)"
         << titleEsc
         << R"(</title>
</head>
<body>
<header class="site-header"><div class="inner">
<a class="brand" href="../videos.html"><span class="dot"></span>Youtube archive</a>
</div></header>
<main class="page">
<p class="crumb"><a href="../videos.html">← all videos</a></p>
<div class="video-page">
<div>
)";

    // ----------- Player -----------
    std::string encodedFile = urlEncode(youtubeVideo.videoFileName);
    std::string videoLocalUrl =
        "file:///" +
        (archiveBoxRootDirectory / "archive" / youtubeVideo.snapshot / "media" / encodedFile).string();

    html << "<div class=\"player\">";
    if (!youtubeVideo.videoFileName.ends_with(".mkv")) {
        html << "<video src=\"../archive/" << youtubeVideo.snapshot
             << "/media/" << encodedFile
             << "\" controls>"
             << "Your browser does not support video playback."
             << "</video>";
    } else {
        html << "<a target=\"_blank\" href=\"" << videoLocalUrl << "\">"
             << "<img src=\"../archive/" << youtubeVideo.snapshot
             << "/media/thumbnail." << youtubeVideo.getThumbnailFormat()
             << "\" alt=\"" << titleEsc << "\">"
             << "</a>";
    }
    html << "</div>\n";

    html << "<h1 class=\"video-title\">" << titleEsc << "</h1>\n";

    // ----------- Prev/Next navigation -----------
    bool backEnabled =
        youtubeVideo.number > 1 && !youtubeVideo.previousVideoId.empty();
    bool nextEnabled =
        youtubeVideo.number < countOfVideosInChannel &&
        !youtubeVideo.nextVideoId.empty();

    html << "<div class=\"nav-row\">";
    if (backEnabled)
        html << "<a class=\"pill\" href=\"./" << youtubeVideo.previousVideoId << ".html\">← Back</a>";
    else
        html << "<span class=\"pill disabled\">← Back</span>";

    if (nextEnabled)
        html << "<a class=\"pill primary\" href=\"./" << youtubeVideo.nextVideoId << ".html\">Next →</a>";
    else
        html << "<span class=\"pill primary disabled\">Next →</span>";

    html << "<span class=\"n\">#" << youtubeVideo.number << " / " << countOfVideosInChannel << "</span>";
    html << "</div>\n";

    // ----------- Metadata chips -----------
    double mb = (double)youtubeVideo.videoFileSizeInBytes / 1024.0 / 1024.0;
    std::string uploadDate = youtubeVideo.uploadDate;
    if (uploadDate.size() >= 8)
        uploadDate = uploadDate.substr(0, 4) + "-" + uploadDate.substr(4, 2) + "-" + uploadDate.substr(6, 2);

    html << "<div class=\"chip-row\">";
    html << "<span class=\"chip\">" << Utils::formatDurationShort(youtubeVideo.videoDuration) << "</span>";
    html << "<span class=\"chip\">" << std::fixed << std::setprecision(2) << mb << " MB</span>";
    if (!uploadDate.empty())
        html << "<span class=\"chip\">" << Utils::escapeHtml(uploadDate) << "</span>";
    html << "<a class=\"chip dl\" href=\"../archive/" << youtubeVideo.snapshot << "/media/" << encodedFile << "\">⭳ Download</a>";
    html << "<a class=\"chip\" target=\"_blank\" href=\"" << finalUrl << "\">▶ On YouTube</a>";
    html << "</div>\n";

    // ----------- Description -----------
    html << "<div class=\"desc\">";
    if (youtubeVideo.description.empty())
        html << "No description";
    else
        html << Utils::escapeHtml(youtubeVideo.description);
    html << "</div>\n";

    // ----------- Comments -----------
    html << "<div class=\"comments\">";
    html << "<h2>Comments";
    if (!youtubeVideo.comments.empty())
        html << " · " << youtubeVideo.comments.size();
    html << "</h2>";
    html << "<div class=\"thread\">";

    for (const auto& co : youtubeVideo.comments) {
        html << "<div class=\"c\" style=\"--depth:" << co.dotCount() << ";\">";

        html << "<div class=\"avatar\">" << Utils::escapeHtml(firstUtf8Char(co.author)) << "</div>";
        html << "<div class=\"c-body\">";

        html << "<div class=\"c-head\"><span class=\"c-author\">" << Utils::escapeHtml(co.author) << "</span>";

        auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(co.timestamp));
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&t);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
        html << "<span class=\"c-time\">" << buf << "</span></div>";

        html << "<div class=\"c-text\">" << Utils::escapeHtml(co.text) << "</div>";

        html << "</div></div>\n";
    }

    html << "</div></div>\n"; // .thread .comments
    html << "</div>\n"; // left column

    // ----------- Sidebar -----------
    html << "<div class=\"side\">";

    html << "<div class=\"box\"><h2>File details</h2>";
    html << "<div class=\"kv\"><span class=\"k\">Format</span><span class=\"v\">"
         << Utils::escapeHtml(youtubeVideo.ext.empty() ? "?" : youtubeVideo.ext) << "</span></div>";
    html << "<div class=\"kv\"><span class=\"k\">Duration</span><span class=\"v\">"
         << Utils::escapeHtml(youtubeVideo.videoDuration) << "</span></div>";
    html << "<div class=\"kv\"><span class=\"k\">Size</span><span class=\"v\">"
         << std::fixed << std::setprecision(2) << mb << " MB</span></div>";
    if (!youtubeVideo.videoFileSha512HashSum.empty())
        html << "<div class=\"kv\"><span class=\"k\">SHA-512</span><span class=\"v\" title=\""
             << youtubeVideo.videoFileSha512HashSum << "\">"
             << shortHash(youtubeVideo.videoFileSha512HashSum) << "</span></div>";
    html << "</div>\n";

    if (youtubeVideo.videoFileName.ends_with(".mkv")) {
        std::string vEsc = escapeForShell(youtubeVideo.videoFileName);
        std::string vWebm = vEsc.substr(0, vEsc.size() - 3) + "webm";

        std::ostringstream cmd;
        cmd << "cd " << (archiveBoxArchiveDirectory / youtubeVideo.snapshot / "media").string()
            << " && ffmpeg -i " << vEsc << " -preset slow -crf 18 " << vWebm;

        html << "<div class=\"box cmd-box\"><h2>Convert to WebM</h2>"
             << "<input type=\"text\" class=\"cmd-input\" readonly value=\""
             << Utils::escapeHtml(cmd.str()) << "\"></div>\n";
    } else {
        html << "<div class=\"box cmd-box\"><h2>File location</h2>"
             << "<input type=\"text\" class=\"cmd-input\" readonly value=\""
             << Utils::escapeHtml((archiveBoxArchiveDirectory / youtubeVideo.snapshot / "media").string())
             << "\"></div>\n";
    }

    html << "<div class=\"box cmd-box\"><h2>Source URL</h2>"
         << "<input type=\"text\" class=\"cmd-input\" readonly value=\""
         << Utils::escapeHtml(finalUrl) << "\"></div>\n";

    html << "</div>\n"; // .side
    html << "</div>\n"; // .video-page
    html << "</main>\n";
    html << "</body></html>";

    singleVideo = html.str();
}

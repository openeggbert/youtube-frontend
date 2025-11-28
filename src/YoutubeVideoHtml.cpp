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
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace std;
namespace fs = std::filesystem;

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

YoutubeVideoHtml::YoutubeVideoHtml(
    const YoutubeVideo& youtubeVideo,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& archiveBoxArchiveDirectory,
    long countOfVideosInChannel
) {
    std::ostringstream html;

    html << R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<link rel="icon" type="image/x-icon" href="../favicon.ico" sizes="16x16">
<title>)"
         << youtubeVideo.title
         << R"(</title>
<style>
body {padding:20px;}
* { font-family:Arial; }
</style>
</head>
<body>
)";

    std::string finalUrl = "https://www.youtube.com/watch?v=" + youtubeVideo.id;

    html << "<input type=\"text\" id=\"youtube_url\" name=\"youtube_url\" size=\"60\" width=\"60\" "
         << "style=\"margint-bottom:20px;margin-right:10px;font-size:110%;padding:5px;\" "
         << "value=\"" << finalUrl << "\"><br><br>\n";

    html << "<a target=\"_blank\" href=\"" << finalUrl << "\">"
         << finalUrl << "</a><br>\n";

    // URL-encode local filename
    std::string encodedFile = urlEncode(youtubeVideo.videoFileName);
    std::string videoLocalUrl =
        "file:///" +
        (archiveBoxRootDirectory / "archive" / youtubeVideo.snapshot / "media" / encodedFile).string();

    // If not MKV, embed HTML5 video
    if (!youtubeVideo.videoFileName.ends_with(".mkv")) {
        html << "<video src=\"../archive/" << youtubeVideo.snapshot
             << "/media/" << encodedFile
             << "\" controls height=\"440px\">"
             << "Your browser does not support the video tag."
             << "</video><br>\n";
    } else {
        html << "<a target=\"_blank\" href=\"" << videoLocalUrl << "\">"
             << "<img style=\"margin:10px;height:500px;\" src=\"../archive/"
             << youtubeVideo.snapshot
             << "/media/thumbnail."
             << youtubeVideo.getThumbnailFormat()
             << "\"></a><br>\n";
    }

    html << "<span style=\"font-size:200%;font-weight:bold;\">"
         << youtubeVideo.title << "</span><br><br>\n";

    html << "#" << youtubeVideo.number << "&nbsp;&nbsp;&nbsp;";

    bool backEnabled =
        youtubeVideo.number > 1 && !youtubeVideo.previousVideoId.empty();

    html << "<button "
         << (backEnabled ? "" : "disabled")
         << " style=\""
         << (backEnabled ? "" : "visibility:hidden;")
         << "font-size:200%;\" onclick=\"window.location ='./"
         << youtubeVideo.previousVideoId << ".html'\">Back</button>";

    html << "&nbsp;&nbsp;&nbsp;";

    bool nextEnabled =
        youtubeVideo.number < countOfVideosInChannel &&
        !youtubeVideo.nextVideoId.empty();

    html << "<button "
         << (nextEnabled ? "" : "disabled")
         << " style=\""
         << (nextEnabled ? "" : "visibility:hidden;")
         << "font-size:200%;\" onclick=\"window.location ='./"
         << youtubeVideo.nextVideoId << ".html'\">Next</button> ";

    html << "<br><br><a href=\"../archive/"
         << youtubeVideo.snapshot << "/media/" << encodedFile
         << "\">Download</a> ";

    double mb = (double)youtubeVideo.videoFileSizeInBytes / 1024.0 / 1024.0;
    html << std::fixed << std::setprecision(2) << mb << " MB ";

    if (youtubeVideo.videoFileName.ends_with(".mkv")) {
        std::string v = youtubeVideo.videoFileName;
        std::string vEsc = escapeForShell(v);
        std::string vWebm = vEsc.substr(0, vEsc.size() - 3) + "webm";

        html << "<input type=\"text\" size=\"100\" style=\"margin-bottom:20px;margin-right:10px;"
             << "font-size:110%;padding:5px;\" value=\"";
        html << "cd " << (archiveBoxArchiveDirectory / youtubeVideo.snapshot / "media").string()
             << " && ffmpeg -i " << vEsc << " -preset slow -crf 18 " << vWebm;
        html << "\"><br>";
    } else {
        html << "<input type=\"text\" size=\"100\" style=\"margin-bottom:20px;margin-right:10px;"
             << "font-size:110%;padding:5px;\" value=\"";
        html << (archiveBoxArchiveDirectory / youtubeVideo.snapshot / "media").string();
        html << "\"><br>";
    }

    html << "<br><br><br>\n";

    // Description
    html << "<pre style=\"white-space: pre-wrap; border:1px solid black;max-width:600px;"
         << "padding:10px;min-height:50px;\">";
    if (youtubeVideo.description.empty())
        html << "No description";
    else
        html << youtubeVideo.description;
    html << "</pre>";

    // Comments
    html << "<h2>Comments</h2>";

    for (const auto& co : youtubeVideo.comments) {
        html << "<div style=\"margin-left:" << (co.dotCount() * 50) << "px;\">";
        html << "<h3>" << co.author << "</h3>";

        // Convert timestamp → formatted date
        auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(co.timestamp));
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&t);

        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        html << "<span style=\"color:grey;font-size:80%;\">" << buf << "</span><br>\n";
        html << "<span style=\"color:grey;font-size:80%;\">" << co.id << " " << co.parentId << "</span><br>\n";

        html << "<pre style=\"white-space: pre-wrap;border:1px solid black;"
             << "max-width:600px;padding:10px;min-height:50px;\">"
             << co.text << "</pre>";

        html << "</div>";
    }

    html << "</body></html>";

    singleVideo = html.str();
}

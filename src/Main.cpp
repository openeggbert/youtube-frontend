///////////////////////////////////////////////////////////////////////////////////////////////
// youtubedl-frontend: Tool generating html pages for Archive Box.
// Copyright (C) 2024-2025 the original author or authors.
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see 
// <https://www.gnu.org/licenses/> or write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
///////////////////////////////////////////////////////////////////////////////////////////////


// import java.io.ByteArrayInputStream;
// import java.io.File;
// import java.io.IOException;
// import java.nio.file.Files;
// import java.util.ArrayList;
// import java.util.Collections;
// import java.util.HashMap;
// import java.util.List;
// import java.util.Map;
// import static com.openeggbert.utils.youtubedlfrontend.Args.TWO_DASHES;

/**
 * @author <a href="mailto:mail@robertvokac.com">Robert Vokac</a>
 * @since 0.0.0
 */

// Main.cpp
// youtubedl-frontend: Tool generating HTML pages for Archive Box.
// Copyright (C) 2024
// GPLv3 license — see original project header for details.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "Args.h"
#include "ArgType.h"
#include "YoutubeVideo.h"
#include "YoutubeVideoHtml.h"
#include "Utils.h"
#include "Constants.h"

namespace fs = std::filesystem;

// Static variables equivalent to Java static fields
static int iii = 0;
static int internalStaticVariableVideoNumberPerRow = 0;
static int processedVideos = 0;

// Forward declaration
static std::string createChannelHtml(
    const std::optional<std::string>& wantedChannelName,
    const std::vector<std::string>& channels,
    const Args& argsInstance,
    const std::map<std::string, std::string>& channelUrls,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& archiveBoxArchiveDirectory);


static std::string encode_base64(const std::vector<unsigned char>& data) {
        BIO* bio = nullptr;
        BIO* b64 = nullptr;
        BUF_MEM* bufferPtr = nullptr;

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, data.data(), (int)data.size());
        BIO_flush(bio);

        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string output(bufferPtr->data, bufferPtr->length);

        BIO_free_all(bio);

        return output;
    }


// ------------------- MAIN -----------------------
int main(int argc, char** argv) {
    std::cout << "youtubedlfrontend - HTML generator\n\n";

    // Java's args[] does not include the program name, so we skip argv[0]
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    // Same fallback behavior as Java
    if (args.size() < 1) {
        std::string argsS =
            "/rv/big/foreign-blupi-videos-on-youtube --video_ 5rGd2VQz3mo --always-generate-metadata 0"
            " --always-generate-html-files 1 --videos-per-row 4 --thumbnail-links-to-youtube 0"
            " --thumbnail-as-base64 0"
            " --channel_ UCqBpgfXap7cZOYkAC34u8Lg";

        std::stringstream ss(argsS);
        std::string token;
        args.clear();
        while (ss >> token)
            args.push_back(token);
        // Java does not exit here (exit was commented out), so we do not exit either
    }

    Args argsInstance(args);
    std::cout << argsInstance.to_string() << "\n";

    // Determine working directory the same way Java does
    std::string workingDirectory;
    if (!args.empty() && args[0].rfind(Args::TWO_DASHES, 0) != 0)
        workingDirectory = args[0];
    else
        workingDirectory = fs::absolute(".").string();

    fs::path archiveBoxRootDirectory = workingDirectory;
    fs::path archiveBoxArchiveDirectory = archiveBoxRootDirectory / "archive";

    // Load videos (same behavior as Java)
    std::vector<YoutubeVideo> youtubeVideos =
        YoutubeVideo::loadYoutubeVideos(archiveBoxArchiveDirectory, argsInstance);

    std::map<std::string, std::string> channelUrls;
    std::vector<std::string> channels;

    // Build channel name → channel URL map and list of unique channels
    for (auto& v : youtubeVideos) {
        const std::string& channelName = v.channelName;
        if (!channelName.empty() && !channelUrls.contains(channelName)) {
            channelUrls[channelName] = v.channelUrl;
            channels.push_back(channelName);
        }
    }

    // Sort channels case-insensitive (same as Java o1.toLowerCase().compareTo)
    std::sort(channels.begin(), channels.end(),
              [](const std::string& a, const std::string& b) {
                  std::string A = a, B = b;
                  std::transform(A.begin(), A.end(), A.begin(), ::tolower);
                  std::transform(B.begin(), B.end(), B.begin(), ::tolower);
                  return A < B;
              });

    // Output directories
    fs::path videosHtmlFile    = archiveBoxRootDirectory / "videos.html";
    fs::path videosDirectory   = archiveBoxRootDirectory / "videos";
    fs::path channelsDirectory = archiveBoxRootDirectory / "channels";

    if (!fs::exists(videosDirectory))   fs::create_directories(videosDirectory);
    if (!fs::exists(channelsDirectory)) fs::create_directories(channelsDirectory);

    // Generate per-channel HTML files
    for (const auto& c : channels) {
        std::string html = createChannelHtml(
            std::optional<std::string>(c),
            channels,
            argsInstance,
            channelUrls,
            youtubeVideos,
            archiveBoxRootDirectory,
            videosDirectory,
            archiveBoxArchiveDirectory
        );

        const std::string& url = channelUrls.at(c);
        const std::string needle = "/channel/";
        auto pos = url.find(needle);
        std::string channelId = (pos == std::string::npos)
                                ? c
                                : url.substr(pos + needle.size());

        Utils::writeTextToFile(html, channelsDirectory / (channelId + ".html"));
    }

    // Generate master list (wantedChannelName = null)
    {
        std::string html = createChannelHtml(
            std::nullopt,
            channels,
            argsInstance,
            channelUrls,
            youtubeVideos,
            archiveBoxRootDirectory,
            videosDirectory,
            archiveBoxArchiveDirectory
        );
        Utils::writeTextToFile(html, videosHtmlFile);
    }

    // Print warnings and statistics
    std::cout << "[Warning] Snapshots without videos:\n";
    for (const auto& s : YoutubeVideo::missingYoutubeVideos)
        std::cout << s << "\n";

    std::cout << "Total duration: "
              << static_cast<int>(
                     (static_cast<double>(YoutubeVideo::totalDurationInMilliseconds)
                      / 1000.0 / 60.0 / 60.0))
              << " hours\n";

    // Sort by duration
    {
        auto sortedByDuration = youtubeVideos;
        std::sort(sortedByDuration.begin(), sortedByDuration.end(),
                  [](const YoutubeVideo& a, const YoutubeVideo& b) {
                      return a.getVideoDurationInMilliseconds()
                           < b.getVideoDurationInMilliseconds();
                  });

        for (auto& y : sortedByDuration) {
            std::cout << y.getVideoDurationInMinutes() << " minutes\t"
                      << "https://youtube.com/watch?v=" << y.id
                      << "\t" << y.title << "\n";
        }
    }

    std::cout << "\n\n\n\n";

    // Sort by file size
    {
        auto sortedBySize = youtubeVideos;
        std::sort(sortedBySize.begin(), sortedBySize.end(),
                  [](const YoutubeVideo& a, const YoutubeVideo& b) {
                      return a.videoFileSizeInBytes
                           < b.videoFileSizeInBytes;
                  });

        for (auto& y : sortedBySize) {
            std::cout << y.getVideoFileSizeInMegaBytes() << " MB\t"
                      << "https://youtube.com/watch?v=" << y.id
                      << "\t" << y.title << "\n";
        }
    }

    return 0;
}

// ------------------- createChannelHtml -----------------------
static std::string createChannelHtml(
    const std::optional<std::string>& wantedChannelName,
    const std::vector<std::string>& channels,
    const Args& argsInstance,
    const std::map<std::string, std::string>& channelUrls,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& archiveBoxArchiveDirectory
) {
    std::ostringstream out;

    out << R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<link rel="icon" type="image/x-icon" href="favicon.ico" sizes="16x16">
<title>Youtube videos</title>
<!-- Generated by: https://code.openeggbert.org/openeggbert/youtubedl-frontend -->
<style>
body {padding:20px;}
* { font-family:Arial; }
.videos { }
.box { padding:10px; }
</style>
</head>
<body>
)";

    for (const auto& channel : channels) {
        if (wantedChannelName && *wantedChannelName != channel)
            continue;

        out << "<h1>" << channel << "</h1>\n";
        out << "<div style=\"max-width:"
            << (THUMBNAIL_WIDTH + 20) * argsInstance.getInt(ArgType::VIDEOS_PER_ROW).value_or(4)
            << "px\">";

        const std::string& url = channelUrls.at(channel);
        const std::string needle = "/channel/";
        auto pos = url.find(needle);
        std::string channelId = (pos == std::string::npos)
                                ? channel
                                : url.substr(pos + needle.size());

        out << "<a target=\"_blank\" href=\"channels/" << channelId << ".html\">Videos</a>";
        out << "&nbsp;&nbsp;&nbsp;( <a href=\"" << url << "\">" << url << "</a> )";

        if (wantedChannelName) {
            // ✅ This is the line that must be present (it was missing earlier)
            out << "<div class=\"videos\">";

            const int vpr = argsInstance.getInt(ArgType::VIDEOS_PER_ROW).value_or(4);

            iii = 0;
            internalStaticVariableVideoNumberPerRow = 0;
            out << "<table>\n";

            long countOfVideosInChannel =
                std::count_if(youtubeVideos.begin(), youtubeVideos.end(),
                              [&](const YoutubeVideo& v) {
                                  return v.channelName == channel;
                              });

            for (auto& youtubeVideo : youtubeVideos) {
                if (youtubeVideo.channelName != channel)
                    continue;

                ++iii;
                if (internalStaticVariableVideoNumberPerRow == 0)
                    out << "<tr>";

                ++internalStaticVariableVideoNumberPerRow;

                out << "<td><div class=\"box\"><table style=\"margin:5px;max-width:"
                    << THUMBNAIL_WIDTH << "px;\">\n<tr><td><a href=\"";

                // Thumbnail link target
                if (argsInstance.getBool(ArgType::THUMBNAIL_LINKS_TO_YOUTUBE))
                    out << "https://www.youtube.com/watch?v=" << youtubeVideo.id;
                else
                    out << "../videos/" << youtubeVideo.id << ".html";

                out << "\" target=\"_blank\"><img src=\"";

                // Thumbnail source
                std::string thumbnailPath =
                    "archive/" + youtubeVideo.snapshot
                    + "/media/mini-thumbnail."
                    + youtubeVideo.getMiniThumbnailFormat();

                if (argsInstance.getBool(ArgType::THUMBNAIL_AS_BASE64)) {
                    try {
                        fs::path filePath = archiveBoxRootDirectory / thumbnailPath;
                        std::vector<uint8_t> bytes;

                        std::cerr << "###=" << filePath.string() << "\n";

                        try {
                            bytes = Utils::resizeImage(filePath,
                                                       25,
                                                       static_cast<int>(9.0 / 16.0 * 25.0),
                                                       youtubeVideo.getThumbnailFormat());
                        } catch (...) {}

                        std::string encoded = encode_base64(bytes);
                        out << "data:image/jpg;base64," << encoded;
                    } catch (const std::exception& ex) {
                        throw std::runtime_error(ex.what());
                    }
                } else {
                    out << "../" << thumbnailPath;
                }

                out << "\" width=\"" << THUMBNAIL_WIDTH << "\"></a></td></tr>\n";

                // Title
                out << "<tr><td><b style=\"font-size:90%;\">"
                    << youtubeVideo.title
                    << "</b></td></tr>\n";

                // Upload date formatted yyyy-mm-dd
                std::string uploadDate = youtubeVideo.uploadDate;
                if (uploadDate.size() >= 8) {
                    uploadDate = uploadDate.substr(0,4) + "-" + uploadDate.substr(4,2) + "-" + uploadDate.substr(6,2);
                }

                out << "<tr><td style=\"font-size:80%;color:grey;\">"
                    << uploadDate << " •︎ "
                    << youtubeVideo.videoDuration
                    << " •︎ #" << iii
                    << "</td></tr>\n";

                youtubeVideo.number = iii;

                out << "</table></div></td>\n";

                if (internalStaticVariableVideoNumberPerRow == vpr) {
                    out << "<tr>";
                    internalStaticVariableVideoNumberPerRow = 0;
                }

                fs::path videoHtmlFile =
                    videosDirectory / (youtubeVideo.id + ".html");

                if (!fs::exists(videoHtmlFile) ||
                    argsInstance.getBool(ArgType::ALWAYS_GENERATE_HTML_FILES)) {

                    std::string singleVideo =
                        YoutubeVideoHtml(youtubeVideo,
                                         archiveBoxRootDirectory,
                                         archiveBoxArchiveDirectory,
                                         countOfVideosInChannel).toString();

                    Utils::writeTextToFile(singleVideo, videoHtmlFile);
                    ++processedVideos;

                    // Directory size equivalent to archiveBoxArchiveDirectory.list().length
                    size_t archiveCount = 0;
                    for (auto it = fs::directory_iterator(archiveBoxArchiveDirectory);
                         it != fs::directory_iterator(); ++it)
                        ++archiveCount;

                    std::cout << "Processed " << processedVideos
                              << " from " << archiveCount << "\n";
                }
            }

            if (internalStaticVariableVideoNumberPerRow < vpr) {
                out << "<tr>";
            }


            out << "</table>\n";
            out << "</div>"; // </div .videos>
        }

        out << "</div>"; // wrapper with max-width
    }

    out << "</body></html>";
    return out.str();
}



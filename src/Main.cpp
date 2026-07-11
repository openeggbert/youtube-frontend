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

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
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
#include "AssetsStyle.h"
#include "YoutubedlFrontendException.h"

namespace fs = std::filesystem;

// Forward declaration
static std::string createChannelHtml(
    const std::optional<std::string>& wantedChannelName,
    const std::vector<std::string>& channels,
    const Args& argsInstance,
    const std::map<std::string, std::string>& channelUrls,
    const std::map<std::string, std::string>& channelIds,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& archiveBoxArchiveDirectory,
    int& processedVideos);

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

// Returns the "image/..." MIME type for a thumbnail file extension.
static std::string mimeTypeForImageFormat(const std::string& format) {
    if (format == "png") return "image/png";
    if (format == "webp") return "image/webp";
    if (format == "gif") return "image/gif";
    return "image/jpeg";
}

static void printUsage() {
    std::cout << "youtube-frontend - static HTML generator for a yt-dlp/ArchiveBox video archive\n\n";
    std::cout << "Usage:\n";
    std::cout << "  youtube_frontend <archive-root-directory> [options]\n\n";
    std::cout << "Options:\n";
    for (auto type : get_arg_type_values()) {
        std::ostringstream flag;
        flag << "--" << get_name(type);
        std::cout << "  " << std::left << std::setw(30) << flag.str() << get_description(type);
        std::string def = get_default_value(type);
        if (!def.empty())
            std::cout << " (default: " << def << ")";
        std::cout << "\n";
    }
    std::cout << "  " << std::left << std::setw(30) << "-h, --help" << "Show this help and exit\n";
    std::cout << "\nExample:\n";
    std::cout << "  youtube_frontend /path/to/archivebox --videos-per-row 3 --thumbnail-as-base64 1\n";
}

// ------------------- MAIN -----------------------
int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    for (const auto& a : args) {
        if (a == "-h" || a == "--help") {
            printUsage();
            return 0;
        }
    }

    if (args.empty()) {
        printUsage();
        return 1;
    }

    std::cout << "youtube-frontend - HTML generator\n\n";

    try {

    Args argsInstance(args);
    std::cout << argsInstance.to_string() << "\n";

    std::string workingDirectory;
    if (!args.empty() && args[0].rfind(Args::TWO_DASHES, 0) != 0)
        workingDirectory = args[0];
    else
        workingDirectory = fs::absolute(".").string();

    fs::path archiveBoxRootDirectory = workingDirectory;
    fs::path archiveBoxArchiveDirectory = archiveBoxRootDirectory / "archive";

    std::vector<YoutubeVideo> youtubeVideos =
        YoutubeVideo::loadYoutubeVideos(archiveBoxArchiveDirectory, argsInstance);

    std::map<std::string, std::string> channelUrls;
    std::map<std::string, std::string> channelIds;
    std::vector<std::string> channels;

    // Build channel name → channel URL/id maps and the list of unique channels.
    // Videos with no channel metadata were already grouped under
    // YoutubeVideo::UNCATEGORIZED_CHANNEL_NAME by loadYoutubeVideos.
    for (auto& v : youtubeVideos) {
        const std::string& channelName = v.channelName;
        if (channelName.empty() || channelUrls.contains(channelName))
            continue;

        channelUrls[channelName] = v.channelUrl;

        if (channelName == YoutubeVideo::UNCATEGORIZED_CHANNEL_NAME) {
            channelIds[channelName] = YoutubeVideo::UNCATEGORIZED_CHANNEL_ID;
        } else {
            const std::string needle = "/channel/";
            auto pos = v.channelUrl.find(needle);
            channelIds[channelName] = (pos == std::string::npos)
                                          ? channelName
                                          : v.channelUrl.substr(pos + needle.size());
        }

        channels.push_back(channelName);
    }

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
    fs::path assetsDirectory   = archiveBoxRootDirectory / "assets";

    if (!fs::exists(videosDirectory))   fs::create_directories(videosDirectory);
    if (!fs::exists(channelsDirectory)) fs::create_directories(channelsDirectory);
    if (!fs::exists(assetsDirectory))   fs::create_directories(assetsDirectory);

    // Shared stylesheet used by every generated page.
    Utils::writeTextToFile(Assets::STYLE_CSS, assetsDirectory / "style.css");

    int processedVideos = 0;

    // Generate per-channel HTML files
    for (const auto& c : channels) {
        std::string html = createChannelHtml(
            std::optional<std::string>(c),
            channels,
            argsInstance,
            channelUrls,
            channelIds,
            youtubeVideos,
            archiveBoxRootDirectory,
            videosDirectory,
            archiveBoxArchiveDirectory,
            processedVideos
        );

        Utils::writeTextToFile(html, channelsDirectory / (channelIds.at(c) + ".html"));
    }

    // Generate master list (wantedChannelName = null)
    {
        std::string html = createChannelHtml(
            std::nullopt,
            channels,
            argsInstance,
            channelUrls,
            channelIds,
            youtubeVideos,
            archiveBoxRootDirectory,
            videosDirectory,
            archiveBoxArchiveDirectory,
            processedVideos
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

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

// ------------------- createChannelHtml -----------------------
static std::string createChannelHtml(
    const std::optional<std::string>& wantedChannelName,
    const std::vector<std::string>& channels,
    const Args& argsInstance,
    const std::map<std::string, std::string>& channelUrls,
    const std::map<std::string, std::string>& channelIds,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& archiveBoxArchiveDirectory,
    int& processedVideos
) {
    std::ostringstream out;

    // Channel subpages live one directory below the archive root (channels/x.html),
    // the master list lives at the root (videos.html) - every relative link/asset
    // href must account for that difference.
    const std::string basePrefix = wantedChannelName ? "../" : "";

    out << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" type="image/x-icon" href=")" << basePrefix << R"(favicon.ico" sizes="16x16">
<link rel="stylesheet" href=")" << basePrefix << R"(assets/style.css">
<title>Youtube videos</title>
<!-- Generated by: https://code.openeggbert.org/openeggbert/youtubedl-frontend -->
</head>
<body>
<header class="site-header"><div class="inner">
<a class="brand" href=")" << basePrefix << R"(videos.html"><span class="dot"></span>Youtube archive</a>
</div></header>
<main class="page">
)";

    if (wantedChannelName)
        out << "<p class=\"crumb\"><a href=\"" << basePrefix << "videos.html\">← all channels</a></p>\n";

    for (const auto& channel : channels) {
        if (wantedChannelName && *wantedChannelName != channel)
            continue;

        const std::string& url = channelUrls.at(channel);
        const std::string& channelId = channelIds.at(channel);

        long countOfVideosInChannel =
            std::count_if(youtubeVideos.begin(), youtubeVideos.end(),
                          [&](const YoutubeVideo& v) {
                              return v.channelName == channel;
                          });

        out << "<section class=\"channel-block\">\n";
        out << "<div class=\"channel-block-head\">";
        out << "<div><h1 class=\"page-title\">" << Utils::escapeHtml(channel) << "</h1>"
            << "<span class=\"count\">" << countOfVideosInChannel << " videos</span></div>";
        out << "<div class=\"channel-links\">";
        out << "<a class=\"pill\" href=\"" << basePrefix << "channels/" << channelId << ".html\">View videos</a>";
        if (!url.empty())
            out << "<a class=\"pill ghost\" target=\"_blank\" rel=\"noopener\" href=\"" << url << "\">Channel on YouTube ↗</a>";
        out << "</div></div>\n";

        if (wantedChannelName) {
            const int vpr = argsInstance.getInt(ArgType::VIDEOS_PER_ROW).value_or(4);

            out << "<div class=\"grid\" style=\"max-width:" << ((THUMBNAIL_WIDTH - 12) * vpr + 18 * (vpr - 1)) << "px\">\n";

            // Count archive snapshot directories once per channel instead of
            // rescanning the whole directory tree for every single video.
            size_t archiveCount = 0;
            for (auto it = fs::directory_iterator(archiveBoxArchiveDirectory);
                 it != fs::directory_iterator(); ++it)
                ++archiveCount;

            int iii = 0;
            for (auto& youtubeVideo : youtubeVideos) {
                if (youtubeVideo.channelName != channel)
                    continue;

                ++iii;
                youtubeVideo.number = iii;

                bool linksToYoutube = argsInstance.getBool(ArgType::THUMBNAIL_LINKS_TO_YOUTUBE);

                out << "<a class=\"card\" href=\"";
                if (linksToYoutube)
                    out << "https://www.youtube.com/watch?v=" << youtubeVideo.id;
                else
                    out << "../videos/" << youtubeVideo.id << ".html";
                out << "\"" << (linksToYoutube ? " target=\"_blank\" rel=\"noopener\"" : "") << ">";

                out << "<div class=\"thumb-wrap\">";
                out << "<img loading=\"lazy\" alt=\"\" src=\"";

                std::string thumbnailPath =
                    "archive/" + youtubeVideo.snapshot
                    + "/media/mini-thumbnail."
                    + youtubeVideo.getMiniThumbnailFormat();

                if (argsInstance.getBool(ArgType::THUMBNAIL_AS_BASE64)) {
                    fs::path filePath = archiveBoxRootDirectory / thumbnailPath;
                    std::vector<uint8_t> bytes;
                    bool resized = true;

                    try {
                        bytes = Utils::resizeImage(filePath,
                                                   25,
                                                   static_cast<int>(9.0 / 16.0 * 25.0),
                                                   youtubeVideo.getThumbnailFormat());
                    } catch (const std::exception& ex) {
                        resized = false;
                        std::cerr << "[Warning] Could not create embedded thumbnail for "
                                  << youtubeVideo.id << ": " << ex.what() << "\n";
                    }

                    if (resized && !bytes.empty()) {
                        std::string encoded = encode_base64(bytes);
                        out << "data:" << mimeTypeForImageFormat(youtubeVideo.getThumbnailFormat())
                            << ";base64," << encoded;
                    } else {
                        // Fall back to a direct file link rather than emitting a broken image.
                        out << basePrefix << thumbnailPath;
                    }
                } else {
                    out << basePrefix << thumbnailPath;
                }

                out << "\">";
                out << "<span class=\"idx-chip\">#" << iii << "</span>";
                out << "<span class=\"dur-chip\">" << Utils::formatDurationShort(youtubeVideo.videoDuration) << "</span>";
                out << "</div>\n";

                std::string uploadDate = youtubeVideo.uploadDate;
                if (uploadDate.size() >= 8)
                    uploadDate = uploadDate.substr(0, 4) + "-" + uploadDate.substr(4, 2) + "-" + uploadDate.substr(6, 2);

                double mb = (double)youtubeVideo.videoFileSizeInBytes / 1024.0 / 1024.0;

                out << "<div class=\"card-body\">";
                out << "<div class=\"card-title\">" << Utils::escapeHtml(youtubeVideo.title) << "</div>";
                out << "<div class=\"card-meta\"><span>" << Utils::escapeHtml(uploadDate) << "</span>"
                    << "<span>" << std::fixed << std::setprecision(0) << mb << " MB</span></div>";
                out << "</div>\n";

                out << "</a>\n";

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

                    std::cout << "Processed " << processedVideos
                              << " from " << archiveCount << "\n";
                }
            }

            out << "</div>\n"; // .grid
        }

        out << "</section>\n";
    }

    out << "</main>\n</body></html>";
    return out.str();
}

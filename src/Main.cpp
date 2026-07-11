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
#include "ThreadPool.h"

namespace fs = std::filesystem;

// Forward declarations
static std::string createChannelIndexHtml(
    const std::vector<std::string>& channels,
    const std::map<std::string, std::string>& channelUrls,
    const std::map<std::string, std::string>& channelIds,
    const std::vector<YoutubeVideo>& youtubeVideos);

static void generateChannelPages(
    const std::string& channelName,
    const std::string& channelId,
    const std::string& channelUrl,
    const Args& argsInstance,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& channelsDirectory,
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

// Resolves the <img> src for a video's grid thumbnail: either a relative
// file path, or (with --thumbnail-as-base64) a small embedded data URI. The
// encoded data URI is cached next to the source thumbnail (<file>.b64) so
// repeat runs don't redo the OpenCV resize + base64 encode every time;
// forceRegenerate (tied to --always-generate-metadata) bypasses the cache.
static std::string buildThumbnailSrc(
    const YoutubeVideo& youtubeVideo,
    const fs::path& archiveBoxRootDirectory,
    const std::string& thumbnailPath,
    const std::string& basePrefix,
    bool asBase64,
    bool forceRegenerate
) {
    if (!asBase64)
        return basePrefix + thumbnailPath;

    fs::path filePath = archiveBoxRootDirectory / thumbnailPath;
    fs::path cacheFile = filePath;
    cacheFile += ".b64";

    if (!forceRegenerate && fs::exists(cacheFile)) {
        std::string cached = Utils::readTextFromFile(cacheFile);
        if (!cached.empty())
            return cached;
    }

    try {
        std::vector<uint8_t> bytes = Utils::resizeImage(filePath,
                                                         25,
                                                         static_cast<int>(9.0 / 16.0 * 25.0),
                                                         youtubeVideo.getThumbnailFormat());
        if (bytes.empty())
            return basePrefix + thumbnailPath;

        std::string dataUri = "data:" + mimeTypeForImageFormat(youtubeVideo.getThumbnailFormat())
                             + ";base64," + encode_base64(bytes);
        Utils::writeTextToFile(dataUri, cacheFile);
        return dataUri;
    } catch (const std::exception& ex) {
        std::cerr << "[Warning] Could not create embedded thumbnail for "
                  << youtubeVideo.id << ": " << ex.what() << "\n";
        return basePrefix + thumbnailPath;
    }
}

// Opening boilerplate (doctype/head/site header) shared by every generated
// page. basePrefix is "" for pages at the archive root (videos.html) and
// "../" for pages one directory down (channels/*.html, videos/*.html).
static std::string pageShellOpen(const std::string& basePrefix) {
    std::ostringstream out;
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
    return out.str();
}

// The channel title + video count + "View videos"/"Channel on YouTube"
// links, shared by the master index and every page of a channel.
static std::string channelHeadHtml(
    const std::string& channelName,
    const std::string& channelId,
    const std::string& channelUrl,
    const std::string& basePrefix,
    long countOfVideosInChannel
) {
    std::ostringstream out;
    out << "<div class=\"channel-block-head\">";
    out << "<div><h1 class=\"page-title\">" << Utils::escapeHtml(channelName) << "</h1>"
        << "<span class=\"count\">" << countOfVideosInChannel << " videos</span></div>";
    out << "<div class=\"channel-links\">";
    out << "<a class=\"pill\" href=\"" << basePrefix << "channels/" << channelId << ".html\">View videos</a>";
    if (!channelUrl.empty())
        out << "<a class=\"pill ghost\" target=\"_blank\" rel=\"noopener\" href=\"" << channelUrl << "\">Channel on YouTube ↗</a>";
    out << "</div></div>\n";
    return out.str();
}

// Prev/next pager shown on a channel page when it has been split across
// multiple files. Page 1 is always "<channelId>.html", later pages are
// "<channelId>-page<N>.html" - all siblings in the same channels/ directory.
static std::string pagerHtml(const std::string& channelId, int page, int totalPages) {
    auto fileFor = [&](int p) {
        return p <= 1 ? (channelId + ".html") : (channelId + "-page" + std::to_string(p) + ".html");
    };

    std::ostringstream out;
    out << "<div class=\"nav-row\">";
    if (page > 1)
        out << "<a class=\"pill\" href=\"" << fileFor(page - 1) << "\">← Previous</a>";
    else
        out << "<span class=\"pill disabled\">← Previous</span>";
    out << "<span class=\"n\">Page " << page << " / " << totalPages << "</span>";
    if (page < totalPages)
        out << "<a class=\"pill primary\" href=\"" << fileFor(page + 1) << "\">Next →</a>";
    else
        out << "<span class=\"pill primary disabled\">Next →</span>";
    out << "</div>\n";
    return out.str();
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

    // Generate per-channel HTML page(s) - one or more per channel once it
    // exceeds --videos-per-page.
    for (const auto& c : channels) {
        generateChannelPages(
            c,
            channelIds.at(c),
            channelUrls.at(c),
            argsInstance,
            youtubeVideos,
            archiveBoxRootDirectory,
            videosDirectory,
            channelsDirectory,
            archiveBoxArchiveDirectory,
            processedVideos
        );
    }

    // Generate master list (index of all channels)
    {
        std::string html = createChannelIndexHtml(channels, channelUrls, channelIds, youtubeVideos);
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

// ------------------- createChannelIndexHtml -----------------------
// The top-level videos.html: one section per channel with just the title,
// video count and links - no video grid (that lives on the channel's own
// paginated pages, see generateChannelPages below).
static std::string createChannelIndexHtml(
    const std::vector<std::string>& channels,
    const std::map<std::string, std::string>& channelUrls,
    const std::map<std::string, std::string>& channelIds,
    const std::vector<YoutubeVideo>& youtubeVideos
) {
    std::ostringstream out;
    out << pageShellOpen("");

    for (const auto& channel : channels) {
        long countOfVideosInChannel =
            std::count_if(youtubeVideos.begin(), youtubeVideos.end(),
                          [&](const YoutubeVideo& v) {
                              return v.channelName == channel;
                          });

        out << "<section class=\"channel-block\">\n";
        out << channelHeadHtml(channel, channelIds.at(channel), channelUrls.at(channel), "", countOfVideosInChannel);
        out << "</section>\n";
    }

    out << "</main>\n</body></html>";
    return out.str();
}

// ------------------- generateChannelPages -----------------------
// Renders and writes one channel's video grid, split across as many
// channels/<id>[-page<N>].html files as needed to keep each page at or
// under --videos-per-page. Also generates each video's own videos/<id>.html
// page along the way (exactly once per video, regardless of pagination).
static void generateChannelPages(
    const std::string& channelName,
    const std::string& channelId,
    const std::string& channelUrl,
    const Args& argsInstance,
    std::vector<YoutubeVideo>& youtubeVideos,
    const fs::path& archiveBoxRootDirectory,
    const fs::path& videosDirectory,
    const fs::path& channelsDirectory,
    const fs::path& archiveBoxArchiveDirectory,
    int& processedVideos
) {
    std::vector<YoutubeVideo*> channelVideos;
    for (auto& v : youtubeVideos)
        if (v.channelName == channelName)
            channelVideos.push_back(&v);

    const long countOfVideosInChannel = static_cast<long>(channelVideos.size());

    // Numbering stays global across pages so Back/Next on each video's own
    // page keeps working the same regardless of which grid page it's on.
    for (size_t idx = 0; idx < channelVideos.size(); ++idx)
        channelVideos[idx]->number = static_cast<int>(idx + 1);

    const int vpr = argsInstance.getInt(ArgType::VIDEOS_PER_ROW).value_or(4);
    const int videosPerPage = std::max(1, argsInstance.getInt(ArgType::VIDEOS_PER_PAGE).value_or(60));
    const int totalPages = std::max(1, static_cast<int>(
        (channelVideos.size() + static_cast<size_t>(videosPerPage) - 1) / static_cast<size_t>(videosPerPage)));

    const std::string basePrefix = "../";
    const bool linksToYoutube = argsInstance.getBool(ArgType::THUMBNAIL_LINKS_TO_YOUTUBE);
    const bool asBase64 = argsInstance.getBool(ArgType::THUMBNAIL_AS_BASE64);
    const bool forceRegenerateThumbCache = argsInstance.getBool(ArgType::ALWAYS_GENERATE_METADATA);
    const bool alwaysGenerateHtmlFiles = argsInstance.getBool(ArgType::ALWAYS_GENERATE_HTML_FILES);

    // Count archive snapshot directories once for the whole channel instead
    // of rescanning the whole directory tree for every single video.
    size_t archiveCount = 0;
    for (auto it = fs::directory_iterator(archiveBoxArchiveDirectory);
         it != fs::directory_iterator(); ++it)
        ++archiveCount;

    // One pool, reused across all of this channel's pages, encodes
    // thumbnails for each page's batch of videos concurrently instead of
    // one at a time in the main generation loop.
    std::optional<ThreadPool> thumbPool;
    if (asBase64)
        thumbPool.emplace(std::max(2u, std::thread::hardware_concurrency()));

    for (int page = 1; page <= totalPages; ++page) {
        size_t startIdx = static_cast<size_t>(page - 1) * static_cast<size_t>(videosPerPage);
        size_t endIdx = std::min(startIdx + static_cast<size_t>(videosPerPage), channelVideos.size());

        // Resolve every card's thumbnail src up front (in parallel when
        // base64 embedding is on) so the HTML-building pass below stays a
        // straight, in-order loop.
        std::vector<std::string> thumbPaths(endIdx - startIdx);
        for (size_t idx = startIdx; idx < endIdx; ++idx) {
            YoutubeVideo* v = channelVideos[idx];
            thumbPaths[idx - startIdx] =
                "archive/" + v->snapshot + "/media/mini-thumbnail." + v->getMiniThumbnailFormat();
        }

        std::vector<std::string> thumbSrcs(endIdx - startIdx);
        if (asBase64) {
            std::vector<std::future<std::string>> futures;
            futures.reserve(endIdx - startIdx);
            for (size_t idx = startIdx; idx < endIdx; ++idx) {
                YoutubeVideo* v = channelVideos[idx];
                const std::string thumbnailPath = thumbPaths[idx - startIdx];
                futures.push_back(thumbPool->enqueue(
                    [v, &archiveBoxRootDirectory, thumbnailPath, &basePrefix, forceRegenerateThumbCache] {
                        return buildThumbnailSrc(*v, archiveBoxRootDirectory, thumbnailPath, basePrefix,
                                                  true, forceRegenerateThumbCache);
                    }));
            }
            for (size_t i = 0; i < futures.size(); ++i)
                thumbSrcs[i] = futures[i].get();
        } else {
            for (size_t idx = startIdx; idx < endIdx; ++idx)
                thumbSrcs[idx - startIdx] = basePrefix + thumbPaths[idx - startIdx];
        }

        std::ostringstream out;
        out << pageShellOpen(basePrefix);
        out << "<p class=\"crumb\"><a href=\"" << basePrefix << "videos.html\">← all channels</a></p>\n";
        out << "<section class=\"channel-block\">\n";
        out << channelHeadHtml(channelName, channelId, channelUrl, basePrefix, countOfVideosInChannel);

        out << "<div class=\"grid\" style=\"max-width:" << ((THUMBNAIL_WIDTH - 12) * vpr + 18 * (vpr - 1)) << "px\">\n";

        for (size_t idx = startIdx; idx < endIdx; ++idx) {
            YoutubeVideo& youtubeVideo = *channelVideos[idx];

            out << "<a class=\"card\" href=\"";
            if (linksToYoutube)
                out << "https://www.youtube.com/watch?v=" << youtubeVideo.id;
            else
                out << "../videos/" << youtubeVideo.id << ".html";
            out << "\"" << (linksToYoutube ? " target=\"_blank\" rel=\"noopener\"" : "") << ">";

            out << "<div class=\"thumb-wrap\">";
            out << "<img loading=\"lazy\" alt=\"\" src=\"" << thumbSrcs[idx - startIdx] << "\">";
            out << "<span class=\"idx-chip\">#" << youtubeVideo.number << "</span>";
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

            fs::path videoHtmlFile = videosDirectory / (youtubeVideo.id + ".html");
            if (!fs::exists(videoHtmlFile) || alwaysGenerateHtmlFiles) {
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

        if (totalPages > 1)
            out << pagerHtml(channelId, page, totalPages);

        out << "</section>\n";
        out << "</main>\n</body></html>";

        std::string fileName = (page == 1) ? (channelId + ".html") : (channelId + "-page" + std::to_string(page) + ".html");
        Utils::writeTextToFile(out.str(), channelsDirectory / fileName);
    }
}

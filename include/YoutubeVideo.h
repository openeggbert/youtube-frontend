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

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <cstdint>
#include <map>

#include "Args.h"
#include "ArgType.h"
#include "YoutubeComment.h"

class YoutubeVideo
{
public:
    std::string id;
    std::string snapshot;
    std::string title;
    std::string videoFileName;
    long videoFileSizeInBytes;
    std::string videoFileSha512HashSum;
    std::string videoDuration;
    std::string channelName;
    std::string channelUrl;
    std::string channelId;
    std::string uploadDate;
    long timestamp;
    std::string description;
    std::string thumbnail;
    std::string miniThumbnail;
    std::vector<YoutubeComment> comments;
    std::string previousVideoId;
    std::string nextVideoId;
    std::string ext;
    int number;

    static std::vector<std::string> missingYoutubeVideos;
    static long totalDurationInMilliseconds;

public:
    YoutubeVideo();

    YoutubeVideo(
        const std::filesystem::path& mediaDirectory,
        bool alwaysGenerateMetadata,
        const std::string& argVideo
    );

    long getVideoDurationInMilliseconds() const;
    long getVideoDurationInMinutes() const;
    long getVideoFileSizeInMegaBytes() const;

    [[nodiscard]] std::string getThumbnailFormat() const;
    [[nodiscard]] std::string getMiniThumbnailFormat() const;

    bool operator<(const YoutubeVideo& other) const;

    // load static
    static std::vector<YoutubeVideo> loadYoutubeVideos(
        const std::filesystem::path& archiveBoxArchiveDirectory,
        const Args& argsInstance
    );

private:
    [[nodiscard]] std::string getExtensionFromUrl(const std::string& url) const;
    static std::string formatTimeStamp(long duration);

    static std::string getVideoFormattedDuration(const std::string& filePath);
};

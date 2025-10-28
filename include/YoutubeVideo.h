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

/**
 *
 * @author robertvokac
 */
#ifndef YOUTUBEVIDEO_H
#define YOUTUBEVIDEO_H


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
    // Default constructor
    YoutubeVideo();

    // Java equivalent constructor: YoutubeVideo(File mediaDirectory, boolean alwaysGen, String argVideo)
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

#endif // YOUTUBEVIDEO_H

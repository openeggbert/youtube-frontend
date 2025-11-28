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

#include "YoutubeVideo.h"
#include "Utils.h"
#include "YoutubedlFrontendException.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "Constants.h"

extern "C" {
#include <libavformat/avformat.h>
    }

using json = nlohmann::json;
namespace fs = std::filesystem;

std::vector<std::string> YoutubeVideo::missingYoutubeVideos;
long YoutubeVideo::totalDurationInMilliseconds = 0;

YoutubeVideo::YoutubeVideo()
    : videoFileSizeInBytes(0),
      timestamp(0),
      number(0)
{
}


YoutubeVideo::YoutubeVideo(
    const fs::path& mediaDirectory,
    bool alwaysGenerateMetadata,
    const std::string& argVideo
) : videoFileSizeInBytes(0),
    timestamp(0),
    number(0)
{
    fs::path metadataFile = mediaDirectory / "metadata";

    // ----------- CASE 1: Read from metadata file ----------
    if (!alwaysGenerateMetadata && fs::exists(metadataFile))
    {
        std::ifstream ifs(metadataFile);
        if (!ifs)
            throw std::runtime_error("Cannot open metadata file");

        std::map<std::string, std::string> props;
        std::string line;
        while (std::getline(ifs, line))
        {
            auto pos = line.find('=');
            if (pos != std::string::npos)
            {
                props[line.substr(0, pos)] = line.substr(pos + 1);
            }
        }

        id = props["id"];
        if (!argVideo.empty() && id != argVideo)
            return;

        snapshot = props["snapshot"];
        title = props["title"];
        videoFileName = props["videoFileName"];
        videoFileSizeInBytes = std::stoll(props["videoFileSizeInBytes"]);
        videoFileSha512HashSum = props["videoFileSha512HashSum"];
        videoDuration = props["videoDuration"];
        channelName = props["channelName"];
        channelUrl = props["channelUrl"];
        channelId = props["channelId"];
        uploadDate = props["uploadDate"];
        timestamp = std::stoll(props["timestamp"]);
        description = props["description"];
        thumbnail = props["thumbnail"];
        miniThumbnail = props["miniThumbnail"];

        // Comments JSON
        comments.clear();
        if (props.count("comments"))
        {
            try
            {
                json arr = json::parse(props["comments"]);
                for (auto& elem : arr)
                {
                    YoutubeComment yc(elem);
                    comments.push_back(yc);
                }
            }
            catch (...)
            {
                throw std::runtime_error("Error parsing comments JSON");
            }
        }

        previousVideoId = props.count("previousVideoId") ? props["previousVideoId"] : "";
        nextVideoId = props.count("nextVideoId") ? props["nextVideoId"] : "";
        ext = props["ext"];
        number = std::stoi(props["number"]);

        return;
    }

    // ----------- CASE 2: Parse mediaDirectory JSON + video info ----------
    std::vector<fs::path> files;
    for (auto& p : fs::directory_iterator(mediaDirectory))
        files.push_back(p.path());

    // Find JSON file in directory
    fs::path jsonFile;
    for (auto& f : files)
    {
        if (f.extension() == ".json")
        {
            jsonFile = f;
            break;
        }
    }

    std::string jsonTxt = jsonFile.empty() ? "" : Utils::readTextFromFile(jsonFile);
    json jsonObject = json::parse(jsonTxt);

    id = jsonObject.value("id", "");

    thumbnail = jsonObject.value("thumbnail", "");
    auto thumbnails = jsonObject["thumbnails"];

    for (auto& t : thumbnails)
    {
        if (t.contains("width"))
        {
            int width = t["width"];
            if (width < static_cast<int>(THUMBNAIL_WIDTH * 0.8))
                continue;
            miniThumbnail = t["url"];
            break;
        }
    }

    fs::path thumbnailFile = mediaDirectory / ("thumbnail." + getThumbnailFormat());
    fs::path miniThumbnailFile = mediaDirectory / ("mini-thumbnail." + getMiniThumbnailFormat());

    // Download thumbnails if missing
    if (!thumbnail.empty())
    {
        if (!fs::exists(thumbnailFile))
        {
            Utils::downloadFile(thumbnail, thumbnailFile);
        }
        if (!fs::exists(miniThumbnailFile) && !miniThumbnail.empty())
        {
            Utils::downloadFile(miniThumbnail, miniThumbnailFile);
        }
    }

    // Find video file (mp4, mkv, webm)
    ext = jsonObject.value("ext", "");
    fs::path videoFile;
    for (auto& f : files)
    {
        std::string name = f.filename().string();
        if (name.ends_with("." + ext) ||
            name.ends_with(".mp4") ||
            name.ends_with(".mkv") ||
            name.ends_with(".webm"))
        {
            videoFile = f;
            break;
        }
    }

    snapshot = mediaDirectory.parent_path().filename().string();

    if (videoFile.empty())
        missingYoutubeVideos.push_back(id);

    // Read .description if present
    fs::path descriptionFile;
    for (auto& f : files)
    {
        if (f.filename().string().ends_with(".description"))
        {
            descriptionFile = f;
            break;
        }
    }
    description = descriptionFile.empty()
                      ? ""
                      : Utils::readTextFromFile(descriptionFile);

    title = jsonObject.value("title", "");

    if (!videoFile.empty() && !videoFile.filename().string().ends_with(".part"))
    {
        videoFileName = videoFile.filename().string();
        videoFileSizeInBytes = fs::file_size(videoFile);
        videoFileSha512HashSum = Utils::calculateSHA512Hash(videoFile);
        videoDuration = getVideoFormattedDuration(videoFile.string());
    }

    channelName = jsonObject.value("channel", "");
    channelUrl = jsonObject.value("channel_url", "");
    channelId = jsonObject.value("channel_id", "");
    uploadDate = jsonObject.value("upload_date", "");
    timestamp = jsonObject.value("timestamp", 0L);

    // Load comments
    comments.clear();
    if (jsonObject.contains("comments"))
    {
        for (auto& elem : jsonObject["comments"])
        {
            comments.emplace_back(elem);
        }
    }
    comments = YoutubeComment::sort(comments);

    std::ofstream ofs(metadataFile);
    ofs << "id=" << id << "\n";
    ofs << "snapshot=" << snapshot << "\n";
    ofs << "title=" << title << "\n";
    ofs << "videoFileName=" << videoFileName << "\n";
    ofs << "videoFileSizeInBytes=" << videoFileSizeInBytes << "\n";
    ofs << "videoFileSha512HashSum=" << videoFileSha512HashSum << "\n";
    ofs << "videoDuration=" << videoDuration << "\n";
    ofs << "channelName=" << channelName << "\n";
    ofs << "channelUrl=" << channelUrl << "\n";
    ofs << "channelId=" << channelId << "\n";
    ofs << "uploadDate=" << uploadDate << "\n";
    ofs << "timestamp=" << timestamp << "\n";
    ofs << "description=" << description << "\n";
    ofs << "thumbnail=" << thumbnail << "\n";
    ofs << "miniThumbnail=" << miniThumbnail << "\n";

    {
        std::vector<nlohmann::json> comments_json;
        for (auto& comment : comments)
        {
            nlohmann::json comment_json = comment.to_json();
            comments_json.emplace_back(comment_json);
        }

        json jComments = comments_json;
        ofs << "comments=" << jComments.dump() << "\n";
    }

    if (!previousVideoId.empty())
        ofs << "previousVideoId=" << previousVideoId << "\n";
    if (!nextVideoId.empty())
        ofs << "nextVideoId=" << nextVideoId << "\n";

    ofs << "ext=" << ext << "\n";
    ofs << "number=" << number << "\n";
}

long YoutubeVideo::getVideoDurationInMilliseconds() const
{
    std::string d = videoDuration; // "hh:mm:ss.xx"
    auto parts = Utils::split(d, ':');
    if (parts.size() != 3)
        return 0;

    long ms = std::stol(parts[0]) * 60L * 60L * 1000L;
    ms += std::stol(parts[1]) * 60L * 1000L;

    auto secParts = Utils::split(parts[2], '.');
    ms += std::stol(secParts[0]) * 1000L;
    if (secParts.size() > 1)
        ms += std::stol(secParts[1]);
    return ms;
}

long YoutubeVideo::getVideoDurationInMinutes() const
{
    return getVideoDurationInMilliseconds() / 1000L / 60L;
}

long YoutubeVideo::getVideoFileSizeInMegaBytes() const
{
    return videoFileSizeInBytes / 1024L / 1024L;
}

std::string YoutubeVideo::getThumbnailFormat() const
{
    return getExtensionFromUrl(thumbnail);
}

std::string YoutubeVideo::getMiniThumbnailFormat() const
{
    return getExtensionFromUrl(miniThumbnail);
}

std::string YoutubeVideo::getExtensionFromUrl(const std::string& url) const
{
    auto pos = url.find_last_of('.');
    if (pos == std::string::npos)
        return "";
    std::string res = url.substr(pos + 1);
    auto qm = res.find('?');
    if (qm != std::string::npos)
        res = res.substr(0, qm);
    return res;
}

std::string YoutubeVideo::formatTimeStamp(long duration)
{
    // duration is in FFmpeg AV_TIME_BASE units
    // convert to seconds:
    double seconds = duration / static_cast<double>(AV_TIME_BASE);

    int hours = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds - hours * 3600) / 60);
    double rest = seconds - (hours * 3600 + mins * 60);
    int secs = static_cast<int>(rest);
    int subsecs = static_cast<int>((rest - secs) * 100);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%02d", hours, mins, secs, subsecs);
    return std::string(buf);
}

std::string YoutubeVideo::getVideoFormattedDuration(const std::string& filePath)
{
    avformat_network_init();
    AVFormatContext* fmtCtx = nullptr;

    if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0)
        return "00:00:00.00";

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        avformat_close_input(&fmtCtx);
        return "00:00:00.00";
    }

    long duration = fmtCtx->duration; // AV_TIME_BASE units
    avformat_close_input(&fmtCtx);

    return formatTimeStamp(duration);
}

// YoutubeVideo.cpp
bool YoutubeVideo::operator<(const YoutubeVideo& o) const
{
    const bool aHas = !channelName.empty();
    const bool bHas = !o.channelName.empty();

    if (aHas && bHas) {
        if (channelName != o.channelName) return channelName < o.channelName;
        if (uploadDate != o.uploadDate)   return uploadDate < o.uploadDate;
        return timestamp < o.timestamp;
    }

    return false;
}

#include <future>
#include <queue>
#include <condition_variable>
#include <mutex>

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;

public:
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    auto enqueue(F f) -> std::future<decltype(f())> {
        auto taskPtr =
            std::make_shared<std::packaged_task<decltype(f())()>>(std::move(f));
        std::future<decltype(f())> res = taskPtr->get_future();
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace([taskPtr] { (*taskPtr)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (auto &t : workers)
            t.join();
    }
};


std::vector<YoutubeVideo> YoutubeVideo::loadYoutubeVideos(
    const fs::path& archiveBoxArchiveDirectory,
    const Args& argsInstance
) {
    std::vector<std::future<YoutubeVideo>> futures;

    bool alwaysMetadata = argsInstance.getBool(ArgType::ALWAYS_GENERATE_METADATA);
    std::string argVideo = argsInstance.getString(ArgType::VIDEO).value_or("");
    std::string argChannelId = argsInstance.getString(ArgType::CHANNEL).value_or("");

    size_t threadCount = std::max(2u, std::thread::hardware_concurrency());
    ThreadPool pool(threadCount);

    // ✅ Iterate directories
    for (auto &entry : fs::directory_iterator(archiveBoxArchiveDirectory)) {
        const fs::path snapshotDir = entry.path();
        fs::path mediaDir = snapshotDir / "media";
        if (!fs::exists(mediaDir))
            continue;

        futures.push_back(
            pool.enqueue([mediaDir, alwaysMetadata, argVideo] {
                return YoutubeVideo(mediaDir, alwaysMetadata, argVideo);
            })
        );
    }

    // ✅ Collect results
    std::vector<YoutubeVideo> videos;
    videos.reserve(futures.size());

    int index = 0;
    size_t archiveCount = futures.size();

    for (auto &f : futures) {
        YoutubeVideo v = f.get();

        // filtering must happen AFTER loading
        if (!argVideo.empty() && v.id != argVideo)
            continue;
        if (!argChannelId.empty() && v.channelId != argChannelId)
            continue;

        ++index;
        std::cout << "\n\nFound video #" << index << "\n";
        std::cout << "id = " << v.id << "\n";
        std::cout << "snapshot = " << v.snapshot << "\n";
        std::cout << "title = " << v.title << "\n";
        std::cout << "videoFileName = " << v.videoFileName << "\n";
        std::cout << "videoFileSizeInBytes = " << v.videoFileSizeInBytes << "\n";
        std::cout << "videoFileSha512HashSum = " << v.videoFileSha512HashSum << "\n";
        std::cout << "videoDuration = " << v.videoDuration << "\n";
        std::cout << "getVideoDurationInMilliseconds = " << v.getVideoDurationInMilliseconds() << "\n";

        totalDurationInMilliseconds += v.getVideoDurationInMilliseconds();

        videos.push_back(std::move(v));
    }

    // ✅ Must preserve original global sorting
    std::sort(videos.begin(), videos.end());

    // ✅ Set previous/next
    for (size_t j = 0; j < videos.size(); ++j) {
        if (j > 0)
            videos[j].previousVideoId = videos[j-1].id;
        if (j < videos.size() - 1)
            videos[j].nextVideoId = videos[j+1].id;
    }

    return videos;
}

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

#include "TestFramework.h"
#include "YoutubeVideo.h"

static YoutubeVideo makeVideo(const std::string& channelName, const std::string& uploadDate, long timestamp)
{
    YoutubeVideo v;
    v.channelName = channelName;
    v.uploadDate = uploadDate;
    v.timestamp = timestamp;
    return v;
}

TEST(video_sorts_by_channel_name_first)
{
    auto a = makeVideo("Alpha channel", "20230101", 100);
    auto b = makeVideo("Beta channel", "20230101", 100);
    CHECK(a < b);
    CHECK(!(b < a));
}

TEST(video_sorts_by_upload_date_within_same_channel)
{
    auto older = makeVideo("Blupi", "20230101", 999);
    auto newer = makeVideo("Blupi", "20230201", 1);
    CHECK(older < newer);
    CHECK(!(newer < older));
}

TEST(video_sorts_by_timestamp_when_channel_and_date_match)
{
    auto earlier = makeVideo("Blupi", "20230101", 100);
    auto later = makeVideo("Blupi", "20230101", 200);
    CHECK(earlier < later);
    CHECK(!(later < earlier));
}

// Videos with no channel metadata are normalized to
// YoutubeVideo::UNCATEGORIZED_CHANNEL_NAME by loadYoutubeVideos before this
// operator ever sees them (Phase E1 fix); operator< itself just needs to
// stay a valid strict-weak-ordering (not UB) if it's ever handed one with
// a genuinely empty channelName.
TEST(video_with_no_channel_name_has_no_defined_order)
{
    YoutubeVideo noChannel;
    auto withChannel = makeVideo("Blupi", "20230101", 100);
    CHECK(!(noChannel < withChannel));
    CHECK(!(withChannel < noChannel));
}

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
#include "Args.h"
#include "ArgType.h"
#include "YoutubedlFrontendException.h"

TEST(args_defaults_when_nothing_passed)
{
    Args args(std::vector<std::string>{});
    CHECK_EQ(args.getInt(ArgType::VIDEOS_PER_ROW).value_or(-1), 4);
    CHECK_EQ(args.getInt(ArgType::VIDEOS_PER_PAGE).value_or(-1), 60);
    CHECK_EQ(args.getBool(ArgType::ALWAYS_GENERATE_METADATA), true);
    CHECK_EQ(args.getBool(ArgType::ALWAYS_GENERATE_HTML_FILES), true);
    CHECK_EQ(args.getBool(ArgType::THUMBNAIL_AS_BASE64), false);
    CHECK_EQ(args.getBool(ArgType::THUMBNAIL_LINKS_TO_YOUTUBE), false);
}

TEST(args_parses_a_valid_flag_value)
{
    Args args({"--videos-per-row", "3"});
    CHECK_EQ(args.getInt(ArgType::VIDEOS_PER_ROW).value_or(-1), 3);
}

TEST(args_video_and_channel_filters_parse)
{
    Args args({"--video", "5rGd2VQz3mo", "--channel", "UCqBpgfXap7cZOYkAC34u8Lg"});
    CHECK_EQ(args.getString(ArgType::VIDEO).value_or(""), std::string("5rGd2VQz3mo"));
    CHECK_EQ(args.getString(ArgType::CHANNEL).value_or(""), std::string("UCqBpgfXap7cZOYkAC34u8Lg"));
}

// Regression test for the bug fixed in Phase A: an invalid --videos-per-row
// used to be stored as "0", collapsing the grid's max-width to 0px instead
// of falling back to the documented default.
TEST(args_invalid_videos_per_row_falls_back_to_default_instead_of_zero)
{
    Args args({"--videos-per-row", "1"});
    CHECK_EQ(args.getInt(ArgType::VIDEOS_PER_ROW).value_or(-1), 4);

    Args argsZero({"--videos-per-row", "0"});
    CHECK_EQ(argsZero.getInt(ArgType::VIDEOS_PER_ROW).value_or(-1), 4);
}

TEST(args_invalid_videos_per_page_falls_back_to_default)
{
    Args args({"--videos-per-page", "0"});
    CHECK_EQ(args.getInt(ArgType::VIDEOS_PER_PAGE).value_or(-1), 60);
}

// Regression test for the Phase E1 fix: an unrecognized --flag used to be
// silently ignored instead of failing loudly.
TEST(args_unknown_flag_throws)
{
    CHECK_THROWS(Args args({"--this-flag-does-not-exist", "1"}));
}

TEST(args_missing_value_for_flag_throws)
{
    CHECK_THROWS(Args args({"--video"}));
}

TEST(args_leading_positional_directory_is_not_treated_as_a_flag)
{
    // Main.cpp passes the working-directory argument through as-is; Args
    // must skip a first token that isn't "--something" rather than erroring.
    CHECK_NOTHROW(Args args({"/some/archive/path", "--video", "abc123"}));

    Args args({"/some/archive/path", "--video", "abc123"});
    CHECK_EQ(args.getString(ArgType::VIDEO).value_or(""), std::string("abc123"));
}

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
#include "Utils.h"

TEST(escapeHtml_escapes_all_special_characters)
{
    CHECK_EQ(Utils::escapeHtml("<b>Tom & Jerry's \"show\"</b>"),
             std::string("&lt;b&gt;Tom &amp; Jerry&#39;s &quot;show&quot;&lt;/b&gt;"));
}

TEST(escapeHtml_leaves_plain_text_unchanged)
{
    CHECK_EQ(Utils::escapeHtml("Blupi jede na hory - 1. dil"),
             std::string("Blupi jede na hory - 1. dil"));
}

TEST(escapeHtml_handles_empty_string)
{
    CHECK_EQ(Utils::escapeHtml(""), std::string(""));
}

TEST(formatDurationShort_drops_zero_hours)
{
    CHECK_EQ(Utils::formatDurationShort("00:12:04.31"), std::string("12:04"));
}

TEST(formatDurationShort_keeps_hours_and_trims_leading_zero)
{
    CHECK_EQ(Utils::formatDurationShort("01:02:04.31"), std::string("1:02:04"));
}

TEST(formatDurationShort_keeps_two_digit_hours)
{
    CHECK_EQ(Utils::formatDurationShort("12:02:04.31"), std::string("12:02:04"));
}

TEST(formatDurationShort_passes_through_unparseable_input)
{
    CHECK_EQ(Utils::formatDurationShort("not-a-duration"), std::string("not-a-duration"));
}

TEST(convertStringToBoolean_accepts_documented_values)
{
    CHECK_EQ(Utils::convertStringToBoolean("1"), true);
    CHECK_EQ(Utils::convertStringToBoolean("true"), true);
    CHECK_EQ(Utils::convertStringToBoolean("0"), false);
    CHECK_EQ(Utils::convertStringToBoolean("false"), false);
}

TEST(convertStringToBoolean_throws_on_anything_else)
{
    CHECK_THROWS(Utils::convertStringToBoolean("yes"));
    CHECK_THROWS(Utils::convertStringToBoolean(""));
}

TEST(split_splits_on_delimiter)
{
    auto parts = Utils::split("00:12:04", ':');
    CHECK_EQ(parts.size(), static_cast<size_t>(3));
    CHECK_EQ(parts[0], std::string("00"));
    CHECK_EQ(parts[1], std::string("12"));
    CHECK_EQ(parts[2], std::string("04"));
}

TEST(replaceUnderscoresBySpaces_replaces_all_underscores)
{
    CHECK_EQ(Utils::replaceUnderscoresBySpaces("foreign_blupi_videos"), std::string("foreign blupi videos"));
}

TEST(makeFirstLetterUppercase_only_touches_first_char)
{
    CHECK_EQ(Utils::makeFirstLetterUppercase("blupi"), std::string("Blupi"));
    CHECK_EQ(Utils::makeFirstLetterUppercase("Blupi"), std::string("Blupi"));
    CHECK_EQ(Utils::makeFirstLetterUppercase(""), std::string(""));
}

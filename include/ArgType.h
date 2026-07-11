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
#include <optional>
#include <array>
#include <vector>

enum class ArgType
{
    VIDEO,
    CHANNEL,
    VIDEOS_PER_ROW,
    ALWAYS_GENERATE_METADATA,
    ALWAYS_GENERATE_HTML_FILES,
    THUMBNAIL_AS_BASE64,
    THUMBNAIL_LINKS_TO_YOUTUBE
};

struct ArgInfo
{
    const char* name;
    const char* defaultValue; // nullptr = no default
    const char* description;
};

static constexpr std::array<ArgInfo, 7> ARG_INFOS{
    {
        {"video", "", "Only process the video with this YouTube video id"},
        {"channel", "", "Only process videos from this YouTube channel id"},
        {"videos-per-row", "4", "Soft column cap for the video grid width"},
        {"always-generate-metadata", "true", "Always rebuild the cached metadata file (0/1)"},
        {"always-generate-html-files", "true", "Always rebuild the per-video HTML pages (0/1)"},
        {"thumbnail-as-base64", "false", "Embed thumbnails as base64 data URIs (0/1)"},
        {"thumbnail-links-to-youtube", "false", "Link thumbnails to YouTube instead of the local video page (0/1)"}
    }
};

// Helper getters
inline const char* get_name(ArgType type)
{
    return ARG_INFOS[static_cast<size_t>(type)].name;
}

inline const char* get_default_value(ArgType type)
{
    return ARG_INFOS[static_cast<size_t>(type)].defaultValue;
}

inline const char* get_description(ArgType type)
{
    return ARG_INFOS[static_cast<size_t>(type)].description;
}

const std::vector<ArgType>& get_arg_type_values();

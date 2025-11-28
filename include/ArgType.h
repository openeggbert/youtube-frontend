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
};

static constexpr std::array<ArgInfo, 7> ARG_INFOS{
    {
        {"video", ""},
        {"channel", ""},
        {"videos-per-row", "4"},
        {"always-generate-metadata", "true"},
        {"always-generate-html-files", "true"},
        {"thumbnail-as-base64", "false"},
        {"thumbnail-links-to-youtube", "false"}
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

const std::vector<ArgType>& get_arg_type_values();

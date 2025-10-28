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
#ifndef ARGTYPE_H
#define ARGTYPE_H
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

// Static table mapping enum → properties (similar to Java enum fields)
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

#endif // ARGTYPE_H

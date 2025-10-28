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

#include "ArgType.h"
/**
 *
 * @author robertvokac
 */
const std::vector<ArgType>& get_arg_type_values()
{
    static std::vector<ArgType> v{
        ArgType::VIDEO,
        ArgType::CHANNEL,
        ArgType::VIDEOS_PER_ROW,
        ArgType::ALWAYS_GENERATE_METADATA,
        ArgType::ALWAYS_GENERATE_HTML_FILES,
        ArgType::THUMBNAIL_AS_BASE64,
        ArgType::THUMBNAIL_LINKS_TO_YOUTUBE
    };
    return v;
}

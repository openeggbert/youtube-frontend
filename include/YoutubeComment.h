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

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

class YoutubeComment {
public:
    std::string id;
    std::string parentId;
    std::string text;
    std::string author;
    long timestamp;

    YoutubeComment();
    YoutubeComment(const nlohmann::json& j);

    nlohmann::json to_json();
    bool operator<(const YoutubeComment& other) const;

    int dotCount() const;

    static std::vector<YoutubeComment> sort(const std::vector<YoutubeComment>& list);

private:
    static std::vector<YoutubeComment> getChildren(
        const std::vector<YoutubeComment>& all,
        const std::string& parentId
    );
};

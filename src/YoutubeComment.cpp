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

#include "YoutubeComment.h"

YoutubeComment::YoutubeComment()
    : timestamp(0)
{
}

YoutubeComment::YoutubeComment(const nlohmann::json& j)
{
    id = j.value("id", "");
    parentId = j.value("parent", "");
    text = j.value("text", "");
    author = j.value("author", "");
    timestamp = j.value("timestamp", 0L);
}

nlohmann::json YoutubeComment::to_json()
{
    nlohmann::json j;
    j["id"] = id;
    j["parent"] = parentId;
    j["text"] = text;
    j["author"] = author;
    j["timestamp"] = timestamp;
    return j;
}

bool YoutubeComment::operator<(const YoutubeComment& other) const
{
    return timestamp < other.timestamp;
}

int YoutubeComment::dotCount() const
{
    int count = 0;
    for (char c : id)
    {
        if (c == '.')
            count++;
    }
    return count;
}

std::vector<YoutubeComment> YoutubeComment::sort(const std::vector<YoutubeComment>& list)
{
    // Get root comments
    std::vector<YoutubeComment> root = getChildren(list, "root");

    // Sort roots by timestamp
    std::sort(root.begin(), root.end());

    // The original Java returns full list, sorted inside
    // so we return the full list
    return root;
}

std::vector<YoutubeComment> YoutubeComment::getChildren(
    const std::vector<YoutubeComment>& all,
    const std::string& parentId
)
{
    // Filter children with matching parentId
    std::vector<YoutubeComment> children;
    for (auto& c : all)
    {
        if (c.parentId == parentId)
            children.push_back(c);
    }

    // Sort them (oldest first)
    std::sort(children.begin(), children.end());

    // Recursively add children under each child
    std::vector<YoutubeComment> result;
    for (auto& c : children)
    {
        result.push_back(c);
        auto sub = getChildren(all, c.id);
        result.insert(result.end(), sub.begin(), sub.end());
    }

    return result;
}

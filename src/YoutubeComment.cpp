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

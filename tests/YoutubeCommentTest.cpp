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
#include "YoutubeComment.h"

static YoutubeComment makeComment(const std::string& id, const std::string& parentId, long timestamp)
{
    YoutubeComment c;
    c.id = id;
    c.parentId = parentId;
    c.timestamp = timestamp;
    c.author = "author-" + id;
    c.text = "text-" + id;
    return c;
}

TEST(comment_dotCount_counts_dots_in_id)
{
    YoutubeComment reply;
    reply.id = "a.b.c";
    CHECK_EQ(reply.dotCount(), 2);

    YoutubeComment root;
    root.id = "a";
    CHECK_EQ(root.dotCount(), 0);
}

TEST(comment_sort_orders_roots_by_timestamp_then_nests_replies_under_their_parent)
{
    std::vector<YoutubeComment> input = {
        makeComment("b", "root", 200),
        makeComment("a", "root", 100),
        makeComment("a.1", "a", 150),
    };

    auto sorted = YoutubeComment::sort(input);

    CHECK_EQ(sorted.size(), static_cast<size_t>(3));
    CHECK_EQ(sorted[0].id, std::string("a"));    // earliest root comment first
    CHECK_EQ(sorted[1].id, std::string("a.1"));  // a's reply immediately follows a
    CHECK_EQ(sorted[2].id, std::string("b"));    // later root comment last
}

// Regression test for the Phase E1 fix: a reply whose parent is missing
// from the data used to be silently dropped instead of shown as top-level.
TEST(comment_sort_keeps_orphaned_replies_instead_of_dropping_them)
{
    std::vector<YoutubeComment> input = {
        makeComment("a", "root", 100),
        makeComment("orphan", "does-not-exist", 200),
    };

    auto sorted = YoutubeComment::sort(input);
    CHECK_EQ(sorted.size(), static_cast<size_t>(2));

    bool foundOrphan = false;
    for (auto& c : sorted)
        if (c.id == "orphan")
            foundOrphan = true;
    CHECK(foundOrphan);
}

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

// A deliberately tiny, dependency-free test harness: no framework is
// installed in this environment and pulling one in (system package or
// FetchContent-from-network) would be a heavier change than this project's
// handful of pure-function unit tests warrant. TEST() registers a function
// at static-init time; run_all_tests() in main.cpp runs every registered
// test and reports pass/fail.

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace testing
{
    struct TestCase
    {
        std::string name;
        std::function<void()> fn;
    };

    std::vector<TestCase>& registry();

    struct Registrar
    {
        Registrar(const std::string& name, const std::function<void()>& fn);
    };

    void checkFailed(const std::string& file, int line, const std::string& expr);

    int run_all_tests();
}

#define TEST(name)                                                          \
    static void name();                                                    \
    static ::testing::Registrar registrar_##name(#name, name);              \
    static void name()

#define CHECK(expr)                                                         \
    do {                                                                    \
        if (!(expr))                                                        \
            ::testing::checkFailed(__FILE__, __LINE__, #expr);              \
    } while (0)

#define CHECK_EQ(actual, expected)                                          \
    do {                                                                    \
        auto _actual = (actual);                                           \
        auto _expected = (expected);                                       \
        if (!(_actual == _expected)) {                                     \
            std::ostringstream _ss;                                        \
            _ss << #actual << " == " << #expected                          \
                << " (got \"" << _actual << "\", expected \""              \
                << _expected << "\")";                                     \
            ::testing::checkFailed(__FILE__, __LINE__, _ss.str());          \
        }                                                                   \
    } while (0)

#define CHECK_THROWS(expr)                                                  \
    do {                                                                    \
        bool _threw = false;                                                \
        try { expr; } catch (...) { _threw = true; }                        \
        if (!_threw)                                                        \
            ::testing::checkFailed(__FILE__, __LINE__,                      \
                                    "expected " #expr " to throw");         \
    } while (0)

#define CHECK_NOTHROW(expr)                                                 \
    do {                                                                    \
        try { expr; }                                                       \
        catch (const std::exception& ex) {                                  \
            std::ostringstream _ss;                                        \
            _ss << "expected " #expr " not to throw, but got: "            \
                << ex.what();                                              \
            ::testing::checkFailed(__FILE__, __LINE__, _ss.str());          \
        }                                                                   \
    } while (0)

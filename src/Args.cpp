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

#include "Args.h"
#include "Arg.h"
#include "Utils.h"
#include "YoutubedlFrontendException.h"

Args::Args(const std::vector<std::string>& args)
{
    for (size_t i = 0; i < args.size(); ++i)
    {
        const std::string& arg = args[i];

        if (i == 0 && arg.rfind(TWO_DASHES, 0) != 0)
        {
            continue; // first must be --..., otherwise skip
        }

        // find match by name
        std::optional<ArgType> found = std::nullopt;

        for (auto a : get_arg_type_values())
        {
            if (arg == TWO_DASHES + get_name(a))
            {
                found = a;
                break;
            }
        }

        if (found.has_value())
        {
            ++i;
            if (i >= args.size())
            {
                throw YoutubedlFrontendException(
                    std::string("Fatal error: missing value for --") + get_name(found.value())
                );
            }

            std::string value = args[i];

            if (found.value() == ArgType::VIDEOS_PER_ROW)
            {
                int intVal = std::atoi(args[i].c_str());
                if (intVal < 2)
                {
                    value = "0";
                }
            }

            map.emplace(found.value(), Arg(found.value(), value));
        }
    }
}

std::optional<std::string> Args::getString(ArgType type) const
{
    auto it = map.find(type);
    if (it == map.end())
    {
        std::string def = get_default_value(type);
        if (def.empty())
        {
            return std::nullopt;
        }
        return def;
    }
    return it->second.value;
}

bool Args::getBool(ArgType type) const
{
    auto o = getString(type);
    if (!o.has_value()) return false;
    return Utils::convertStringToBoolean(o.value());
}

std::optional<int> Args::getInt(ArgType type) const
{
    auto o = getString(type);
    if (!o.has_value()) return std::nullopt;
    return std::stoi(o.value());
}

std::string Args::to_string() const
{
    std::ostringstream ss;
    bool first = true;

    for (const auto& [type, arg] : map)
    {
        if (!first)
        {
            ss << "\n";
        }
        first = false;

        ss << TWO_DASHES << get_name(type) << "=" << arg.value;
    }

    return ss.str();
}

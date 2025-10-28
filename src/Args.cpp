//
// Created by robertvokac on 10/28/25.
//

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
            // we need this as a replacement for ArgType.values()
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

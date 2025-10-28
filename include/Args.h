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
#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <iostream>

#include "ArgType.h"
#include "Arg.h"

class Args {
public:
    static inline const std::string TWO_DASHES = "--";

private:
    std::unordered_map<ArgType, Arg> map;

public:
    Args() = default;

    explicit Args(const std::vector<std::string>& args);

    std::optional<std::string> getString(ArgType type) const;

    bool getBool(ArgType type) const;

    std::optional<int> getInt(ArgType type) const;

    std::string to_string() const;
};
#endif // ARGS_H

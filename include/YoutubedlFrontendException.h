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
 * @author <a href="mailto:mail@robertvokac.com">Robert Vokac</a>
 * @since 0.0.0
 */
#ifndef YOUTUBEDLFRONTENDEXCEPTION_H
#define YOUTUBEDLFRONTENDEXCEPTION_H

#include <stdexcept>
#include <string>
#include <exception>

/**
 * @author Robert Vokac
 * @since 0.0.0
 */
class YoutubedlFrontendException : public std::runtime_error {
public:
    // constructor: only message
    explicit YoutubedlFrontendException(const std::string& msg)
        : std::runtime_error(msg) {}

    // constructor: message + inner exception
    YoutubedlFrontendException(const std::string& msg, const std::exception& e)
        : std::runtime_error(msg + " | caused by: " + e.what()) {}
};
#endif // YOUTUBEDLFRONTENDEXCEPTION_H

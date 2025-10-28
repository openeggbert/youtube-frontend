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


// import dev.mccue.guava.hash.Hashing;
// import java.awt.Image;
// import java.awt.image.BufferedImage;
// import java.io.BufferedReader;
// import java.io.ByteArrayOutputStream;
// import java.io.File;
// import java.io.FileWriter;
// import java.io.IOException;
// import java.io.InputStream;
// import java.io.InputStreamReader;
// import java.io.PrintWriter;
// import java.nio.file.Files;
// import java.nio.file.Path;
// import java.nio.file.Paths;
// import java.nio.file.StandardCopyOption;
// import java.text.DateFormat;
// import java.text.DecimalFormat;
// import java.text.NumberFormat;
// import java.text.SimpleDateFormat;
// import java.util.*;
// import javax.imageio.ImageIO;

/**
 *
 * @author pc00289
 */
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <string_view>
#include <vector>

#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>

namespace fs = std::filesystem;

class Utils
{
public:
    Utils() = delete;
    Utils(const Utils&) = delete;
    Utils& operator=(const Utils&) = delete;

    // public static final DateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
    // public static final NumberFormat TWO_DECIMAL_POINTS_FORMATTER = new DecimalFormat("#0.00");

    static std::string replaceUnderscoresBySpaces(const std::string& s);

    static std::string makeFirstLetterUppercase(const std::string& s);


    static int getCountOfSlashOccurrences(std::string_view s);
    static std::vector<fs::path> listAllFilesInDir(const fs::path& dir);

    static std::string createDoubleDotSlash(int times);


    static void copyFile(const fs::path& src, const fs::path& dstDir);
    static void writeTextToFile(const std::string& text, const fs::path& file);

    static std::string readTextFromFile(const fs::path& file);

    static std::string calculateSHA512Hash(const fs::path& file);

    static bool convertStringToBoolean(const std::string& s);


    static std::vector<uint8_t> resizeImage(const fs::path& input,
                                            int width, int height,
                                            const std::string& format);


    static void downloadFile(const std::string& url, const fs::path& target);

    static std::vector<std::string> split(const std::string&, char delimiter);


private:
    static void listAllFilesInDirRec(const fs::path& dir, std::vector<fs::path>& files);
};


#endif // UTILS_H

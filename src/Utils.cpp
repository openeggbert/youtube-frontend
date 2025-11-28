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

#include "Utils.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "YoutubedlFrontendException.h"
#include <curl/curl.h>

std::string Utils::replaceUnderscoresBySpaces(const std::string& s)
{
    std::string r = s;
    std::replace(r.begin(), r.end(), '_', ' ');
    return r;
}

std::string Utils::makeFirstLetterUppercase(const std::string& s)
{
    if (s.empty())
        return s;

    std::string r = s;
    if (std::islower(static_cast<unsigned char>(r[0])))
        r[0] = std::toupper(static_cast<unsigned char>(r[0]));

    return r;
}

int Utils::getCountOfSlashOccurrences(std::string_view s)
{
    return std::count(s.begin(), s.end(), '/');
}

std::vector<fs::path> Utils::listAllFilesInDir(const fs::path& dir)
{
    std::vector<fs::path> result;
    listAllFilesInDirRec(dir, result);
    return result;
}

std::string Utils::createDoubleDotSlash(int times)
{
    std::string r;
    for (int i = 0; i < times; i++)
        r += "../";
    return r;
}

void Utils::copyFile(const fs::path& src, const fs::path& dstDir)
{
    try
    {
        fs::copy_file(src, dstDir / src.filename(),
                      fs::copy_options::overwrite_existing);
    }
    catch (const std::exception& e)
    {
        throw YoutubedlFrontendException("Copying file failed: " + src.string());
    }
}

void Utils::writeTextToFile(const std::string& text, const fs::path& file)
{
    std::ofstream ofs(file);
    if (!ofs)
        throw std::runtime_error("Cannot write to file: " + file.string());
    ofs << text;
}

std::string Utils::readTextFromFile(const fs::path& file)
{
    if (!fs::exists(file))
        return "";

    std::ifstream ifs(file);
    if (!ifs)
        throw std::runtime_error("Cannot read file: " + file.string());

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

std::string Utils::calculateSHA512Hash(const fs::path& file)
{
    if (!fs::exists(file))
        throw std::runtime_error("File not found: " + file.string());

    std::ifstream ifs(file, std::ios::binary);
    SHA512_CTX ctx;
    SHA512_Init(&ctx);

    std::array<char, 4096> buf;
    while (ifs.good())
    {
        ifs.read(buf.data(), buf.size());
        SHA512_Update(&ctx, buf.data(), ifs.gcount());
    }

    unsigned char hash[SHA512_DIGEST_LENGTH];
    SHA512_Final(hash, &ctx);

    std::ostringstream oss;
    for (unsigned char c : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;

    return oss.str();
}

bool Utils::convertStringToBoolean(const std::string& s)
{
    if (s == "1" || s == "true") return true;
    if (s == "0" || s == "false") return false;
    throw std::runtime_error("Could not create boolean from string: " + s);
}


std::vector<uint8_t> Utils::resizeImage(const fs::path& input,
                                        int width, int height,
                                        const std::string& format)
{
    cv::Mat src = cv::imread(input.string());
    if (src.empty())
        throw std::runtime_error("Could not read image: " + input.string());

    cv::Mat dst;
    cv::resize(src, dst, cv::Size(width, height), 0, 0, cv::INTER_AREA);

    std::vector<uint8_t> buf;
    if (!cv::imencode("." + format, dst, buf))
        throw std::runtime_error("Image encoding failed");

    return buf;
}


void Utils::downloadFile(const std::string& url, const fs::path& target)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw YoutubedlFrontendException("Failed to initialize CURL");

    // Open file for writing
    FILE* fp = fopen(target.string().c_str(), "wb");
    if (!fp)
    {
        curl_easy_cleanup(curl);
        throw YoutubedlFrontendException("Failed to open file for writing: " + target.string());
    }

    // Configure CURL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr); // Use default write function
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

    // Perform the download
    CURLcode res = curl_easy_perform(curl);

    // Cleanup
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw YoutubedlFrontendException("Failed to download file: " + url);
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}


void Utils::listAllFilesInDirRec(const fs::path& dir, std::vector<fs::path>& files)
{
    files.push_back(dir);

    for (auto& p : fs::directory_iterator(dir))
    {
        if (fs::is_directory(p.path()))
            listAllFilesInDirRec(p.path(), files);
        else
            files.push_back(p.path());
    }
}

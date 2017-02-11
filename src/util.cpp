/*
 * Part of the Chrd chord editor
 * http://www.github.com/oetherington/chrd
 *
 * Copyright 2016 Ollie Etherington.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "util.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
std::string VFormat(const char *const fmt, va_list args)
{
    size_t size = 512;
    char stackbuf[size];
    std::vector<char> dynamicbuf;
    char *buf = &stackbuf[0];

    while (1) {
        const int needed = vsnprintf(buf, size, fmt, args);

		if (needed > -1 && needed < static_cast<int>(size))
            return std::string (buf, static_cast<size_t>(needed));

        size = (needed > 0) ? (static_cast<size_t>(needed) + 1) : (size * 2);
        dynamicbuf.resize(size);
        buf = &dynamicbuf[0];
    }
}
#pragma clang diagnostic pop

std::string Format(const char *const fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string result = VFormat(fmt, args);
	va_end(args);
	return result;
}

std::string &LTrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string &RTrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

std::string &Trim(std::string &s)
{
	return LTrim(RTrim(s));
}

std::vector<std::string> &Split(const std::string &s, char delim,
		std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		if (item.size())
			elems.push_back(item);
    return elems;
}

std::vector<std::string> Split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	Split(s, delim, elems);
	return elems;
}

void ReplaceAll(std::string &str, const std::string &from,
		const std::string &to)
{
	if (from.empty())
		return;

	size_t start_pos = 0;

	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

std::string ReadFile(const std::string &path)
{
	std::ifstream ifs(path);

	if (!ifs.good())
		throw Error(std::string("Couldn't open file: ") + path);

	std::stringstream ss;
	ss << ifs.rdbuf();

	return ss.str();
}

void WriteFile(const std::string &path, const std::string &data)
{
	std::ofstream ofs(path);

	if (!ofs.good())
		throw Error(std::string("Couldn't open file for writing: ") + path);

	ofs << data;
}

void WriteFile(const std::string &path, const unsigned char *const data,
		const size_t len)
{
	std::ofstream ofs(path);

	if (!ofs.good())
		throw Error(std::string("Couldn't open file for writing: ") + path);

	ofs.write((const char *const) data, len);
}

std::string NameFromPath(const char *const path)
{
	return std::string(basename(path));
}

bool FileExists(const char *const file)
{
	struct stat st = { 0 };
	return !stat(file, &st);
}

bool DirectoryExists(const char *const dir)
{
	struct stat st = { 0 };
	return !stat(dir, &st);
}

void EnsureDirectoryExists(const char *const dir)
{
	if (!DirectoryExists(dir))
		if (mkdir(dir, 0777))
			throw Error(Format("Couldn't create directory '%s': %s", dir,
						strerror(errno)));
}

void RemoveDirectory(const char *const dir)
{
	if (rmdir(dir))
		throw Error(Format("Couldn't remove directory '%s': %s", dir,
					strerror(errno)));
}

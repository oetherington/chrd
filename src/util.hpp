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

#pragma once

#ifndef UTIL_HPP
#define UTIL_HPP

#include "xxhash.h"
#include <string>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <sstream>

std::string VFormat(const char *const fmt, va_list args);

__attribute__((__format__ (__printf__, 1, 2)))
std::string Format(const char *const fmt, ...);

std::string &LTrim(std::string &s);
std::string &RTrim(std::string &s);
std::string &Trim(std::string &s);

std::vector<std::string> &Split(const std::string &s, char delim,
		std::vector<std::string> &elems);
std::vector<std::string> Split(const std::string &s, char delim);

void ReplaceAll(std::string &str, const std::string &from,
		const std::string &to);

std::string ReadFile(const std::string &path);
void WriteFile(const std::string &path, const std::string &data);
void WriteFile(const std::string &path, const unsigned char *const data,
		const size_t len);

std::string NameFromPath(const char *const path);

bool FileExists(const char *const file);

bool DirectoryExists(const char *const dir);
void EnsureDirectoryExists(const char *const dir);
void RemoveDirectory(const char *const dir);

typedef XXH64_hash_t Hash;

static inline Hash HashData(const unsigned char *const data,
		const size_t len)
{
	return XXH64((const void *) data, len, 0xDEADBEEF);
}

static inline Hash HashString(const std::string &s)
{
	return HashData((const unsigned char *const) s.c_str(), s.size());
}

static inline Hash HashFile(const char *const path)
{
	return HashString(ReadFile(path));
}

class Error {
private:
	std::string m_what;

public:
	explicit Error(const std::string what) noexcept
		: m_what(what)
	{}

	inline const std::string &What() const { return m_what; }

	inline const std::string &ToString() const { return What(); }
};

#endif

/*
 * fileutils.cpp
 *
 *  Created on: Aug 3, 2019
 *      Author: sinan
 */

#include "fileutils.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stack>

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace fileutils
{
std::string load_file(const std::string& filepath)
{
	// if directory
	if (exist(filepath, fileutils::FT_DIR))
		return string();

	std::ifstream f(filepath.c_str());
	if (!f.is_open())
		return string();

	std::string content((std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());

	return content;
}

bool load_file(const std::string& filepath, std::vector<std::string>& lines)
{
	ifstream iFile;
	string rString;
	const int size = 1024;
	char buf[size];

	lines.clear();

	iFile.open(filepath.c_str(), ios::in | ios::binary);
	if (!iFile.is_open())
		return false;

	while (iFile.getline(buf, size))
	{
		lines.push_back(buf);
	}

	iFile.close();

	return true;
}

bool save_file(const std::string& filePath, const std::string& text,
		const int chmod)
{

	if (filePath.empty())
		return false;

	// if directory
	if (exist(filePath, fileutils::FT_DIR))
		return false;

	// create parent directory
	string parent = dirname(filePath);
	if (!parent.empty() && !exist(parent))
		mkdir(parent, 777);

	ofstream oFile;

	string path = filePath;

	oFile.open(path.c_str());

	if (!oFile.is_open())
	{
		//cout << path << " file could not be saved" << endl;
		return false;
	}

	oFile << text;

	oFile.close();

	if (chmod != -1)
		fileutils::chmod(filePath, chmod);

	return true;
}

bool remove(const std::string& filepath)
{
	return (::remove(filepath.c_str()) == 0);
}

bool mkdir(const std::string & dirpath, mode_t mode)
{
	int res = 0;
	size_t offset = 0, end = 1;

	do
	{
		offset = dirpath.find('/', end);

		// create sub-folder
		res = ::mkdir(dirpath.substr(0, offset).c_str(), mode);
		if (res < 0 && errno != EEXIST) // break loop if any other error occurs than file exists.
			break;
		else
			res = 0;

		end = offset + 1;

	} while (offset != string::npos);

	return res;
}

int chmod(const std::string & path, int mode)
{
	char buf[7];

	snprintf(buf, sizeof(buf), "%d", mode);

	int mode_new = (int) strtol(buf, NULL, 16);
	int u = (mode_new & 0xF00) >> 2;
	int g = (mode_new & 0x0F0) >> 1;
	int o = (mode_new & 0x00F) >> 0;

	int f_mode = u | g | o;

	// if not a directory, then do it immediately
	if (!exist(path))
	{
		return ::chmod(path.c_str(), f_mode);
	}

	// if a directory, then traverse it

	int res = 0;
	vector<string> filenames;
	stack<string> stackPath;
	string top, childpath;

	stackPath.push(path);

	while (!stackPath.empty())
	{
		top = stackPath.top();
		stackPath.pop();

		//		 change mode of sub-folder
		if (::chmod(top.c_str(), f_mode) < 0)
			res = -1;

		if (read_dir(top, filenames))
		{
			for (size_t i = 0; i < filenames.size(); ++i)
			{
				childpath = top + "/" + filenames[i];
				if (exist(childpath, FT_DIR))
				{
					stackPath.push(childpath);
				}
				else // change mode of file
				{
					if (::chmod(childpath.c_str(), f_mode) < 0)
						res = -1;
				}
			} // end-of-for filenames
		} // end-of-if getFilesInDir
	} // end-of-while !stackPath.empty()

	return res;
}

std::string dirname(const std::string & filepath)
{
	char * dup = ::strdup(filepath.c_str());
	string dirname = ::dirname(dup);
	::free(dup);

	return dirname;
}

std::string basename2(const std::string& filepath, bool remove_extension)
{
	char * dup = ::strdup(filepath.c_str());
	string bname = ::basename(dup);
	::free(dup);

	if (remove_extension)
	{
		size_t dot = bname.find('.');
		if (dot != string::npos)
			bname.erase(dot);
	}

	return bname;
}

std::string extension(const std::string & filepath)
{
	string filename = fileutils::basename2(filepath, false);
	size_t dot = filename.rfind('.');
	if (dot == string::npos)
		return string();

	return filename.substr(dot);
}

bool exist(const std::string& path, int file_types)
{
	if (file_types == FT_ALL)
	{
		return (::access(path.c_str(), F_OK) == 0);
	}

	struct stat stat;

	if (::stat(path.c_str(), &stat))
		return false;

	if ((file_types & FT_DIR) && S_ISDIR(stat.st_mode))
		return true;
	if ((file_types & FT_CHAR) && S_ISCHR(stat.st_mode))
		return true;
	if ((file_types & FT_BLOCK) && S_ISBLK(stat.st_mode))
		return true;
	if ((file_types & FT_REG) && S_ISREG(stat.st_mode))
		return true;
	if ((file_types & FT_FIFO) && S_ISFIFO(stat.st_mode))
		return true;
	if ((file_types & FT_LINK) && S_ISLNK(stat.st_mode))
		return true;
	if ((file_types & FT_SOCKET) && S_ISSOCK(stat.st_mode))
		return true;

	return false;
}
bool read_dir(const std::string& dirpath, std::vector<std::string>& filenames)
{
	bool res = true;
	DIR * d;
	struct dirent *dir;
	struct dirent entry;

	filenames.clear();

	d = opendir(dirpath.c_str());
	if (!d)
		return false;

	while (true)
	{
		if (readdir_r(d, &entry, &dir))
		{
			//			perror("readdir");
			res = false;
			break;
		}

		if (!dir)
			break;

		//		printf("%s\n", dir->d_name);

		// skip '.' and '..'
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
			continue;

		filenames.push_back(dir->d_name);

	} // end-of-while

	closedir(d);

	return res;
}

}

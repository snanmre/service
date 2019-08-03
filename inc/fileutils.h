/*
 * fileutils.h
 *
 *  Created on: Aug 3, 2019
 *      Author: sinan
 */

#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include <string>
#include <vector>

#include <sys/stat.h>


#define DIRPATH_SERVICES		"./services"
#define DIRPATH_RUNTIME			"/run"
#define DIRPATH_SERVICE_SCRIPTS	"/run/shm/service"
#define DIRPATH_SERVICE_PIDS	DIRPATH_RUNTIME "/service"
#define FILEPATH_SERVICES_LIST	DIRPATH_SERVICE_PIDS "/services.list"


#define FILE_EXTENSION_CONFIG	".conf"


namespace fileutils
{
enum FileType
{
	FT_ALL = 0x00,
	FT_DIR = 0x01,
	FT_CHAR = 0x02,
	FT_BLOCK = 0x04,
	FT_REG = 0x08,
	FT_FIFO = 0x10,
	FT_LINK = 0x20,
	FT_SOCKET = 0x40
};

/** @warning small files only */
std::string load_file(const std::string & filepath);

bool load_file(const std::string & filepath, std::vector<std::string> & lines);

bool save_file(const std::string & filePath, const std::string & text,
		const int chmod = 777);

bool remove(const std::string & filepath);

bool mkdir(const std::string & dirpath, mode_t mode);

int chmod(const std::string & path, int mode);

/** parent directory path of the given file */
std::string dirname(const std::string & filepath);
std::string basename2(const std::string & filepath,
		bool remove_extension = false);

/** extension including dot */
std::string extension(const std::string & filepath);

bool exist(const std::string & path, int file_types = FT_ALL);

bool read_dir(const std::string & dirpath,
		std::vector<std::string> & filenames);
}

#endif /* FILEUTILS_H_ */

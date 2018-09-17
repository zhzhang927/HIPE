//@HIPE_LICENSE@
#include <core/misc.h>
#include <string>

#include <sstream>

#pragma warning(push, 0)
#include <boost/filesystem/operations.hpp>
#pragma warning(pop)

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

void hipe_usleep(long long usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}


int hipe_gettimeofday(hipetimeval* p, void* tz)
{
	ULARGE_INTEGER ul; // As specified on MSDN.
	FILETIME ft;

	// Returns a 64-bit value representing the number of
	// 100-nanosecond intervals since January 1, 1601 (UTC).
	GetSystemTimeAsFileTime(&ft);

	// Fill ULARGE_INTEGER low and high parts.
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;
	// Convert to microseconds.
	ul.QuadPart /= 10ULL;
	// Remove Windows to UNIX Epoch delta.
	ul.QuadPart -= 11644473600000000ULL;
	// Modulo to retrieve the microseconds.
	p->tv_usec = (long)(ul.QuadPart % 1000000LL);
	// Divide to retrieve the seconds.
	p->tv_sec = (long)(ul.QuadPart / 1000000LL);

	return 0;
}

#include <cstdlib>
#include <stdlib.h>
#include <sstream>
std::string getEnv(std::string path)
{
	char env_p[163840];
	GetEnvironmentVariable(path.c_str(), env_p, 163840);
	std::stringstream new_path;
	new_path << env_p;
	if (new_path.str().empty())
		return std::string("");

	return new_path.str();
}

void addEnv(std::string path)
{
	char env_p[163840];
	GetEnvironmentVariable("PATH", env_p, 163840);
	std::stringstream new_path;
	new_path << path << ";" << env_p;
	std::string cs = new_path.str();
	SetEnvironmentVariable("PATH", cs.c_str());

	GetEnvironmentVariable("PATH", env_p, 163840);


}

void addVarEnv(std::string key, std::string value)
{
	char env_p[163840];
	GetEnvironmentVariable(key.c_str(), env_p, 163840);
	std::stringstream new_path;
	new_path << value << ";" << env_p;
	std::string cs = new_path.str();
	SetEnvironmentVariable(key.c_str(), cs.c_str());

	GetEnvironmentVariable(key.c_str(), env_p, 163840);


}

#else
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

int hipe_gettimeofday(hipetimeval* p, void* tz) {
	return gettimeofday(p, (struct timezone *)tz);
}

void hipe_usleep(long long usec)
{
	usleep(usec);
}

std::string getEnv(std::string path)
{
	char * env_p;
	env_p = getenv("PATH");
	if (env_p == nullptr)
		return std::string("");

	std::stringstream new_path;
	new_path << env_p;

	return new_path.str();
}

void addEnv(std::string path)
{
	char * env_p;
	env_p = getenv("PATH");
	std::stringstream new_path;
	new_path << path << ";" << env_p;
	std::string cs = new_path.str();
	setenv("PATH", cs.c_str(), 1);
	
}

void addVarEnv(std::string key, std::string value)
{
	char * env_p;
	env_p = getenv(key.c_str());
	std::stringstream new_path;
	new_path << value << ";" << env_p;
	std::string cs = new_path.str();
	setenv(key.c_str(), cs.c_str(), 1);

}

#endif

bool isFileExist(std::string filename)
{
	if (!(boost::filesystem::exists(boost::filesystem::path(filename)) && boost::filesystem::is_regular_file(boost::filesystem::path(filename))))
	{
		std::stringstream buildMsg;
		buildMsg << "file [";
		buildMsg << filename;
		buildMsg << "] not found";
		throw std::invalid_argument(buildMsg.str());
	}
	return true;
}

bool isDirExist(const std::string & dirname)
{
	if (!(boost::filesystem::exists(boost::filesystem::path(dirname)) && boost::filesystem::is_directory(boost::filesystem::path(dirname))))
	{
		return false;
	}
	return true;

}

bool createDirectory(const std::string & dirpath)
{
	if (isDirExist(dirpath)) return false;

	return boost::filesystem::create_directory(boost::filesystem::path(dirpath));
}

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#define cd _chdir
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#define cd chdir
#endif

bool SetCurrentWorkingDir(const std::string & path)
{
	if (cd(path.c_str()) != 0)
		return false;

	return true;
}

std::string GetCurrentWorkingDir(void)
{
	char buff[BUFSIZ];
	GetCurrentDir(buff, BUFSIZ);
	std::string current_working_dir(buff);
	return current_working_dir;
}

std::string PathSeparator()
{
#if defined _WIN32 || defined __CYGWIN__
		return std::string("\\");
#else
		return std::string("/");
#endif
}

ImageFileType getImageTypeByMagic(const unsigned char* data, unsigned int len)
{
	if (len < 16) return IMAGE_FILE_INVALID;

	// .jpg:  FF D8 FF
	// .png:  89 50 4E 47 0D 0A 1A 0A
	// .gif:  GIF87a      
	//        GIF89a
	// .tiff: 49 49 2A 00
	//        4D 4D 00 2A
	// .bmp:  BM 
	// .webp: RIFF ???? WEBP 
	// .ico   00 00 01 00
	//        00 00 02 00 ( cursor files )

	switch (data[0])
	{
	case (unsigned char)'\xFF':
		return (!strncmp((const char*)data, "\xFF\xD8\xFF", 3)) ?
			IMAGE_FILE_JPG : IMAGE_FILE_INVALID;

	case (unsigned char)'\x89':
		return (!strncmp((const char*)data,
			"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8)) ?
			IMAGE_FILE_PNG : IMAGE_FILE_INVALID;

	case 'G':
		return (!strncmp((const char*)data, "GIF87a", 6) ||
			!strncmp((const char*)data, "GIF89a", 6)) ?
			IMAGE_FILE_GIF : IMAGE_FILE_INVALID;

	case 'I':
		return (!strncmp((const char*)data, "\x49\x49\x2A\x00", 4)) ?
			IMAGE_FILE_TIFF : IMAGE_FILE_INVALID;

	case 'M':
		return (!strncmp((const char*)data, "\x4D\x4D\x00\x2A", 4)) ?
			IMAGE_FILE_TIFF : IMAGE_FILE_INVALID;

	case 'B':
		return ((data[1] == 'M')) ?
			IMAGE_FILE_BMP : IMAGE_FILE_INVALID;

	case 'R':
		if (strncmp((const char*)data, "RIFF", 4))
			return IMAGE_FILE_INVALID;
		if (strncmp((const char*)(data + 8), "WEBP", 4))
			return IMAGE_FILE_INVALID;
		return IMAGE_FILE_WEBP;

	case '\0':
		if (!strncmp((const char*)data, "\x00\x00\x01\x00", 4))
			return IMAGE_FILE_ICO;
		if (!strncmp((const char*)data, "\x00\x00\x02\x00", 4))
			return IMAGE_FILE_ICO;
		return IMAGE_FILE_INVALID;

	default:
		return IMAGE_FILE_INVALID;
	}
}


std::string getExtensionFromImageData(const unsigned char* data, unsigned int len)
{
	ImageFileType type = getImageTypeByMagic(data, len);

	switch (type)
	{
	case IMAGE_FILE_GIF:
		return "gif";
	case IMAGE_FILE_BMP:
		return "bmp";
	case IMAGE_FILE_JPG:
		return "jpeg";
	case IMAGE_FILE_PNG:
		return "png";
	case IMAGE_FILE_TIFF:
		return "tiff";
	case IMAGE_FILE_WEBP:
		return "webp";
	case IMAGE_FILE_ICO:
		return "ico";
	default:
		return "";

	}
}

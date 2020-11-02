/*
-------------------------------------------------------------------------------

    Copyright (c) Charles Carley.

    Contributor(s): none yet.

-------------------------------------------------------------------------------
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#include <direct.h>
#include <io.h>
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum Colors
{
    CS_BLACK = 0,
    CS_DARKBLUE,
    CS_DARKGREEN,
    CS_DARKCYAN,
    CS_DARKRED,
    CS_DARKMAGENTA,
    CS_DARKYELLOW,
    CS_LIGHT_GREY,
    CS_GREY,
    CS_BLUE,
    CS_GREEN,
    CS_CYAN,
    CS_RED,
    CS_MAGENTA,
    CS_YELLOW,
    CS_WHITE,
    CS_COLOR_MAX
};

struct finddata_t
{
    string      name;
    _finddata_t data;

    bool operator<(const finddata_t& rhs) const
    {
        int a = (data.attrib & _A_SUBDIR);
        int b = (rhs.data.attrib & _A_SUBDIR);
        return a > b;
    }

    static bool sort_size(const finddata_t& lhs, const finddata_t& rhs)
    {
        // Give the directories a lower value so they are 
        // first in the list when a file also has zero size.
        int64_t a = (lhs.data.attrib & _A_SUBDIR) ? -1 : 0;
        int64_t b = (rhs.data.attrib & _A_SUBDIR) ? -1 : 0;
        return (lhs.data.size + a) < (rhs.data.size + b);
    }
};


// Replicating some similar options.
// http://www.man7.org/linux/man-pages/man1/ls.1.html
struct Options
{
    bool byline;         // -x, -c = by column (default)
    bool all;            // -a (hidden)
    bool system;         // -as (system)
    bool dirOnly;        // -d
    bool fileOnly;       // -f only files
    bool comma;          // -m
    bool quote;          // -q
    bool list;           // -l
    bool recursive;      // -R
    bool shortpath;      // -S
    int  winWidth;
};

struct ListReport
{
    uint64_t totalBytes;
    uint64_t totalFiles;
    uint64_t totalDirectories;
};

typedef vector<finddata_t> pathvec_t;
typedef vector<string>     strvec_t;
typedef vector<size_t>     ivec_t;

const size_t MaxName         = 28;
const size_t SizeWidth       = 18;
const char   SeperatorWin    = '\\';
const char   SeperatorNx     = '/';
const char   DefaultWildcard = '*';
const string Empty           = "";
const string Wildcard        = string(1, DefaultWildcard);

// Precomputed strings and offsets
const string SizeInBytes     = "Size in Bytes";
const string LastUsed        = "Last Modified";
const string FileName        = "Name";
const string Bytes           = "bytes";
const string Files           = " file(s)";
const string Directory       = " directory";
const string Directories     = " directories";
const string Found           = "Found: ";
const string And             = " and ";
const size_t SizeLabelCenter = 3;
const size_t LastModCenter   = 6;
const size_t NameLeft        = 5;

void          help();
void          setColor(int fore, int back = CS_BLACK);
unsigned char getColor(int fore, int back);

void listAll(const string&   cur,
             const string&   ex,
             const strvec_t& args,
             const Options&  opts,
             ListReport*     rept);

void combinePath(string&       dest,
                 const string& path,
                 const string& subpath,
                 const string& search);

void splitPath(const string& input,
               string&       path,
               string&       pattern);

void normalizePath(string&       dest,
                   const string& input);

void makeName(string&        dest,
              const string&  subDir,
              const string&  name,
              const Options& opts);

void writeListHeader(const string&  directory,
                     const Options& opts);

void writeListFooter(size_t sizeInBytes,
                     size_t files,
                     size_t dirs);

void writeReport(ListReport* rept);

void calculateColumns(pathvec_t&     vec,
                      ivec_t&        iv,
                      const size_t   maxWidth,
                      const Options& opts);

void getBytesString(string&        dest,
                    const uint64_t val);

bool shouldBeIncluded(const _finddata_t& val,
                     const Options&     opts);

BOOL WINAPI CtrlCallback(DWORD evt);

int main(int argc, char** argv)
{
    size_t   i;
    strvec_t args, externals;
    Options  opts = {};

    SetConsoleCtrlHandler(CtrlCallback, TRUE);

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &info) == 0)
    {
        // Just set it to something
        opts.winWidth = 100;
    }
    else
    {
        // If the output is redirected this
        // may become invalid, and should be handled above.
        opts.winWidth = info.dwSize.X;
    }

    if (argc > 1)
    {
        for (i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                char ch = argv[i][1];
                switch (ch)
                {
                case 'x':
                    opts.byline = true;
                    break;
                case 'c':
                    opts.byline = false;
                    break;
                case 'a':
                    if (argv[i][2] == 's')
                        opts.system = true;
                    opts.all = true;
                    break;
                case 'd':
                    opts.dirOnly = true;
                    break;
                case 'f':
                    opts.fileOnly = true;
                    break;
                case 'm':
                    opts.comma = true;
                    break;
                case 'q':
                    opts.quote = true;
                    break;
                case 'l':
                    opts.list = true;
                    break;
                case 'h':
                    help();
                    break;
                case 'S':
                    opts.shortpath = true;
                    break;
                case 'R':
                    opts.byline    = true;
                    opts.recursive = true;
                    break;
                }
            }
            else
            {
                string path, arg;
                string str;

                normalizePath(str, argv[i]);
                splitPath(str, path, arg);

                externals.push_back(path);

                if (arg.find(DefaultWildcard) == string::npos)
                    arg.push_back(SeperatorWin);
                if (arg.back() == SeperatorWin)
                    arg.push_back(DefaultWildcard);

                args.push_back(arg);
            }
        }
        if (args.empty())
            args.push_back(Wildcard);
    }
    else
        args.push_back(Wildcard);

    ListReport  lr     = {};
    ListReport* result = nullptr;
    if (opts.list && opts.recursive)
        result = &lr;

    if (!externals.empty())
    {
        for (string external : externals)
            listAll(Empty, external, args, opts, result);
    }
    else
    {
        assert(!args.empty());
        listAll(Empty, Empty, args, opts, result);
    }

    if (result)
        writeReport(result);

    return 0;
}

bool isDotEntry(char *cp)
{
    return cp[0] == '.' && cp[1] == '\0' ||
           cp[0] == '.' && cp[1] == '.' && cp[2] == '\0';
}

void listAll(const string&   callDir,
             const string&   subDir,
             const strvec_t& args,
             const Options&  opts,
             ListReport*     rept)
{
    _finddata_t find     = {};
    size_t      i        = 1, nrbytes, nrfiles, nrdirs;
    size_t      maxwidth = 0;
    strvec_t    dirs;

    if (opts.recursive)
    {
        string path;
        combinePath(path, callDir, subDir, Wildcard);

        intptr_t fp = _findfirst(path.c_str(), &find);
        if (fp != -1)
        {
            do
            {
                bool isSystem = !opts.system && (find.attrib & _A_SYSTEM) != 0;
                bool skip     = !opts.all && (find.attrib & _A_HIDDEN) != 0;

                if (!skip && !isSystem && find.attrib & _A_SUBDIR)
                {
                    if (!isDotEntry(find.name))
                        dirs.push_back(find.name);
                }
            } while (_findnext(fp, &find) == 0);

            _findclose(fp);
        }
    }

    pathvec_t vec;
    for (string arg : args)
    {
        string subpath;
        combinePath(subpath, callDir, subDir, arg);

        intptr_t fp = _findfirst(subpath.c_str(), &find);
        if (fp != -1)
        {
            do
            {
                if (shouldBeIncluded(find, opts))
                {
                    if (!isDotEntry(find.name))
                    {
                        finddata_t d = {find.name, find};
                        maxwidth     = std::max<size_t>(d.name.size(), maxwidth);
                        vec.push_back(d);
                    }
                }
            } while (_findnext(fp, &find) == 0);

            _findclose(fp);
        }
    }

    nrfiles = nrdirs = nrbytes = 0;
    if (!vec.empty())
    {
        if (opts.list)
        {
            sort(vec.begin(), vec.end(), finddata_t::sort_size);
            writeListHeader(subDir, opts);
        }
        else
            stable_sort(vec.begin(), vec.end());

    }

    ivec_t colums;
    calculateColumns(vec, colums, maxwidth, opts);

    size_t k = 0;
    for (i = 0; i < vec.size(); ++i)
    {
        const finddata_t& d = vec.at(i);

        bool isHidden    = (d.data.attrib & _A_HIDDEN) != 0;
        bool isDirectory = (d.data.attrib & _A_SUBDIR) != 0;
        bool isSystem    = (d.data.attrib & _A_SYSTEM) != 0;

        if (opts.list)
        {
            tm   tval;
            char buf[22] = {}; 
            if (::localtime_s(&tval, (time_t*)&d.data.time_write) == 0)
                ::strftime(buf, 22, "%D %r", &tval);

            cout << right;
            cout << setw(SizeWidth);
            if (isDirectory)
            {
                cout << ' ';  // filling SizeWidth
                cout << ' ';
                nrdirs++;
            }
            else
            {
                setColor(CS_YELLOW);
                string bytes;
                getBytesString(bytes, d.data.size);
                cout << bytes;

                cout << ' ';
                nrfiles++;
                nrbytes += d.data.size;
            }

            cout << left;
            setColor(CS_LIGHT_GREY);
            cout << buf << ' ';

            if (isSystem)
                setColor(CS_MAGENTA);
            else if (isHidden)
                setColor(CS_GREY);
            else if (isDirectory)
                setColor(CS_GREEN);
            else
                setColor(CS_WHITE);
            cout << d.name << '\n';
        }
        else
        {
            if (isSystem)
                setColor(CS_MAGENTA);
            else if (isDirectory && isHidden)
                setColor(CS_GREY);
            else if (isDirectory)
                setColor(CS_GREEN);
            else if (isHidden)
                setColor(CS_GREY);
            else
                setColor(CS_WHITE);

            string name;
            makeName(name, subDir, d.name, opts);
            if (!opts.byline)
            {
                size_t col = k++ % colums.size();
                cout << left;
                cout << setw(colums.at(col) + 1);
                cout << name << ' ';
                if (col == colums.size() - 1)
                    cout << '\n';
            }
            else
                cout << name << '\n';
        }
    }

    if (rept)
    {
        rept->totalDirectories += nrdirs;
        rept->totalFiles += nrfiles;
        rept->totalBytes += nrbytes;
    }

    if (opts.list)
    {
        if (!vec.empty())
            writeListFooter(nrbytes, nrfiles, nrdirs);
    }

    if (opts.recursive)
    {
        for (string dir : dirs)
        {
            string path;
            combinePath(path, subDir, dir, Empty);
            listAll(callDir, path, args, opts, rept);
        }
    }

    setColor(CS_WHITE);
}

void help()
{
    cout << "ls <options> wild-card\n\n";
    cout << "options:\n";
    cout << "    -x  list directory entries line by line.\n";
    cout << "    -c  list directory entries in columns (default).\n";
    cout << "    -a  list hidden files.\n";
    cout << "    -as list hidden files as well as system files.\n";
    cout << "    -d  only list directories.\n";
    cout << "    -f  only list files.\n";
    cout << "    -m  add a comma after each entry.\n";
    cout << "    -q  surround entries in quotations.\n";
    cout << "    -R  list recursively.\n";
    cout << "    -l  list the file size, last write time and the file name.\n";
    cout << "    -S  build a short path name. used with the -x and the -l options.\n";
    cout << "    -h  show this help message.\n";
    exit(0);
}

BOOL WINAPI CtrlCallback(DWORD evt)
{
    if (evt == CTRL_C_EVENT || evt == CTRL_BREAK_EVENT)
    {
        // To prevent lingering colors.
        setColor(CS_WHITE);
        ExitProcess(0);
        return 1;
    }
    return 0;
}

void getBytesString(string&        dest,
                    const uint64_t val)
{
    stringstream ss;
    ss << val;
    dest.clear();

    string tmp = ss.str();
    size_t i, s = tmp.size(); 
    if (s > 3)
    {
        size_t k = s % 3;
        size_t j = k == 0 ? 1 : 0;
        for (i = 0; i < s; ++i)
        {
            if (i+1 >= k)
            {
                dest.push_back(tmp[i]);
                if (j % 3 == 0 && (i+1) < s)
                    dest.push_back(',');
                j++;
            }
            else
                dest.push_back(tmp[i]);
        }
    }
    else
        dest = tmp;
}

void calculateColumns(pathvec_t& vec, ivec_t& iv, const size_t maxWidth, const Options& opts)
{
    size_t s = vec.size(), i, j;
    size_t nrCol = opts.winWidth / (maxWidth + 1);

    iv = ivec_t(nrCol, 0);
    for (i = 0; i < s; ++i)
    {
        const finddata_t& d = vec.at(i);

        j = i % nrCol;

        iv[j] = max<size_t>(iv[j], d.name.size());
    }
}

void makeName(string& dest, const string& subDir, const string& name, const Options& opts)
{
    if (opts.byline)
        dest = subDir + name;
    else
        dest = name;

    if (opts.shortpath && dest.find(' ') != string::npos)
    {
        string search = subDir + SeperatorWin + name;

        size_t len = (size_t)::GetShortPathName(search.c_str(), nullptr, 0);
        if (len > 0)
        {
            char* tmp = new char[len + 1];

            size_t nlen = (size_t)::GetShortPathName(search.c_str(), tmp, (DWORD)len);
            if (nlen != 0 && nlen <= len)
            {
                tmp[nlen] = 0;

                dest = tmp;
                if (!opts.byline)
                {
                    string pth;
                    splitPath(dest, pth, dest);
                }
            }
            delete[] tmp;
        }
    }

    if (opts.quote)
        dest = "\"" + dest + "\"";

    if (opts.comma)
        dest.push_back(',');
}

bool shouldBeIncluded(const _finddata_t& val, const Options& opts)
{
    if (!opts.all && (val.attrib & _A_HIDDEN) != 0)
        return false;
    if (!opts.system && (val.attrib & _A_SYSTEM) != 0)
        return false;
    if (opts.dirOnly && !(val.attrib & _A_SUBDIR))
        return false;
    if (opts.fileOnly && (val.attrib & _A_SUBDIR) != 0)
        return false;
    return true;
}

void splitPath(const string& input,
               string&       path,
               string&       ptrn)
{
    size_t pos = input.find_last_of(SeperatorWin);
    if (pos != string::npos)
    {
        path = input.substr(0, pos + 1);
        ptrn = input.substr(pos + 1, input.size());
    }
    else
    {
        path = Empty;
        ptrn = input;
    }
}

void normalizePath(string&       dest,
                   const string& input)
{
    dest = input;
    size_t pos;
    while ((pos = dest.find(SeperatorNx)) != string::npos)
        dest = dest.replace(pos, 1, &SeperatorWin);
}

void combinePath(string&       dest,
                 const string& path,
                 const string& subpath,
                 const string& search)
{
    dest = path;
    if (!dest.empty())
    {
        if (dest.back() != SeperatorWin)
            dest.push_back(SeperatorWin);
    }

    if (!subpath.empty())
        dest += subpath;

    if (!dest.empty())
    {
        if (dest.back() != SeperatorWin)
            dest.push_back(SeperatorWin);
    }

    if (!search.empty())
    {
        if (search.front() == SeperatorWin)
            dest += search.substr(1, search.size());
        else
            dest += search;
    }
}

void writeListHeader(const string& directory, const Options& opts)
{
    if (!directory.empty())
    {
        setColor(CS_DARKGREEN);
        cout << '\n';
        string dir;
        makeName(dir, directory, Empty, opts);
        cout << dir << '\n';
    }
    setColor(CS_LIGHT_GREY);
    cout << '\n';
    cout << setw(SizeLabelCenter) << ' ' << SizeInBytes << setw(LastModCenter) << ' ';
    cout << LastUsed << setw(NameLeft) << ' ';
    cout << FileName;
    cout << '\n';
    cout << '\n';
}

void writeReport(ListReport* rept)
{
    cout << '\n';
    setColor(CS_WHITE);
    cout << Found;
    setColor(CS_YELLOW);
    cout << '\n';

    string bytes;
    getBytesString(bytes, rept->totalBytes);
    cout << right << bytes;
    setColor(CS_DARKYELLOW);
    cout << ' ' << Bytes;
    setColor(CS_WHITE);

    cout << right << ' ' << rept->totalFiles << Files;
    if (rept->totalDirectories != 0)
    {
        cout << And;
        if (rept->totalDirectories > 1)
            cout << rept->totalDirectories << Directories;
        else
            cout << rept->totalDirectories << Directory;
    }
    cout << '\n';
}

void writeListFooter(size_t sizeInBytes, size_t files, size_t dirs)
{
    setColor(CS_YELLOW);
    cout << '\n';

    string bytes;
    getBytesString(bytes, sizeInBytes);

    cout << right << setw(SizeWidth) << bytes;
    setColor(CS_DARKYELLOW);

    cout << ' ' << Bytes;
    setColor(CS_WHITE);
    cout << right << setw(16) << ' ' << files << Files;
    if (dirs != 0)
    {
        cout << And;
        if (dirs > 1)
            cout << dirs << Directories;
        else
            cout << dirs << Directory;
    }
    cout << '\n';
}


void setColor(int fg, int bg)
{
    if (fg < 0 || fg > CS_COLOR_MAX || bg < 0 || bg > CS_COLOR_MAX)
        return;
    ::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), getColor(fg, bg));
}

const unsigned char COLOR_TABLE[16][16] = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
    {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},
    {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F},
    {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F},
    {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F},
    {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F},
    {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F},
    {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F},
    {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F},
    {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F},
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF},
    {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF},
    {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF},
    {0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF},
    {0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF},
    {0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF},
};

unsigned char getColor(int fore, int back)
{
    return COLOR_TABLE[back][fore];
}

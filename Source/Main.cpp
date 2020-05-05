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
        return lhs.data.size < rhs.data.size;
    }
};

// Replicating some similar options.
// http://www.man7.org/linux/man-pages/man1/ls.1.html
struct Options
{
    bool   byline;     // -x, -c = by column (default)
    bool   all;        // -a (hidden)
    bool   dirOnly;    // -d
    bool   comma;      // -m
    bool   list;       // -l
    bool   recursive;  // -R
    size_t winRight;
};

typedef vector<finddata_t> pathvec_t;
typedef vector<string>     strvec_t;

const size_t Padding         = 6;
const size_t MaxName         = 40;
const size_t SizeWidth       = 18;
const char   SeperatorWin    = '\\';
const char   SeperatorNx     = '/';
const string DefaultWildcard = "*";
const string Empty           = "";


void          resetState();
void          writeColor(int fore, int back = CS_BLACK);
unsigned char getColor(int fore, int back);
void          listAll(const string&   cur,
                      const string&   ex,
                      const strvec_t& args,
                      const Options&  opts);
string        combinePath(const string& path,
                          const string& subpath,
                          const string& search);
void          splitPath(const string& input,
                         string&       path,
                         string&       pattern);
string        normalizePath(const string& input);


int main(int argc, char** argv)
{
    size_t                     i;
    strvec_t                   args, externals;
    Options                    opts = {};
    CONSOLE_SCREEN_BUFFER_INFO info;
    string                     root_dir;

    atexit(resetState);

    GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &info);
    opts.winRight = info.srWindow.Right;

    if (argc <= 1)
        args.push_back(DefaultWildcard);
    else
    {
        i = 1;
        while (i < argc)
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
                    opts.all = true;
                    break;
                case 'd':
                    opts.dirOnly = true;
                    break;
                case 'm':
                    opts.comma = true;
                    break;
                case 'l':
                    opts.list = true;
                    break;
                case 'R':
                    opts.byline    = true;
                    opts.recursive = true;
                    break;
                }
                ++i;
            }
            else
            {
                string str = normalizePath(argv[i++]);
                string path, arg;
                splitPath(str, path, arg);

                externals.push_back(path);

                if (arg.find(DefaultWildcard) == string::npos)
                    arg += SeperatorWin;
                if (arg.back() == SeperatorWin || str.back() == SeperatorNx)
                    arg += DefaultWildcard;

                args.push_back(arg);
            }
        }
        if (args.empty())
            args.push_back(DefaultWildcard);
    }

    if (!externals.empty())
    {
        for (string external : externals)
            listAll(Empty, external, args, opts);
    }
    else if (!args.empty())
        listAll(Empty, Empty, args, opts);
    else  // should never happen
        cout << "failed to collect any arguments!\n";
    return 0;
}


void listAll(const string&   callDir,
             const string&   subDir,
             const strvec_t& args,
             const Options&  opts)
{
    _finddata_t find     = {};
    size_t      i        = 1;
    size_t      maxwidth = 0;
    strvec_t    dirs;

    if (opts.recursive)
    {
        string path = combinePath(callDir, subDir, DefaultWildcard);

        intptr_t fp = _findfirst(path.c_str(), &find);
        if (fp != -1)
        {
            do
            {
                bool isSystem = (find.attrib & _A_SYSTEM) != 0;
                bool skip     = !opts.all && (find.attrib & _A_HIDDEN) != 0;

                if (!skip && !isSystem && find.attrib & _A_SUBDIR)
                {
                    string dname = find.name;
                    if (dname != "." && dname != "..")
                        dirs.push_back(find.name);
                }
            } while (_findnext(fp, &find) == 0);
        }
        _findclose(fp);
    }

    pathvec_t vec;
    for (string arg : args)
    {
        string subpath = combinePath(callDir, subDir, arg);

        intptr_t fp = _findfirst(subpath.c_str(), &find);
        if (fp != -1)
        {
            do
            {
                finddata_t d = {find.name, find};
                if (!(find.attrib & _A_SYSTEM))
                {
                    maxwidth = std::max<size_t>(d.name.size(), maxwidth);
                    if (maxwidth > MaxName)
                        maxwidth = MaxName;

                    if (d.name != "." && d.name != "..")
                    {
                        if (d.name.size() > MaxName)
                        {
                            d.name = d.name.substr(0, MaxName - 3);
                            d.name += "...";
                        }
                        vec.push_back(d);
                    }
                }
            } while (_findnext(fp, &find) == 0);

            _findclose(fp);
        }
    }

    if (opts.list)
        sort(vec.begin(), vec.end(), finddata_t::sort_size);
    else
        stable_sort(vec.begin(), vec.end());

    i = 0;
    for (finddata_t d : vec)
    {
        bool isHidden = (d.data.attrib & _A_HIDDEN) != 0;
        if (!opts.all && isHidden)
            continue;

        bool isDirectory = (d.data.attrib & _A_SUBDIR) != 0;
        if (opts.dirOnly && !isDirectory)
            continue;

        if (opts.list)
        {
            tm tval;
            char buf[32] = {};
            if (::localtime_s(&tval, (time_t*)&d.data.time_write)==0)
                ::strftime(buf, 32, "%D %r", &tval);

            cout << right;
            cout << setw(SizeWidth);
            
            if (isDirectory)
            {
                // fill the SizeWidth with space.
                cout << ' ';
                cout << ' ';
            }
            else
            {
                writeColor(CS_YELLOW);
                cout << d.data.size;
                cout << ' ';
            }

            cout << left;
            writeColor(CS_LIGHT_GREY);
            cout << buf << ' ';

            if (isHidden)
                writeColor(CS_GREY);
            else if (isDirectory)
                writeColor(CS_GREEN);
            else writeColor(CS_WHITE);

            cout << subDir + d.name << '\n';
        }
        else
        {
            if (isDirectory && isHidden)
                writeColor(CS_GREY);
            else if (isDirectory)
                writeColor(CS_GREEN);
            else if (isHidden)
                writeColor(CS_GREY);
            else
                writeColor(CS_WHITE);

            if (!opts.byline)
            {
                // Find when this will write 
                // past the bounds of the window.
                if (i * (maxwidth + 2 * Padding) >= opts.winRight)
                {
                    i = 0;
                    cout << '\n';
                }
            }


            cout << setw(maxwidth);

            if (opts.byline)
            {
                if (opts.comma)
                    cout << left << subDir + d.name + ',';
                else
                    cout << left << subDir + d.name;
                cout << ' ';
            }
            else
            {
                if (opts.comma)
                    cout << left << d.name + ',';
                else
                    cout << left << d.name;
                cout << ' ';
            }

            if (opts.byline)
                cout << '\n';
            else
                ++i;
        }

        // always set it back to the default color.
        writeColor(CS_WHITE);
    }

    if (opts.recursive)
    {
        for (string dir : dirs)
            listAll(callDir, combinePath(subDir, dir, Empty), args, opts);
    }
}


void resetState()
{
    writeColor(CS_WHITE);
}


void splitPath(const string& input,
                string&       path,
                string&       pattern)
{
    size_t pos = input.find_last_of(SeperatorWin);
    if (pos != string::npos)
    {
        path = input.substr(0, pos+1);
        pattern = input.substr(pos+1, input.size());
    }
    else
    {
        path = Empty;
        pattern = input;
    }
}

string normalizePath(const string& input)
{
    string rval = input;
    size_t pos;
    while ((pos = rval.find(SeperatorNx)) != string::npos)
        rval = rval.replace(pos, 1, &SeperatorWin);
    return rval;

}


string combinePath(const string& path,
                   const string& subpath,
                   const string& search)
{
    string rval = path;
    if (!rval.empty())
    {
        if (rval.back() != SeperatorWin)
            rval += SeperatorWin;
    }
    if (!subpath.empty())
        rval += subpath;

    if (!rval.empty())
    {
        if (rval.back() != SeperatorWin)
            rval += SeperatorWin;
    }
    if (!search.empty())
        rval += search;

    return rval;
}




void writeColor(int fg, int bg)
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

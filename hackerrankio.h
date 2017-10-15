#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#ifdef _MSC_VER
#   include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
int hc_vsnprintf(char * s, size_t n, const char * format, va_list arg ) {
    return _vsnprintf_c_l(s, n, format, arg);
}
int hc_vscprintf(const char * format, va_list arg ) {
    return _vscprintf_l(format, NULL, arg);
}
int hc_vfscanf(FILE * stream, const char * format, va_list arg) {
    return _vfscanf_l(stream, format, NULL, arg);
}
int hc_vfprintf(FILE * stream, const char * format, va_list arg) {
    return _vfprintf_l(stream, format, NULL, arg);
}
#else
int hc_vsnprintf(char * s, size_t n, const char * format, va_list arg) {
    return vsnprintf(s, n, format, arg);
}
int hc_vscprintf(const char * format, va_list arg ) {
    char buf[256];
    return vsnprintf(buf, sizeof(buf), format, arg);
}
int hc_vfscanf(FILE * stream, const char * format, va_list arg) {
    return vfscanf(stream, format, arg);
}
int hc_vfprintf(FILE * stream, const char * format, va_list arg) {
    return vfprintf(stream, format, arg);
}
int fopen_s(FILE** f, const char* fn, const char* m) {
    *f = fopen(fn, m);
    return *f != NULL;
}
#endif

void debug(const char* fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    hc_vfprintf(stderr, fmt, argList);
    va_end(argList);
}



#ifdef __cplusplus
#   include <iostream>
#   include <string>
#   include <exception>
#   include <fstream>

class inputOutputOverride {
public:
    class verified_stream : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        typedef std::basic_streambuf<char, std::char_traits<char> > basestream;
        typedef basestream::char_type char_type;
        typedef basestream::int_type int_type;

        verified_stream(const char* fn)
            : _src(fn, std::ios::in)
        {
            assert(_src.is_open());
        }

        int_type sputc(char_type b) {
            char_type a = _src.get();
            assert(a == b);
            return a;
        }
        
        int_type sputbackc(char_type _Ch) {
            throw;
        }
        int_type sungetc() {
            throw;
        }

        bool is_open() {
            return _src.is_open();
        }

    protected:
        virtual int_type overflow(int_type c) {
            if (c < 0) {
                return basestream::traits_type::eof();
            } else {
                char_type a = _src.get();
                if (isspace(c) && _src.eof()) {
                    return basestream::traits_type::eof();
                }
                assert(a == (char_type)c);
                return c;
            }
        }

        virtual std::streamsize xsputn(const char_type *data, std::streamsize count) {
            if (count > 0) {
                char_type* comp = new char_type[(size_t)count];
                _src.get(comp, count);
                assert(memcmp(comp, data, (size_t)count) == 0);
                delete[] comp;
            }
            return count;
        }

        void pbump(int _Off) {
            throw;
        }
        void setp(char_type *_First, char_type *_Last) {
            throw;
        }
        void setp(char_type *_First, char_type *_Next, char_type *_Last) {
            throw;
        }

    private:
        std::fstream _src;
    };

    inputOutputOverride()
        : _input("input.txt", std::ios::in)
        , _output("output.txt")
    {
        assert(_input.is_open());
        assert(_output.is_open());

        // redirect cout, cin
        std::cin.rdbuf(_input.rdbuf());
        std::cout.rdbuf(&_output);
    }

private:
    std::fstream _input;
    verified_stream _output;
};

static inputOutputOverride g_ioOverride;

#else
static FILE* hc_inputFile = NULL;
static FILE* hc_outputFile = NULL;
static int hc_failures = 0;

int __cdecl hcprintf(char const* const format, ...)
{
    va_list argList;
    int result, length;
    va_start(argList, format);
    length = hc_vscprintf(format, argList);
    if (length > 0) {
#ifndef _MSC_VER
        va_end(argList);
        va_start(argList, format);
#endif

        char* out = NULL, * in = NULL;
        size_t readSize, readResult = 0;
        int different;

        // format the buffer
        out = (char*)malloc(length + 1);
        assert(out != NULL);
        result = hc_vsnprintf(out, length + 1, format, argList);
        assert(result >= 0);

        // read the same number of characters from the output file
        readSize = strlen(out);
        in = (char*)malloc(readSize+1);
        assert(in != NULL);
        while (!feof(hc_outputFile) && readResult < readSize) {
            size_t r = fread(in + readResult, 1, readSize - readResult, hc_outputFile);
            if (r == 0) {
                break;
            }
            readResult += r;
        }
        in[readResult] = 0;

        // trim any whitespace off the end
        if (feof(hc_outputFile) && readResult < readSize) {
            for (size_t i = readResult; i < readSize; ++i) {
                if (!isspace(out[i])) {
                    hc_failures++;
                }
            }
            readSize = readResult;
        }

        // compare the results
        assert(readResult >= readSize);
        different = memcmp(in, out, readResult) != 0;
        if (different) {
            hc_failures++;
        }

        fprintf(stdout, "%s", out);

        free(in);
        free(out);
    }

    va_end(argList);
    return result;
}

int __cdecl hcscanf(char const* const format, ...)
{
    int result;
    va_list argList;
    va_start(argList, format);
    result = hc_vfscanf(hc_inputFile, format, argList);
    va_end(argList);
    return result;
}

int hcmain();

int main() {
    fopen_s(&hc_inputFile, "input.txt", "r");
    fopen_s(&hc_outputFile, "output.txt", "r");

    debug("------\n");
    fflush(stderr);
    int r = hcmain();
    fflush(stdout);
    debug("\n------\n");

    if (hc_failures) {
        size_t size, read = 0;

        debug("There were %d mismatches. Expected output:\n", hc_failures);
        debug("------\n");
        fseek(hc_outputFile, 0, SEEK_END);
        size = ftell(hc_outputFile);
        fseek(hc_outputFile, 0, SEEK_SET);

        while (!feof(hc_outputFile) && read < size) {
            char buf[256];
            int r;
            size_t sizeToRead = size - read;

            if (sizeToRead > sizeof(buf)) {
                sizeToRead = sizeof(buf);
            }

            r = fread(buf, 1, sizeToRead, hc_outputFile);
            if (r == 0) {
                break;
            }
            buf[r] = 0;
            read += r;

            debug("%s", buf);
        }
        debug("\n------\n");
    } else {
        debug("Pass.\n");
    }

    fclose(hc_inputFile);
    fclose(hc_outputFile);
    return r;
}

#   define main hcmain
#   define scanf hcscanf
#   define printf hcprintf
#endif

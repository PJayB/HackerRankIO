#pragma once

#include <assert.h>
#include <stdarg.h>

#ifdef __cplusplus
#   include <iostream>
#   include <string>
#   include <exception>
#   include <fstream>

class inputOutputOverride {
public:
    class verified_stream : public std::basic_streambuf<char, std::char_traits<char>> {
    public:
        typedef std::basic_streambuf<char, std::char_traits<char>> basestream;
        typedef basestream::char_type char_type;
        typedef basestream::int_type int_type;

        verified_stream(const char* fn)
            : _src(fn, std::ios::in)
        {
            assert(_src.is_open());
        }

        int_type __CLR_OR_THIS_CALL sputc(char_type b) {
            char_type a = _src.get();
            assert(a == b);
            return a;
        }
        
        int_type __CLR_OR_THIS_CALL sputbackc(char_type _Ch) {
            throw;
        }
        int_type __CLR_OR_THIS_CALL sungetc() {
            throw;
        }

        bool is_open() {
            return _src.is_open();
        }

    protected:
        virtual int_type __CLR_OR_THIS_CALL overflow(int_type c) {
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

        virtual std::streamsize __CLR_OR_THIS_CALL xsputn(const char_type *data, std::streamsize count) {
            if (count > 0) {
                char_type* comp = new char_type[(size_t)count];
                _src.get(comp, count);
                assert(memcmp(comp, data, (size_t)count) == 0);
                delete[] comp;
            }
            return count;
        }

        void __CLR_OR_THIS_CALL pbump(int _Off) {
            throw;
        }
        void __CLR_OR_THIS_CALL setp(char_type *_First, char_type *_Last) {
            throw;
        }
        void __CLR_OR_THIS_CALL setp(char_type *_First, char_type *_Next, char_type *_Last) {
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
#   include <stdio.h>
#   include <ctype.h>
#   include <memory.h>
#   ifdef _MSC_VER
#      include <malloc.h>
#   endif
#   include <string.h>

static FILE* inputFile = NULL;
static FILE* outputFile = NULL;

int __cdecl hcprintf(char const* const format, ...)
{
    va_list argList;
    int result, length;
    va_start(argList, format);
    length = _vscprintf_l(format, NULL, argList);
    if (length > 0) {
        char* out = NULL, * in = NULL;
        size_t readSize, readResult = 0;

        // format the buffer
        out = (char*)malloc(length + 1);
        assert(out != NULL);
        result = _vsnprintf_c_l(out, length + 1, format, NULL, argList);
        assert(result >= 0);

        // read the same number of characters from the output file
        readSize = strlen(out);
        in = (char*)malloc(readSize);
        assert(in != NULL);
        while (!feof(outputFile) && readResult < readSize) {
            size_t r = fread(in + readResult, 1, readSize - readResult, outputFile);
            if (r == 0) {
                break;
            }
            readResult += r;
        }

        // trim any whitespace off the end
        if (feof(outputFile) && readResult < readSize) {
            for (size_t i = readResult; i < readSize; ++i) {
                assert(isspace(out[i]));
            }
            readSize = readResult;
        }

        // compare the results
        assert(readResult >= readSize);
        assert(memcmp(in, out, readResult) == 0);

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
    result = _vfscanf_l(inputFile, format, NULL, argList);
    va_end(argList);
    return result;
}

int hcmain();

int main() {
    fopen_s(&inputFile, "input.txt", "r");
    fopen_s(&outputFile, "output.txt", "r");

    int r = hcmain();

    fclose(inputFile);
    fclose(outputFile);
    return r;
}

#   define main hcmain
#   define scanf hcscanf
#   define printf hcprintf
#endif

void debug(const char* fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    _vfprintf_l(stdout, fmt, NULL, argList);
    va_end(argList);
}

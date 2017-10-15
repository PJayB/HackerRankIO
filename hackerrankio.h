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

int hc_checkTrailingOutput() {
    // If the output file isn't at the end, it's likely we're missing some output
    // Only reject non-whitespace characters
    while (!feof(hc_outputFile)) {
        char buf[32];
        int read = fread(buf, 1, sizeof(buf), hc_outputFile);
        if (read > 0) {
            for (int i = 0; i < read; ++i) {
                if (!isspace(buf[i])) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int main() {
    fopen_s(&hc_inputFile, "input.txt", "r");
    fopen_s(&hc_outputFile, "output.txt", "r");

    debug("------\n");
    fflush(stderr);
    int r = hcmain();
    fflush(stdout);
    debug("\n------\n");

    hc_failures += hc_checkTrailingOutput();

    if (hc_failures) {
        size_t size, read = 0;

        debug("Output is divergent from truth. Expected output:\n");
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

#define main hcmain
#define scanf hcscanf
#define printf hcprintf

#ifdef __cplusplus
#   include <iostream>
#   include <string>
#   include <exception>
#   include <fstream>
#   include <vector>

class inputOutputOverride {
public:
    class verified_stream : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        typedef std::basic_streambuf<char, std::char_traits<char> > basestream;
        typedef basestream::char_type char_type;
        typedef basestream::int_type int_type;

        verified_stream() {}

    protected:
        virtual int_type overflow(int_type c) {
            if (c < 0) {
                return basestream::traits_type::eof();
            } else {
                hcprintf("%c", c);
                return c;
            }
        }

        virtual std::streamsize xsputn(const char_type *data, std::streamsize count) {
            if (count > 0) {
                _buf.resize(count+1);
                memcpy(_buf.data(), data, count);
                _buf[count] = 0;
                hcprintf("%s", _buf.data());
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
        std::vector<char_type> _buf;
    };

    class virtual_input_stream : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        typedef std::basic_streambuf<char, std::char_traits<char> > basestream;
        typedef basestream::char_type char_type;
        typedef basestream::int_type int_type;

      /**
       *  @brief  Alters the stream positions.
       *
       *  Each derived class provides its own appropriate behavior.
       *  @note  Base class version does nothing, returns a @c pos_type
       *         that represents an invalid stream position.
      */
      virtual pos_type 
      seekoff(off_type, std::ios_base::seekdir,
	      std::ios_base::openmode /*__mode*/ = std::ios_base::in | std::ios_base::out)
      { throw; } 

      /**
       *  @brief  Alters the stream positions.
       *
       *  Each derived class provides its own appropriate behavior.
       *  @note  Base class version does nothing, returns a @c pos_type
       *         that represents an invalid stream position.
      */
      virtual pos_type 
      seekpos(pos_type, 
	      std::ios_base::openmode /*__mode*/ = std::ios_base::in | std::ios_base::out)
      { throw; } 

    protected:
        virtual std::streamsize showmanyc() { 
            long c = ftell(hc_inputFile);
            fseek(hc_inputFile, 0, SEEK_END);
            long e = ftell(hc_inputFile);
            fseek(hc_inputFile, c, SEEK_SET);
            return e - c; 
        }

        virtual int_type underflow() {
            if (feof(hc_inputFile)) {
                return traits_type::eof(); 
            } else {
                char_type p;
                if (hcscanf("%c", &p) == 0) {
                    return traits_type::eof();
                }
                debug("underflow: %c\n", p);
                return p;
            }
        }

        virtual int_type uflow() {
            return underflow();
        }        

        virtual std::streamsize xsgetn(char_type *data, std::streamsize count) {
            std::streamsize read = 0;
            while (read < count && !feof(hc_inputFile)) {
                int r = fread(data + read, 1, (size_t)(count - read), hc_inputFile);
                if (r == 0) {
                    break;
                }
                read += r;
            }
            return read;
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

        virtual int_type pbackfail(int_type /* __c */  = traits_type::eof()) {
            throw;
        }
    };

    inputOutputOverride()
    {
        // redirect cout, cin
        std::cin.rdbuf(&_input);
        std::cout.rdbuf(&_output);
    }

private:
    virtual_input_stream _input;
    verified_stream _output;
};

static inputOutputOverride g_ioOverride;
#endif

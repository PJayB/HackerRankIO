HackerRank I/O
--------------

Enables offline development and verification of HackerRank solutions written in C or C++. It is designed such that there are as few code changes needed as possible.

This header redirects `scanf` or `std::cin` so that your HackerRank code will be read from `input.txt` instead of from the keyboard. 

Similarly, `printf` and `std::cout` are intercepted so that everything you print will be compared to the contents of `output.txt`. An assert is thrown when your input does not match the expected results.

License
=======
Licensed under the "I don't give a sith" license. No warranty is implied.


Usage
=====
At the top of your HackerRank solution, include this header. For best results, guard this with something platform specific so that you can simply copy and paste your solution into the HackerRank text editor without modifications.

For example, on Windows:

```cpp
#ifdef _WIN32
#	include "../hackerrankio/hackerrankio.h"
#endif
```

The header also provides an alternative to `printf`: `debug`, which will always print text to `stdout`.



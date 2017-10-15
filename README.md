HackerRank I/O
==============

Enables offline development and verification of HackerRank solutions written in C or C++. It is designed with minimal code changes in mind.

This header redirects `scanf` or `std::cin` so that your HackerRank code will be read from `input.txt` instead of from the keyboard. 

Similarly, `printf` and `std::cout` are intercepted so that everything you print will be compared to the contents of `output.txt`. An assert is thrown when your input does not match the expected results.

License
-------
Licensed under the "I don't give a sith" license. No warranty is implied.


Integrating HackerRankIO
------------------------
At the top of your HackerRank solution, include this header. For best results, guard this with something HackerRank doesn't define (e.g. something platform specific, or something custom via `-D` or `/D`) so that you can simply copy and paste your solution into the HackerRank text editor without modifications.

For example, on Windows:

```cpp
#ifdef HACKERRANKIO
#	include "../hackerrankio/hackerrankio.h"
#endif
```

Because the header redirects `printf` and `std::cout`, the header provides an alternative for printing to the `stdout`: 

`void debug(const char*, ...);`


Running Your Solution
---------------------
In the current directory, create two text files: 

* `input.txt` should contain the sample input provided by HackerRank
* `output.txt` should contain the sample output provided by HackerRank

Create, build and debug your solution in your favorite programming environment. For example, GCC:

```sh
gcc -DHACKERRANK -o my-solution my-solution.c
./my-solution
```

If your built solution runs to completion, your solution passed the given test case. If it asserts, the output from your solution diverged from `output.txt`.


Example
-------
This example is taken from https://www.hackerrank.com/challenges/missing-numbers.

### Sample Input in `input.txt`
```
10
203 204 205 206 207 208 203 204 205 206
13
203 204 204 205 206 207 205 208 203 206 205 206 204
```

### Sample Output in `output.txt`
```
204 205 206
```

### Source in `missing-numbers.c`
```c
#ifdef HACKERRANKIO
#   include "../../../hackerrankio/hackerrankio.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_DELTA 101

int main() {
    int n, m;
    int xmin = 10000;
    int *a, *b, d[MAX_DELTA];

    scanf("%d", &n);
    a = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; ++i) {
        scanf("%d", &a[i]);
    }

    scanf("%d", &m);
    b = (int*)malloc(m * sizeof(int));
    for (int i = 0; i < m; ++i) {
        scanf("%d", &b[i]);
        if (b[i] < xmin)
            xmin = b[i];
    }

    memset(d, 0, sizeof(d));
    for (int i = 0; i < n; ++i) {
        d[a[i] - xmin]--;
    }
    for (int i = 0; i < m; ++i) {
        d[b[i] - xmin]++;
    }
    for (int i = 0; i < MAX_DELTA; ++i) {
        if (d[i] != 0)
            printf("%d ", xmin + i);
    }

    return 0;
}
```

### Running the Solution
```sh
> gcc -DHACKERRANK -o missing-numbers missing-numbers.c
> ./missing-numbers
>
```

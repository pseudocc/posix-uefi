POSIX-UEFI
==========

<blockquote>We hate that horrible and ugly UEFI API, we want POSIX!</blockquote>

This is a very small build environment that helps you to develop for UEFI under Linux (and other POSIX systems). It was
greatly inspired by [gnu-efi](https://sourceforge.net/projects/gnu-efi) (big big kudos to those guys), but it is a lot
smaller, easier to integrate (works with Clang and GNU gcc both) and easier to use because it provides a POSIX like API
for your UEFI application.

An UEFI environment consist of two parts: a firmware with GUID protocol interfaces and a user library. We cannot change
the former, but we can make the second friendlier. That's what POSIX-UEFI does for your application. It is a small API
wrapper library around the GUID protocols, not a fully blown POSIX compatible libc implementation.

You have two options on how to integrate it into your project:

Distributing as Static Library
------------------------------

In the `uefi` directory, run
```sh
$ make
```
This will create `build/uefi` with all the necessary files in it. These are:

 - **crt0.o**, the run-time that bootstraps POSIX-UEFI
 - **link.ld**, the linker script you must use with POSIX-UEFI (same as with gnu-efi)
 - **libuefi.a**, the library itself
 - **uefi.h**, the all-in-one C / C++ header

You can use this and link your application with it, but you won't be able to recompile it, plus you're on your own with
the linking and converting.

Strictly speaking you'll only need **crt0.o** and **link.ld**, that will get you started and will call your application's
"main()", but to get libc functions like memcmp, strcpy, malloc or fopen, you'll have to link with **libuefi.a**.

Distributing as Source
----------------------

This is the preferred way, as it also provides a Makefile to set up your toolchain properly.

 1. simply copy the `uefi` directory into your source tree (or set up a git submodule). A dozen files, about 128K in total.
 2. create an extremely simple **Makefile** like the one below
 3. compile your code for UEFI by running `make`

```
TARGET = helloworld.efi

include uefi/Makefile
```
An example **helloworld.c** goes like this:
```c
#include <uefi.h>

int main(int argc, char **argv)
{
    printf("Hello World!\n");
    return 0;
}
```
By default it uses Clang + lld, and PE is generated without conversion. If `USE_GCC` is set, then the host native's GNU gcc + ld
used to create a shared object and get converted into an .efi file.

If you comment out `USE_UTF8` in uefi.h, then all character representation will use `wchar_t`, and there will be no string
conversion between your application and the UEFI interfaces. This also means you must use `L""` and `L''` literals everywhere,
and you main would receive `wchar_t **argv`.

### Available Makefile Options

| Variable   | Description                                                                                          |
|------------|------------------------------------------------------------------------------------------------------|
| `TARGET`   | the target application (required)                                                                    |
| `SRCS`     | list of source files you want to compile (defaults to \*.c \*.S)                                     |
| `CFLAGS`   | compiler flags you want to use (empty by default, like "-Wall -pedantic -std=c99")                   |
| `LDFLAGS`  | linker flags you want to use (I don't think you'll ever need this, just in case)                     |
| `LIBS`     | additional libraries you want to link with (like "-lm", only static .a libraries allowed)            |
| `USE_GCC`  | set this if you want native GNU gcc + ld + objccopy instead of LLVM Clang + Lld                      |
| `ARCH`     | the target architecture                                                                              |

Here's a more advanced **Makefile** example:
```
ARCH = x86_64
TARGET = helloworld.efi
SRCS = $(wildcard *.c)
CFLAGS = -pedantic -Wall -Wextra -Werror --std=c11 -O2
LDFLAGS =
LIBS = -lm

USE_GCC = 1
include uefi/Makefile
```
The build environment configurator was created in a way that it can handle any number of architectures, however
only `x86_64` crt0 has been throughfully tested for now. There's an `aarch64` crt0 too, but since I don't have
an ARM UEFI board, it hasn't been tested on real machine. Should work though.

Notable Differences to POSIX libc
---------------------------------

This library is nowhere near as complete as glibc or musl for example. It only provides the very basic libc functions
for you, because simplicity was one of its main goals. It is the best to say this is just wrapper around the UEFI API,
rather than a POSIX compatible libc.

All strings in the UEFI environment are stored with 16 bits wide characters. The library provides `wchar_t` type for that,
and the `USE_UTF8` define to convert between `char` and `wchar_t` transparently. If you comment out `USE_UTF8`, then for
example your main() will NOT be like `main(int argc, char **argv)`, but `main(int argc, wchar_t **argv)` instead. All
the other string related libc functions (like strlen() for example) will use this wide character type too. For this reason,
you must specify your string literals with `L""` and characters with `L''`. To handle both configurations, `char_t` type is
defined, which is either `char` or `wchar_t`, and the `CL()` macro which might add the `L` prefix to constant literals.
Functions that supposed to handle characters in int type (like `getchar`, `putchar`), do not truncate to unsigned char,
rather to wchar_t.

File types in dirent are limited to directories and files only (DT_DIR, DT_REG), but for stat in addition to S_IFDIR and
S_IFREG, S_IFIFO also returned (for console streams: stdin, stdout, stderr).

Note that `getenv` and `setenv` aren't POSIX standard, because UEFI environment variables are binary blobs.

That's about it, everything else is the same.

List of Provided POSIX Functions
--------------------------------

### dirent.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| opendir       | as usual, but might accept wide char strings                               |
| readdir       | as usual                                                                   |
| rewinddir     | as usual                                                                   |
| closedir      | as usual                                                                   |

Because UEFI has no concept of device files nor of symlinks, dirent fields are limited and only DT_DIR and DT_REG supported.

### stdlib.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| atoi          | as usual, but might accept wide char strings and understands "0x" prefix   |
| atol          | as usual, but might accept wide char strings and understands "0x" prefix   |
| strtol        | as usual, but might accept wide char strings                               |
| malloc        | as usual                                                                   |
| calloc        | as usual                                                                   |
| realloc       | as usual (needs testing)                                                   |
| free          | as usual                                                                   |
| abort         | as usual                                                                   |
| exit          | as usual                                                                   |
| exit_bs       | leave this entire UEFI bullshit behind (exit Boot Services)                |
| mbtowc        | as usual (UTF-8 char to wchar_t)                                           |
| wctomb        | as usual (wchar_t to UTF-8 char)                                           |
| mbstowcs      | as usual (UTF-8 string to wchar_t string)                                  |
| wcstombs      | as usual (wchar_t string to UTF-8 string)                                  |
| srand         | as usual                                                                   |
| rand          | as usual, but uses EFI_RNG_PROTOCOL if possible                            |
| getenv        | pretty UEFI specific                                                       |
| setenv        | pretty UEFI specific                                                       |

```c
int exit_bs();
```
Exit Boot Services. Returns 0 on success.

```c
uint8_t *getenv(char_t *name, uintn_t *len);
```
Query the value of environment variable `name`. On success, `len` is set, and a malloc'd buffer returned. It is
the caller's responsibility to free the buffer later. On error returns NULL.
```c
int setenv(char_t *name, uintn_t len, uint8_t *data);
```
Sets an environment variable by `name` with `data` of length `len`. On success returns 1, otherwise 0 on error.

### stdio.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| fopen         | as usual, but might accept wide char strings, also for mode                |
| fclose        | as usual                                                                   |
| fflush        | as usual                                                                   |
| fread         | as usual, only real files accepted (no stdin)                              |
| fwrite        | as usual, only real files accepted (no stdout nor stderr)                  |
| fseek         | as usual, only real files accepted (no stdin, stdout, stderr)              |
| ftell         | as usual, only real files accepted (no stdin, stdout, stderr)              |
| feof          | as usual, only real files accepted (no stdin, stdout, stderr)              |
| fprintf       | as usual, might be wide char strings, max BUFSIZ, files, stdout, stderr    |
| printf        | as usual, might be wide char strings, max BUFSIZ, stdout only              |
| sprintf       | as usual, might be wide char strings, max BUFSIZ                           |
| vfprintf      | as usual, might be wide char strings, max BUFSIZ, files, stdout, stderr    |
| vprintf       | as usual, might be wide char strings, max BUFSIZ                           |
| vsprintf      | as usual, might be wide char strings, max BUFSIZ                           |
| snprintf      | as usual, might be wide char strings                                       |
| vsnprintf     | as usual, might be wide char strings                                       |
| getchar       | as usual, blocking, stdin only (no stream redirects), returns UNICODE      |
| getchar_ifany | non-blocking, returns 0 if there was no key press, UNICODE otherwise       |
| putchar       | as usual, stdout only (no stream redirects)                                |

File open modes: `"r"` read, `"w"` write, `"a"` append. Because of UEFI peculiarities, `"wd"` creates directory.

String formating is limited; only supports padding via number prefixes, `%d`, `%x`, `%X`, `%c`, `%s`, `%q` and
`%p`. When `USE_UTF8` is not defined, then formating operates on wchar_t, so it also supports the non-standard `%S`
(printing an UTF-8 string), `%Q` (printing an escaped UTF-8 string). These functions don't allocate memory, but in
return the total length of the output string cannot be longer than `BUFSIZ` (8k if you haven't defined otherwise),
except for the variants which have a maxlen argument. For convenience, `%D` requires `efi_physical_address_t` as
argument, and it dumps memory, 16 bytes or one line at once. With the padding modifier you can dump more lines, for
example `%5D` gives you 5 lines (80 dumped bytes).

Special "device files" you can open:

| Name                | Description                                        |
|---------------------|----------------------------------------------------|
| `/dev/stdin`        | returns ST->ConIn                                  |
| `/dev/stdout`       | returns ST->ConOut, fprintf                        |
| `/dev/stderr`       | returns ST->StdErr, fprintf                        |
| `/dev/serial(baud)` | returns Serial IO protocol, fread, fwrite, fprintf |
| `/dev/disk(n)`      | returns Block IO protocol, fread, fwrite           |

With disk devices, `fread` and `fwrite` arguments look like this (because UEFI can't handle position internally), and returned
size is rounded to block size:
```c
size_t fread(ptr, buffer size, lba number, stream);
size_t fwrite(ptr, buffer size, lba number, stream);
```
To interpret a GPT, there are typedefs like `efi_partition_table_header_t` and `efi_partition_entry_t` which you can point
to the read data.

### string.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| memcpy        | as usual, works on bytes                                                   |
| memmove       | as usual, works on bytes                                                   |
| memset        | as usual, works on bytes                                                   |
| memcmp        | as usual, works on bytes                                                   |
| memchr        | as usual, works on bytes                                                   |
| memrchr       | as usual, works on bytes                                                   |
| memmem        | as usual, works on bytes                                                   |
| memrmem       | as usual, works on bytes                                                   |
| strcpy        | might work on wide char strings                                            |
| strncpy       | might work on wide char strings                                            |
| strcat        | might work on wide char strings                                            |
| strncat       | might work on wide char strings                                            |
| strcmp        | might work on wide char strings                                            |
| strncmp       | might work on wide char strings                                            |
| strdup        | might work on wide char strings                                            |
| strchr        | might work on wide char strings                                            |
| strrchr       | might work on wide char strings                                            |
| strstr        | might work on wide char strings                                            |
| strtok        | might work on wide char strings                                            |
| strtok_r      | might work on wide char strings                                            |
| strlen        | might work on wide char strings                                            |

### sys/stat.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| stat          | as usual, but might accept wide char strings                               |
| fstat         | UEFI doesn't have fd, so it uses FILE\*                                    |
| mkdir         | as usual, but might accept wide char strings, and mode unused              |

Because UEFI has no concept of device major and minor number nor of inodes, struct stat's fields are limited.

### time.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| localtime     | argument unused, always returns current time in struct tm                  |
| mktime        | as usual                                                                   |
| time          | as usual                                                                   |

### unistd.h

| Function      | Description                                                                |
|---------------|----------------------------------------------------------------------------|
| usleep        | the usual                                                                  |
| sleep         | the usual                                                                  |
| unlink        | as usual, but accepts wide char strings                                    |
| rmdir         | as usual, but accepts wide char strings                                    |

Accessing UEFI Services
-----------------------

It is very likely that you want to call UEFI specific functions directly. For that, POSIX-UEFI specifies some globals
in `uefi.h`:

| Global Variable | Description                                              |
|-----------------|----------------------------------------------------------|
| `*BS`           | *efi_boot_services_t*, pointer to the Boot Time Services |
| `*RT`           | *efi_runtime_t*, pointer to the Runtime Services         |
| `*ST`           | *efi_system_table_t*, pointer to the UEFI System Table   |
| `IM`            | *efi_handle_t* of your Loaded Image                      |

The EFI structures, enums, typedefs and defines are all converted to ANSI C standard POSIX style, for example
BOOLEAN ->  boolean_t, UINTN -> uintn_t, EFI_MEMORY_DESCRIPTOR -> efi_memory_descriptor_t, and of course
EFI_BOOT_SERVICES -> efi_boot_services_t.

Calling UEFI functions is as simple as with EDK II, just do the call, no need for "uefi_call_wrapper":
```c
    ST->ConOut->OutputString(ST->ConOut, L"Hello World!\r\n");
```
(Note: unlike with printf, with OutputString you must use `L""` and also print carrige return `L"\r"` before `L"\n"`. These
are the small things that POSIX-UEFI does for you to make your life comfortable.)

There are two additional, non-POSIX calls in the library. One is `exit_bs()` to exit Boot Services, and the other is
a non-blocking version `getchar_ifany()`.

Unlike gnu-efi, POSIX-UEFI does not pollute your application's namespace with unused GUID variables. It only provides
header definitions, so you must create each GUID instance if and when you need them.

Example:
```c
efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
efi_gop_t *gop = NULL;

status = BS->LocateProtocol(&gopGuid, NULL, (void**)&gop);
```

Also unlike gnu-efi, POSIX-UEFI does not provide standard EFI headers. It expects that you have installed those under
/usr/include/efi from EDK II or gnu-efi, and POSIX-UEFI makes it possible to use those system wide headers without
naming conflicts. POSIX-UEFI itself ships the very minimum set of typedefs and structs (with POSIX-ized names).
```c
#include <efi.h>
#include <uefi.h> /* this will work as expected! Both POSIX-UEFI and EDK II / gnu-efi typedefs available */
```
The advantage of this is that you can use the simplicity of the POSIX-UEFI library and build environment, while getting
access to the most up-to-date protocol and interface definitions at the same time.

License
-------

POSIX_UEFI is licensed under the terms of the MIT license.

Cheers,

bzt

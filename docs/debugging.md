# Why is it so hard?

DTIO is a multiprocess system which also relies on LD\_PRELOAD
interception of I/O calls. Typical debuggers like GDB are single
process, and it's not very clear where to put them if you're doing an
LD\_PRELOAD. In addition, LD\_PRELOAD intercepts calls, and it can be
quite unclear which calls are being intercepted.

## Logging

DTIO uses spdlog for logging. It sets a LOG\_LEVEL in the
CMakeLists.txt that should define how much logging information to
show. INFO is the default. If something is going wrong, DEBUG is
intended to help figure out why. TRACE is exhaustive, and only
recommended in the case where you don't even know where something is
failing or want to understand everything the system is doing.

## Parallel Debugging

Parallel debugging is possible, you have to get the process to print
its PID and wait, then connect your GDB to that particular pid with
`gdb -p [pid]`. DTIO doesn't implement anything to assist with
parallel debugging at the moment, and in most cases the logging
mechanisms should be sufficient to avoid this.

## Debugging Interception

First, know that you can use valgrind or GDB on a process that uses
LD\_PRELOAD. Normally, it could cause errors because valgrind and GDB
both use I/O libraries like POSIX, but DTIO tries to avoid any
problems by limiting interception where appropriate, so there
shouldn't be any interception outside of the application.

Using GDB on a program with interception works like this:
`./a.out arg1 arg2 ...` becomes
`LD\_PRELOAD=/path/to/libdtio\_posix\_interception.so gdb --args
./a.out arg1 arg2 ...`

Valgrind works similarly:
`./a.out arg1 arg2 ...` becomes
`LD\_PRELOAD=/path/to/libdtio\_posix\_interception.so valgrind ./a.out
arg1 arg2 ...`

Sometimes, this isn't enough. For example, you may want to know which
symbols a library provides. The command for this is:
`nm -gDC libdtio\_posix\_interception.so` 
The -C helps with C++ which we do use in DTIO.

I also recommend the `ldd` command, which lists which libraries a
library is linked to. This can be useful when debugging a library.

The other thing you might want to know is that `LD\_PRELOAD` itself
provides useful debugging features. Try this:
`./a.out arg1 arg2 ...` becomes
`LD\_DEBUG=all LD\_DEBUG\_OUTPUT=debug.txt
LD\_PRELOAD=/path/to/libdtio\_posix\_interception.so ./a.out arg1 arg2
...`
You'll get detailed information about which symbol lookups are begin
made by your process and in which libraries. There's also an
`LD\_DEBUG=help` feature which lists the available options there,
though `all` is the most comprehensive one. You almost always want to
use an output file here, because the information is detailed.

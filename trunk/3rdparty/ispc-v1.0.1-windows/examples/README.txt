====================
ISPC Examples README
====================

This directory has a number of sample ispc programs.  Before building them
(on an system), install the appropriate ispc compiler binary into a
directory in your path.  Then, if you're running Windows, open the
"examples.sln" file and built from there.  For building under Linux/OSX,
there are makefiles in each directory that build the examples individually.

Almost all of them benchmark ispc implementations of the given computation
against regular serial C++ implementations, printing out a comparison of
the runtimes and the speedup delivered by ispc.  It may be instructive to
do a side-by-side diff of the C++ and ispc implementations of these
algorithms to learn more about wirting ispc code.
 
AOBench
=======

This is an ISPC implementation of the "AO bench" benchmark
(http://syoyo.wordpress.com/2009/01/26/ao-bench-is-evolving/).  The command
line arguments are:

ao (num iterations) (x res) (yres)

It executes the program for the given number of iterations, rendering an
(xres x yres) image each time and measuring the computation time with both
serial and ispc implementations.

AOBench_Instrumented
====================

This version of AO Bench is compiled with the --instrument ispc compiler
flag.  This causes the compiler to emit calls to a (user-supplied)
ISPCInstrument() function at interesting places in the compiled code.  An
example implementation of this function that counts the number of times the
callback is made and records some statistics about control flow coherence
is provided in the instrument.cpp file.

*** Note: on Linux, this example currently hits an assertion in LLVM during
*** compilation

Mandelbrot
==========

Mandelbrot set generation.  This example is extensively documented at the
http://ispc.github.com/example.html page.

Mandelbrot_tasks
================

Implementation of Mandelbrot set generation that also parallelizes across
cores using tasks.  Under Windows, a simple task system built on
Microsoft's Concurrency Runtime is used (see tasks_concrt.cpp).  On OSX, a
task system based on Grand Central Dispatch is used (tasks_gcd.cpp), and on
Linux, a pthreads-based task system is used (tasks_pthreads.cpp).  When
using tasks with ispc, no task system is mandated; the user is free to plug
in any task system they want, for ease of interoperating with existing task
systems.
 
Options
=======

This program implements both the Black-Scholes and Binomial options pricing
models in both ispc and regular serial C++ code.

RT
==

This is a simple ray tracer; it reads in camera parameters and a bounding
volume hierarchy and renders the scene from the given viewpoint.  The
command line arguments are:

rt <scene name base>

Where <scene base name> is one of "cornell", "teapot", or "sponza".

The implementation originally derives from the bounding volume hierarchy
and triangle intersection code from pbrt; see the pbrt source code and/or
"Physically Based Rendering" book for more about the basic algorithmic
details.

Simple
======

This is a simple "hello world" type program that shows a ~10 line
application program calling out to a ~5 line ispc program to do a simple
computation.

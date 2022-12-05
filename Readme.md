# SegPredict-ImageSegment

This project was formerly developed by a dedicated team and is now maintained and improved by RuneScaping.

## Contributors:

* RuneScaping
* Yunpeng Li
* Pol Monsó Purtí

---------------------------------------------------------------------------------

# How to compile the code #

Follow this step-by-step guide to compile the code and run it on your system. It is designed for Linux systems but it can also be run on Windows or Mac OS.

1. Copy the source code from the web.
2. Make sure you have cmake installed on your system.
3. Compile third-party libraries.

Detailed instructions are available for compiling on **UNIX** and **Windows**.

4. Edit the variables in the "THIRD-PARTY LIBRARIES" section in `CMakeLists_common.txt`.
5. Edit the config file. This program relies on a configuration file containing all parameters needed for training or applying the algorithm to a new dataset.
6. Running the code. Look at `howto.pdf`

---------------------------------------------------------------------------------

## Additional information for developers ##

- Multithreading

For ssvm, edit `svm_struct_globals.h` and change `NTHREADS`
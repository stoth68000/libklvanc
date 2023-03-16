# Introduction

Libklvanc is a library which can be used for parsing/generation of Vertical
Ancillary Data (VANC) commonly found in the Serial Digital Interface (SDI) wire protocol.

The library includes a general parser/decoder and an encoder for
VANC lines, as well as the ability to both decode and generate protocols
commonly found in SDI, including:
- SMPTE ST 334 - CEA-708 closed captions in VANC
- SMPTE ST 2016 Active Format Descriptor (AFD) and Bar Data
- SCTE-104 Ad triggers
- SMPTE ST 2038 arbitrary VANC encapsulation
- SMPTE ST 12-2 Timecodes
- SMPTE RDD 8 Subtitle Distribution packets

By providing both encoders and decoders, the library can be used for common
use cases involving both capture of SDI (and subsequent decoding) as well as
generation of VANC for inclusion in an SDI output interface.  This includes
computing/validating checksums at various levels and dealing with subtle edge
cases related to VANC line formatting such as ensuring packets are contiguous.
 
The library also provides utility functions for various colorspaces VANC
may be represented in, including the V210 format typically used by
BlackMagic Decklink SDI cards.

Beyond this Readme.MD file, API level documentation can be generated via
Doxygen (see "Making Documentation" section below).

Users should refer to the "tools" subdirectory for some test code which
which make use of the library.  For a "real world" application, refer to
the klvanc-tools package, which make use of libklvanc in conjunction with
Blackmagic cards to capture real SDI signals.

https://github.com/stoth68000/klvanc-tools

# LICENSE

	LGPL-V2.1
	See the included lgpl-2.1.txt for the complete license agreement.

## Dependencies
* Doxygen (if generation of API documentation is desired)

## Compilation

        ./autogen.sh --build
        ./configure --enable-shared=no
        make

## Making Documentation:
To make doxygen documentation in the doxygen folder, run the following command:

        make docs

To view the documentation, cd into the doxygen/html/ directory and open the index.html file in a browser window.
## Verifying the compilation:
The library comes with a series of test cases which allow the user to confirm
the library is working appropriately (as well as allowing us to watch for
regressions).

To run the tests after compilation, run the following command:

        make test

## Compilation with meson

Meson can be used as an alternative to autotools:

        meson build
        ninja -C build

The documentation will be built automatically if Doxygen is available

## Running tests with meson

        meson test -C build

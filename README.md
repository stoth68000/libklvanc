# Introduction

Libklvanc is a library which can be used for parsing/generation of Vertical
Ancillary Data (VANC) commonly found in the Serial Digital Interface (SDI) wire protocol.

The library includes a general parser/decoder and an encoder for
VANC lines, as well as the ability to both decode and generate protocols
commonly found in SDI, including:
- CEA-708 closed captions
- Active Format Descriptor (AFD)
- SCTE-104 Ad triggers
- SMPTE 2038 arbitrary VANC encapsulation

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

Users should refer to the "tools" subdirectory for some example applications
which make use of the library.  Note that these tools depend on the Decklink
API headers since they generally interact with a real SDI card.  The library
itself has no dependency on the Decklink API, but the example tools do.

# LICENSE

	LGPL-V2.1
	See the included lgpl-2.1.txt for the complete license agreement.

## Dependencies
* ncurses (optional)
* zlib-dev
* Doxygen (if generation of API documentation is desired)

## Compilation
    ./autogen.sh --build
    ./configure --enable-shared=no
    make

## Making Documentation:
To make doxygen documentation in the doxygen folder, run the following command:

        make docs

To view the documentation, cd into the doxygen/html/ directory and open the index.html file in a browser window.


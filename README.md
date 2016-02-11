StripZIP
========

StripZIP is a utility that removes metadata from ZIP files. Specifically it
targets any metadata that would prevent a ZIP file from being used as part of
a repeatable build process.

Instead of manipulating the timestamp and owner information for each file
before packaging into a ZIP, package files normally and then run stripzip on
the ZIP file.

Features
--------

- Remove date and time information from ZIP central directory listing
- Zero extended metadata for ZIP extended headers
- Complain about extended metadata headers it doesn't understand

Motivation
----------

Repeatable (aka reproducible, deterministic) builds are key to managing
binaries that may be built on a multitude of systems. The invariant property of
this type of build is that each artifact of the build process will have an
identical hash provided they were built from the same source files. This allows
fast identification of artifacts that have changed.

Installation
------------

StripZIP is currently build using GCC and make. It has no external
dependencies. It does not currently have an installer; when running make the
emitted binary will be in the source folder.

To build:

    $ make

Usage
-----

    $ zip archive.zip -r folder_of_stuff
    $ stripzip archive.zip

Notes:
 - Currently StripZIP will modify the archive in place
 - ZIP on Linux will add extra metadata which although StripZIP can clean so
   that builds are repeatable on the same machine, it's better not to add it at
   all. In this case, it's better to run ZIP with the `-X` or `--no-extra`
   flags.

Contribute
----------

- Issue Tracker: github.com/zeeaero/stripzip/issues
- Source Code: github.com/zeeaero/stripzip

Support
-------

If you are having issues, please let us know via the issue tracker.

License
-------

The project is licensed under the BSD license.


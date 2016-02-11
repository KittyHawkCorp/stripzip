StripZIP
========

StripZIP is a utility that removes metadata from ZIP files. Specifically it
targets any metadata that would prevent a ZIP file from being used as part of
a repeatable build process.

Instead of manipulating the timestamp and owner information for each file
before packaging into a ZIP; package files normally and then run stripzip on
the ZIP file.

Features
--------

- Remove date and time information from ZIP central directory listing
- Zero extended metadata for ZIP extended headers
- Complain about extended metadata headers it doesn't understand

Installation
------------

StripZIP is currently build using GCC and make. It has no external
dependencies. It does not currently have an installer; when running make the
emitted binary will be in the source folder.

To build:
    $ make

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


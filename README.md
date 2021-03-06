# OpenIO Data backends

The present repository provides several tools:

A set of libraries:
* **oio-utils**: miscellaneous utility tools, e.g. checksum functions, network management, http parsing, etc.
* **oio-http-parser**: Wraps the [http-parser](https://github.com/nodejs/http-parser)
* **oio-data**: an API for Blob store into OpenIO
* **oio-data-kinetic**: a Kinetic blob store implementation (cf. [KOSP](https://www.openkinetic.org/))
* **oio-data-http**: an HTTP blob store implementation
* **oio-data-rawx**: a specialization of oio-data-http, with more constraints on the fields
* **oio-data-ec**: a wrapper of other data backends, performing Erasure Coding

A set of binary CLI tools
* **kinetic-stress-put**: stress a kinetic drive with a massive PUT load and a configurable naming of the keys (random, ascending, decreasing)
* **oio-kinetic-proxy**: a minimal coroutine-based HTTP server wrapping the **oio-kinetic-client**.
* **oio-kinetic-listener**: listens to the multicast UDP announces of kinetic drives and register those in the configured conscience.
* **oio-rawx**: an ersatz of the officiel OpenIO SDS rawx service, used to validate the design. Also coroutine-based.
* **oio-ec-proxy**: an ersatz of the official OpenIO ecd. Also coroutine-based

## Status

This repository is under an active development effort and is subject to frequent changes.

## License

Copyright (C) 2016 OpenIO SAS

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](http://www.gnu.org/licenses/lgpl-3.0)
The content under **./src** and **./tests** is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 3.0 of the License, or (at your option) any later version.
These libraries are distributed in the hope that they will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public License along with this library. See the [LICENCE.lgpl3](./LICENCE.lgpl3) file.

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](http://www.gnu.org/licenses/agpl-3.0)
The content under **./bin** is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.
You should have received a copy of the GNU Affero General Public License along with this program. See the [LICENSE.agpl3](./LICENSE.agpl3) file.

## Dependencies

* libcrypto: for MD5 and SHA computations, coming from your favorite SSL vendor
* libattr: to handle extended attributes, necesseray for the rawx backend
* [protobuf](https://github.com/google/protobuf): Google's Protocol Buffers, necessary for the Kinetic Protocol. 
* [ragel](https://github.com/colmnet/ragel): Used as a lexer to easily and efficiently recognize HTTP headers.
* [http-parser](https://github.com/nodejs/http-parser) included as a Git submodule
* [the Kinetic protocol](https://github.com/Kinetic/kinetic-protocol) included as a Git submodule, a Protocol Buffer definition maintained by the Kinetic Open Storage group.
* [the Kinetic Java code](https://github.com/Kinetic/kinetic-java) included as a Git submodule, used when testing has been enabled, for the simulator it embeds.
* [libmill](https://github.com/sustrik/libmill): for easy cooperative concurrency, thanks to Martin Sustrik's coroutines.
* [glog](https://github.com/google/glog)
* [gtest](https://github.com/google/googletest)
* [gflags](https://github.com/gflags/gflags)
* [liberasurecode](https://github.com/openstack/liberasurecode)
* [rapidjson](http://rapidjson.org/)


## Build & Install

Get the sources and all the dependencies

    git clone https://github.com/open-io/oio-kinetic.git
    cd oio-kinetic
    git submodule init
    git submodule update

Build the project using the pkg-config management for the dependencies:

    cmake .
    make
    make install

Alternatively, you could only use shipped third-party dependencies:

    cmake -DSYS=OFF -DGUESS=OFF .
    make
    make install

Possible CMake options:

Name | Value | Description
---- | ----- | -----------
SYS | ON/OFF | ON by default, allows locating dependencies with pkg-config
GUESS | ON/OFF | ON by default, allows locating dependencies at standard places
TESTING | ON/OFF | ON by default, enables testing targets, implying new dependencies
RAGEL_EXE | PATH | Specify the absolute path to the `ragel` CLI tool. If not set, `ragel` will be located in the `PATH` environment variable.
PROTOBUF\_EXE | PATH | Specify the absolute path to the `protoc` CLI tool. If not set, `protoc` will be located in the `PATH`  environment variable.

Name | Value | Description
---- | ----- | -----------
GFLAGS\_SYSTEM | ON/OFF | Use system-wide gflags known by pkg-config
GFLAGS\_GUESS  | ON/OFF | Guess the system-wide place of gflags
GFLAGS\_INCDIR | path | Custom path to gflags headers directory
GFLAGS\_LIBDIR | path | Custom path to gflags libraries directory

Name | Value | Description
---- | ----- | -----------
GTEST\_SYSTEM | ON/OFF | Use system-wide gtest known by pkg-config
GTEST\_GUESS  | ON/OFF | Guess the system-wide place of gtest
GTEST\_INCDIR | path | Custom path to gtest headers directory
GTEST\_LIBDIR | path | Custom path to gtest libraries directory

Name | Value | Description
---- | ----- | -----------
GLOG\_SYSTEM | ON/OFF | Use system-wide glog known by pkg-config
GLOG\_GUESS  | ON/OFF | Guess the system-wide place of glog
GLOG\_INCDIR | path | Custom path to glog headers directory
GLOG\_LIBDIR | path | Custom path to glog libraries directory

Name | Value | Description
---- | ----- | -----------
EC\_SYSTEM | ON/OFF | Use system-wide glog known by pkg-config
EC\_GUESS  | ON/OFF | Guess the system-wide place of glog
EC\_INCDIR | path | Custom path to glog headers directory
EC\_LIBDIR | path | Custom path to glog libraries directory

Name | Value | Description
---- | ----- | -----------
MILL\_SYSTEM | ON/OFF | Use system-wide libmill known by pkg-config
MILL\_GUESS  | ON/OFF | Guess the system-wide place of libmill
MILL\_INCDIR | path | Custom path to libmill headers directory
MILL\_LIBDIR | path | Custom path to libmill libraries directory

Name | Value | Description
---- | ----- | -----------
ATTR\_SYSTEM | ON/OFF | Use system-wide libattr known by pkg-config
ATTR\_GUESS  | ON/OFF | Guess the system-wide place of libattr
ATTR\_INCDIR | path | Custom path to libattr headers directory
ATTR\_LIBDIR | path | Custom path to libattr libraries directory

Name | Value | Description
---- | ----- | -----------
RAPIDJSON\_SYSTEM | ON/OFF | Use system-wide rapidjson known by pkg-config
RAPIDJSON\_GUESS  | ON/OFF | Guess the system-wide place of rapidjson
RAPIDJSON\_INCDIR | path | Custom path to rapidjson headers directory

Name | Value | Description
---- | ----- | -----------
RAPIDJSON\_GUESS  | ON/OFF | Guess the system-wide place of rapidjson
RAPIDJSON\_INCDIR | path | Custom path to rapidjson headers directory


For GTEST, GFLAGS and GLOG, if neither SYSTEM, GUESS nor explicit paths worked, a cmake ExternalProject will be used.
For EC, no external project is involved (liberasurecode is not managed by CMake yet)

For each dependency, 4 options are available: \_SYSTEM, \_GUESS, \_INCDIR and \_LIBDIR.
The precedence order is always the same: \_INCDIR/LIBDIR >> SYSTEM >> GUESS.
If no explicit INCDIR/LIBDIR is set, if SYSTEM is set to OFF, and GUESS also set to OFF, then cmake will use an [ExternalProject](https://cmake.org/cmake/help/v3.0/module/ExternalProject.html) and build it on the moment.

## Modules

### oio::api::blob

### oio::mem::blob

Implemention of a blob store where all blobs are completely held in memory.
Currently for testing purposes, there is neither thread safety at all managed around the shared map of items, nor any capacity limit on the cache.

### oio::local::blob

File based implementation of a blob store, "à la" RAWX.

### oio::http::blob

HTTP-based classes used in **oio**, all written in an imperative style. Two ways to parallelize the communicaitons are offered: either you play with a combination of `std::thread` and `net::RegularSocket`, or you play with libmill's coroutines and `net::MillSocket`.

Though incompatible with reactive frameworks where all the sockets are explicitely organized around a main event loop, the imperative way of `oio::http::blob` offers the advantage of an extreme simplicity.

### oio::rawx::blob

Wrapper around `oio::http::blob` with few additional checks to match the expectations of a RAWX service.

### oio::kinetic::blob

Kinetic based backend, written around libmill's coroutines.

### oio::ec::blob

When uploading, `oio::ec::blob::Upload` computes the Erasure Coded form of the input then
Wrapper implementation around other blob stores.

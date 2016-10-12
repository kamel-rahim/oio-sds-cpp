# OpenIO Kinetic backend

The present repository provides several tools:

A set of libraries:
* **oio-utils**: miscellaneous utility tools, e.g. checksum functions, network management, http parsing, etc.
* **oio-http-parser**: Wraps the [http-parser](https://github.com/nodejs/http-parser)
* **oio-data**: an API for Blob store into OpenIO
* **oio-data-kinetic**: a Kinetic blob store implementation
* **oio-data-http**: an HTTP blob store implementation
* **oio-data-rawx**: a specialization of oio-data-http, with more constraints on the fields

A set of binary CLI tools
* **kinetic-stress-put**: stress a kinetic drive with a massive PUT load and a configurable naming of the keys (random, ascending, decreasing)
* **oio-kinetic-proxy**: a minimal coroutine-based HTTP server wrapping the **oio-kinetic-client**.
* **oio-kinetic-listener**: listens to the multicast UDP announces of kinetic drives and register those in the configured conscience.
* **oio-rawx**: an ersatz of the officiel OpenIO SDS rawx service, used to validate the design. Also coroutine-based.

## Status

This repository is under an active development effort and is subject to frequent changes. 

## License

Copyright 2016 Contributors (see the [AUTHORS](./AUTHORS) file). 

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This program is distributed in the hope that it will be useful, but is
provided AS-IS, WITHOUT ANY WARRANTY; including without the implied
warranty of MERCHANTABILITY, NON-INFRINGEMENT or FITNESS FOR A
PARTICULAR PURPOSE. See the Mozilla Public License for more details.


## Dependencies

* [http-parser](https://github.com/nodejs/http-parser) included as a Git
  submodule
* [the Kinetic protocol](https://github.com/Kinetic/kinetic-protocol)
  included as a Git submodule, a Protocol Buffer definition maintained
  by the Kinetic Open Storage group.
* [libmill](https://github.com/sustrik/libmill): for easy cooperative
  concurrency, thanks tyo Martin Sustrik's coroutines.
* [ragel](https://github.com/colmnet/ragel): Used as a lexer to easily
  and efficiently recognize HTTP headers 
* [glog](https://github.com/google/glog)
* [gtest](https://github.com/google/googletest)
* [gflags](https://github.com/gflags/gflags)


Build & Install
---------------

Get the sources and all the dependencies

    git clone https://github.com/open-io/oio-kinetic.git
    cd oio-kinetic
    git submodule init
    git submodule update

Build the whole

    cmake .
    make
    make install


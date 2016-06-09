# OpenIO Kinetic backend

The present repository provides 2 tools:

* **oio-kinetic-client**: a coroutine-based code library to put, get and
  delete BLOBs spread on kinetic drives.
* **oio-kinetic-proxy**: a minimal coroutine-based HTTP server wrapping
  the **oio-kinetic-client**.  

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

### oio-kinetic-client

* [the Kinetic protocol](https://github.com/Kinetic/kinetic-protocol): Maintained by the Kinetic Open Storage group, as a Protocol Buffer definition.
* [libmill](https://github.com/sustrik/libmill): Used for easy cooperative concurrency, thanks tyo Martin Sustrik's coroutines.
* [glog](https://github.com/google/glog)

### oio-kinetic-proxy

* [http-parser](https://github.com/nodejs/http-parser): Currently shipped and slightly modified for a "c++ friendly" version.
* [ragel](https://github.com/colmnet/ragel): Used as a lexer to easily and efficiently recognize HTTP headers 
* [glog](https://github.com/google/glog)
* [libmill](https://github.com/sustrik/libmill): Used for easy cooperative concurrency, thanks tyo Martin Sustrik's coroutines.

Build & Install
---------------

    cmake .
    make
    make install


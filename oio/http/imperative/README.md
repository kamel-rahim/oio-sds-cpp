oio::http::imperative
=====================

HTTP-based classes used in **oio**, all written in an imperative style.
Two ways to parallelize the communicaitons are offered: either you play
with a combination of `std::thread` and `net::RegularSocket`, or you
play with libmill's coroutines and `net::MillSocket`.

Though incompatible with reactive frameworks where all the sockets are
explicitely organized around a main event loop, the imperative way of
`oio::http::imperative` offers the advantage of an extremme simplicity.

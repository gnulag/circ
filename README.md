circ
===

Meet **circ**, our channel locked Ultimate IRC Bot. After recently deprecating
vini, we decided to up the ante: to forge our own bot from scratch. No forking,
no outside code, but this time we can't leave #gnulag. All leading up to
eventually taking on one of IRC's biggest challenges, agreeing on
something.

## Building

```
$ git clone --recurse-submodules https://github.com/gnulag/circ.git
$ mkdir circ/build
$ cmake ..
$ make
```

### Requirements:

* `cmake`
* `make`
* a C compiler
* clang-format
* libev
* gnutls
* glib
* libsqlite3

#### Debian/Ubuntu
```
$ sudo apt install build-essential cmake libev-dev gnutls-dev glib2.0-dev libsqlite3-dev clang-format-8
```

#### Fedora
```
$ sudo dnf install "@Development Tools" cmake glib2-devel libev-devel gnutls-devel libsqlite3x-devel clang-devel
```

## Running

Put a copy of `config.json` into the binary directory and edit it.

Run `circ`.

## Contributing

For contribution guidelines, and editor config see `CONTRIBUTING.md`.

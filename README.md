# dumbhttpd - a tiny, hacky HTTP server

This repository contains the code of a simple HTTP server written in C as an exercise.
As such, I deem it unsuitable for anything other than educational purposes and I take no responsibility for any damage done as a result of using this software.

## Description

This server implements **GET**, **POST**, **PUT**, **DELETE** and **HEAD** (while not strictly adhering to the specification).
By using these methods, it allows to retrieve, upload and delete documents respectively.
The `Keep-Alive` mode of communication is (not yet) supported.

## Usage

To compile the server, simply run `make` and you'll find the binary under `bin/dumbhttpd`.

In order to start the server, run `./bin/dumbhttpd <port> <root directory>`. 

## Credits

* [http-status-codes-cpp](https://github.com/j-ulrich/http-status-codes-cpp) - the header file containing status codes comes from there.
* [RFC2616](https://tools.ietf.org/html/rfc2616) - a rather lengthy document describing the ins and outs of HTTP/1.1.

# node-libevhtp

Use libevhtp from Node.js (low-level)

Author: Christopher J. Brody

License: MIT or BSD-3

I can hereby testify that this project is completely my own work and not subject to agreements with any other parties.
The exception is `nan_object_wrap_template.h` which is derived from the Node.js Nan project.
In case of code written under direct guidance from sample code the link is given for reference.
In case I accept contributions from any others I will require CLA with similar statements.
I may offer this work under other licensing terms in the future.

NOTE: This project is under development and should be considered experimental.
API is subject to change and some optimizations may be needed.

Status: under development
- SSL and REGEX capabilities are currently disabled

## Pre-requisites:

- check out libevhtp
- unpack and build libevent-2.0.22-stable

XXX TODO: should be automatic!

## Build and test

```shell
npm install
```

## rebuild

```shell
node-gyp rebuild
```

## quick test

```shell
node mytest.js
```

In another window:

```shell
curl http://localhost:8000
```

# http-libuv

Ryan's example ([vimeo](https://vimeo.com/24713213), [youtube](https://www.youtube.com/watch?v=aLm40q7qm3w)) reproduced in 2022. This repo just updates Ryan's example with latest API of [http-parser](https://github.com/nodejs/http-parser) and in [llhttp](https://github.com/nodejs/llhttp). Maybe it is a good example to look at, before read other more complex http server projects based on libuv.

## Build

Simple. just `make all`. (Use llhttp: `USE_LLHTTP=1 make all`)

http-parser/llhttp-6.0.6 and [libuv](https://github.com/libuv/libuv)-1.44.1 libraries are statically linked.

## Run

```sh
./sample-server
```

In another terminal send a simple http get request with curl. (Run multiple times to connect to the server repeatedly)

```sh
curl http://localhost:8000/
```

sample output:

```text
sample server launched
Waiting for connections...

on_connection
on alloc
on read
Read:
GET / HTTP/1.1
Host: localhost:8000
User-Agent: curl/7.71.1
Accept: */*



***MESSAGE BEGIN***

Url: /
Header field: Host
Header value: localhost:8000
Header field: User-Agent
Header value: curl/7.71.1
Header field: Accept
Header value: */*

***HEADERS COMPLETE***


***MESSAGE COMPLETE***

on close
```

Connect to the server and send random string to be displayed at the server side. Once the connection stops, event loop will also exit (`uv_stop` at main.cc:L130).

```sh
nc localhost 8000
```

## Other implementations

There are many other implementations online and over the years some become more complex to support different platform and restructured to follow languages best practices. Here are a few others I found:

* <https://github.com/peters/libuv-http>
* <https://github.com/heapwolf/cxx-http>
* <https://github.com/guenchi/Igropyr>
* <https://github.com/montehunter/uvllhttpd>

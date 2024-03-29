#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "uv.h"

#ifdef USE_LLHTTP
  #include "llhttp.h"
  typedef llhttp_t parser_t;
  typedef llhttp_settings_t parser_settings_t;
  #define PARSER_SETTINGS_INIT(m) llhttp_settings_init(m)
#else // use http_parser
  #include "http_parser.h"
  typedef http_parser parser_t;  
  typedef http_parser_settings parser_settings_t;
  #define PARSER_SETTINGS_INIT(m) http_parser_settings_init(m)
#endif // USE_LLHTTP

#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n" \
  "\r\n" \
  "hello world\n"

namespace {

uv_tcp_t server;
uv_loop_t* loop;

typedef struct {
  uv_tcp_t handle;
  parser_t parser;
  uv_write_t write_req;
  uv_buf_t resbuf;
} client_t;

parser_settings_t settings;

bool check_error(int ret) {
  if (!ret) return false;
  fprintf(stderr, "libuv error: %s\n", uv_strerror(ret));
  return true;
}

void on_close(uv_handle_t* handle) {
  printf("on close\n");
  client_t* client = (client_t*) handle->data;
  free(client);
  client = nullptr;
}

void after_write(uv_write_t* req, int status) {
  check_error(status);
  uv_close((uv_handle_t*) req->handle, on_close);
}

int on_message_begin(parser_t* _) {
  (void)_;
  printf("\n***MESSAGE BEGIN***\n\n");
  return 0;
}

int on_headers_complete(parser_t* _) {
  (void)_;
  printf("\n***HEADERS COMPLETE***\n\n");
  return 0;
}

int on_message_complete(parser_t* parser) {
  printf("\n***MESSAGE COMPLETE***\n\n");
  client_t* client = (client_t*) parser->data;
  uv_write(&client->write_req, (uv_stream_t*) &client->handle,
    &client->resbuf, 1, after_write); // after_write not defined
  return 0;
}

int on_url(parser_t* _, const char* at, size_t length) {
  (void)_;
  printf("Url: %.*s\n", (int)length, at);
  return 0;
}

int on_header_field(parser_t* _, const char* at, size_t length) {
  (void)_;
  printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

int on_header_value(parser_t* _, const char* at, size_t length) {
  (void)_;
  printf("Header value: %.*s\n", (int)length, at);
  return 0;
}

int on_body(parser_t* _, const char* at, size_t length) {
  (void)_;
  printf("Body: %.*s\n", (int)length, at);
  return 0;
}

// all the callbacks implementation are from http-parser/contrib/parsertrace.c
//  to print something demonstrating the processing of each phase.
// on_message_complete is rewritten to send back response.
void setup_http_parser_settings() {
  PARSER_SETTINGS_INIT(&settings);
  settings.on_message_begin = on_message_begin;
  settings.on_url = on_url;
  settings.on_header_field = on_header_field;
  settings.on_header_value = on_header_value;
  settings.on_headers_complete = on_headers_complete;
  settings.on_body = on_body;
  settings.on_message_complete = on_message_complete;
}

void on_alloc(uv_handle_t* /*handle*/, size_t suggested_size, uv_buf_t* buf) {
  printf("on alloc\n");
  *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  printf("on read\n");
  /*do something*/
  if (nread >= 0) {
    printf("Read:\n%.*s\n", (int) nread, buf->base);
    /*parse http*/
    client_t* client = (client_t*) uv_handle_get_data((uv_handle_t*) stream);
    parser_t* parser = &client->parser;

#ifdef USE_LLHTTP
    auto ret = llhttp_execute(parser, buf->base, nread);
    if (ret != HPE_OK) {
      // fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(ret),
      //   parser->reason);
      printf("Not a http request, no response sent\n");
    }
#else // use http_parser
    auto parsed = http_parser_execute(parser, &settings, buf->base, nread);
    if (parsed < static_cast<size_t>(nread)) {
      // fprintf(stderr, "parser error(%s): parsed lesser bytes %ld of %ld\n",
      //   http_errno_name(http_errno(parser->http_errno)), parsed, nread);
      printf("Not a http request, no response sent\n");
    }
#endif // USE_LLHTTP 

  } else {
    // error
    if (nread != UV_EOF) {
      printf("Read error: %ld\n", nread);
    }
    uv_close((uv_handle_t*) stream, on_close);
    uv_stop(loop);
    printf("server schedules to shutdown\n");
  }

  free(buf->base);
}

void on_connection(uv_stream_t* server_handle, int status) {
  assert(server_handle == (uv_stream_t*) &server);
  printf("\non_connection\n");

  if (check_error(status)) return;

  // allocate http parser and a handle for each connection
  //  note that we need to allocate them on heap to use over event looping.
  //  (on stack will not work)
  client_t* client = (client_t*) malloc(sizeof(client_t));

  // init
#ifdef USE_LLHTTP 
  llhttp_init(&client->parser, HTTP_BOTH, &settings);
#else // use http parser 
  http_parser_init(&client->parser, HTTP_REQUEST);
#endif // USE_LLHTTP 

  auto ret = uv_tcp_init(server_handle->loop, &client->handle);
  // let the data pointer of handle to point to the client struct,
  //  so we can access http parser.
  uv_handle_set_data((uv_handle_t*) &client->handle, client);
  // let the data pointer of parser to point to the client struct,
  //  so we can access handle.
  client->parser.data = client;
  client->resbuf = uv_buf_init(RESPONSE, sizeof(RESPONSE));

  check_error(ret);
  ret = uv_accept(server_handle, (uv_stream_t*) &client->handle);
  if (check_error(ret)) {
    uv_close((uv_handle_t*) &client->handle, on_close);
  } else {
    ret = uv_read_start((uv_stream_t*) &client->handle, on_alloc, on_read);
    check_error(ret);
  }
}

} // anonymous namespace

int main() {
  printf("sample server launched\nWaiting for connections...\n");
 
  // setup http parser settings
  setup_http_parser_settings();

  // we only need a single loop, so use the default loop.
  // uv_loop_t* loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));
  // uv_loop_init(loop);
  loop = uv_default_loop();

  // start a server
  uv_tcp_init(loop, &server);

  struct sockaddr_in address;
  uv_ip4_addr("0.0.0.0", 8000, &address);

  int ret = uv_tcp_bind(&server, (const struct sockaddr*) &address, 0);
  check_error(ret);

  ret = uv_listen((uv_stream_t*) &server, 128, on_connection);
  check_error(ret);

  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  // using default loop.
  // free(loop);
  return 0;
}
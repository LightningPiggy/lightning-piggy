// Runs Main/HttpResponse.h, the firmware's HTTP response reader, against scripted servers.
// Build and run with tests/host/run.sh; no device or Arduino toolchain needed.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "Arduino.h"
#include "../../Main/HttpResponse.h"

unsigned long fakeMillis = 0;
size_t String::failConcatOver = 0;

// A client like the device's: non-blocking, so read() returns -1 until the next piece "arrives".
// The response is delivered in pieces of pieceSize bytes, with one empty read before each piece.
class FakeClient {
  public:
    FakeClient(const std::string& data, size_t pieceSize, bool closeAtEnd = true)
      : data_(data), pieceSize_(pieceSize ? pieceSize : data.size() + 1), closeAtEnd_(closeAtEnd) {}
    int read(uint8_t* buf, size_t len) {
      if (pos_ >= data_.size()) return -1;
      if (available_ == 0) { // nothing has arrived yet: the next piece arrives after this call
        available_ = pieceSize_;
        return -1;
      }
      size_t n = len < available_ ? len : available_;
      if (n > data_.size() - pos_) n = data_.size() - pos_;
      for (size_t i = 0; i < n; i++) buf[i] = (uint8_t)data_[pos_ + i];
      pos_ += n;
      available_ -= n;
      return (int)n;
    }
    int read() { uint8_t c; return read(&c, 1) == 1 ? c : -1; }
    bool connected() { return pos_ < data_.size() || !closeAtEnd_; }
  private:
    std::string data_;
    size_t pieceSize_, pos_ = 0, available_ = 0;
    bool closeAtEnd_;
};

// A server that always has more to send: start, then filler repeated for ever. Each read takes a little time.
class EndlessClient {
  public:
    EndlessClient(const std::string& start, const std::string& filler) : start_(start), filler_(filler) {}
    int read(uint8_t* buf, size_t len) {
      for (size_t i = 0; i < len; i++, pos_++)
        buf[i] = (uint8_t)(pos_ < start_.size() ? start_[pos_] : filler_[(pos_ - start_.size()) % filler_.size()]);
      if (++reads_ % 50 == 0) fakeMillis++;
      return (int)len;
    }
    int read() { uint8_t c; read(&c, 1); return c; }
    bool connected() { return true; }
  private:
    std::string start_, filler_;
    size_t pos_ = 0;
    unsigned long reads_ = 0;
};

static int failures = 0, checks = 0;
static const unsigned long TIMEOUT = 15000;

static void check(const char* name, const std::string& response, HttpReadResult wantResult, const std::string& wantBody, bool closeAtEnd = true) {
  // Deliver the response whole, then split at every possible size: the result must not depend on how the bytes arrive.
  for (size_t piece = 0; piece <= response.size(); piece++) {
    fakeMillis = 4294960000UL; // close to rollover, so the deadline arithmetic gets exercised too
    FakeClient client(response, piece, closeAtEnd);
    String body;
    int status = 0;
    HttpReadResult got = readHttpResponse(client, TIMEOUT, body, status);
    checks++;
    if (got != wantResult || body.std() != wantBody) {
      failures++;
      printf("FAIL %s (piece size %zu): got '%s' with %u body bytes, wanted '%s' with %zu\n", name, piece,
             httpReadResultToString(got), body.length(), httpReadResultToString(wantResult), wantBody.size());
      return;
    }
  }
  printf("ok   %s\n", name);
}

int main(int argc, char** argv) {
  if (argc == 2) { // decode a response captured from a real server (see tests/host/capture.py)
    std::ifstream in(argv[1], std::ios::binary);
    std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    FakeClient client(raw, 7);
    String body;
    int status = 0;
    HttpReadResult got = readHttpResponse(client, TIMEOUT, body, status);
    printf("result: %s\nstatus: %d\nbody bytes: %u\n", httpReadResultToString(got), status, body.length());
    fwrite(body.c_str(), 1, body.length(), stdout);
    printf("\n");
    return got == HTTP_READ_OK ? 0 : 1;
  }

  const std::string OK = "HTTP/1.1 200 OK\r\n";
  const std::string CHUNKED = "Transfer-Encoding: chunked\r\n";

  // What m.lightningpiggy.com (Netlify) sends to the update checker, byte for byte apart from the date and request id.
  check("update checker reply from Netlify",
        OK + "Cache-Control: no-store\r\nContent-Type: text/plain; charset=utf-8\r\nDate: Fri, 30 Oct 2026 21:40:00 GMT\r\n"
        "Server: Netlify\r\nStrict-Transport-Security: max-age=31536000\r\nVary: Accept-Encoding\r\n"
        "X-Nf-Request-Id: 01M67C59CZ7VXAG16K266DAGQB\r\nConnection: close\r\n" + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n\r\n",
        HTTP_READ_OK, "6.4.0");

  // Chunked bodies
  check("several chunks", OK + CHUNKED + "\r\n3\r\nabc\r\n1\r\nd\r\n2\r\nef\r\n0\r\n\r\n", HTTP_READ_OK, "abcdef");
  check("chunk bigger than the read buffer", OK + CHUNKED + "\r\n5dc\r\n" + std::string(1500, 'x') + "\r\n0\r\n\r\n", HTTP_READ_OK, std::string(1500, 'x'));
  check("uppercase hex chunk size", OK + CHUNKED + "\r\nA\r\n0123456789\r\n0\r\n\r\n", HTTP_READ_OK, "0123456789");
  check("chunk extension", OK + CHUNKED + "\r\n5;name=value\r\n6.4.0\r\n0;last\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("zero-padded sizes", OK + CHUNKED + "\r\n005\r\n6.4.0\r\n000\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("trailer fields after the last chunk", OK + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\nX-Trailer: 1\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("connection closes right after the last chunk size", OK + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n", HTTP_READ_OK, "6.4.0");
  check("chunk data containing CRLF and a size-like line", OK + CHUNKED + "\r\n7\r\na\r\n0\r\nb\r\n0\r\n\r\n", HTTP_READ_OK, "a\r\n0\r\nb");
  check("chunk data containing a zero byte", OK + CHUNKED + "\r\n3\r\n" + std::string("a\0b", 3) + "\r\n0\r\n\r\n", HTTP_READ_OK, std::string("a\0b", 3));
  check("empty chunked body", OK + CHUNKED + "\r\n0\r\n\r\n", HTTP_READ_OK, "");
  check("bare LF line endings", "HTTP/1.1 200 OK\nContent-Length: 5\n\n6.4.0", HTTP_READ_OK, "6.4.0");

  // Header spelling
  check("header in lowercase", OK + "transfer-encoding: chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("header in odd case without space", OK + "TRANSFER-ENCODING:Chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("header with extra whitespace", OK + "Transfer-Encoding:   chunked  \r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("identity, chunked", OK + "Transfer-Encoding: identity, chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("chunked wins over Content-Length", OK + "Content-Length: 99\r\n" + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("very long header line", OK + "Set-Cookie: " + std::string(3000, 'c') + "\r\n" + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("long header line that mentions chunked late", OK + "X-Note: " + std::string(300, 'c') + " transfer-encoding: chunked\r\nContent-Length: 5\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");
  check("header line without a colon", OK + "garbage\r\nContent-Length: 5\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");

  // Content-Length and close-delimited bodies
  check("Content-Length", OK + "Content-Length: 5\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");
  check("Content-Length, connection kept open", OK + "Content-Length: 5\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0", false);
  check("Content-Length with extra bytes after the body", OK + "Content-Length: 5\r\n\r\n6.4.0EXTRA", HTTP_READ_OK, "6.4.0");
  check("Content-Length of zero", OK + "Content-Length: 0\r\n\r\n", HTTP_READ_OK, "");
  check("Content-Length bigger than the read buffer", OK + "Content-Length: 2000\r\n\r\n" + std::string(2000, 'y'), HTTP_READ_OK, std::string(2000, 'y'));
  check("body until close", OK + "Content-Type: text/plain\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");
  check("long body until close", OK + "\r\n" + std::string(1300, 'z'), HTTP_READ_OK, std::string(1300, 'z'));
  check("empty body until close", OK + "\r\n", HTTP_READ_OK, "");
  check("status 204", "HTTP/1.1 204 No Content\r\n\r\n", HTTP_READ_OK, "");
  check("HTTP/1.0 reply", "HTTP/1.0 200 OK\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");

  // Replies that must be rejected, with nothing returned as data
  check("gzip, chunked", OK + "Transfer-Encoding: gzip, chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_UNSUPPORTED_ENCODING, "");
  check("Content-Encoding gzip", OK + "Content-Encoding: gzip\r\nContent-Length: 5\r\n\r\n6.4.0", HTTP_READ_UNSUPPORTED_ENCODING, "");
  check("Content-Encoding br with chunked", OK + "Content-Encoding: br\r\n" + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_UNSUPPORTED_ENCODING, "");
  check("404 page", "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found", HTTP_READ_BAD_STATUS_CODE, "");
  check("502 page from the hosting platform", "HTTP/1.1 502 Bad Gateway\r\n" + CHUNKED + "\r\n5\r\n<htm>\r\n0\r\n\r\n", HTTP_READ_BAD_STATUS_CODE, "");
  check("301 redirect", "HTTP/1.1 301 Moved Permanently\r\nLocation: https://example.com/\r\nContent-Length: 0\r\n\r\n", HTTP_READ_BAD_STATUS_CODE, "");
  check("not HTTP at all", "<html>captive portal</html>\r\n\r\n", HTTP_READ_BAD_STATUS_LINE, "");
  check("status line without a code", "HTTP/1.1\r\n\r\n", HTTP_READ_BAD_STATUS_LINE, "");
  check("nothing received", "", HTTP_READ_NO_STATUS_LINE, "");
  check("closed in the middle of the headers", OK + "Content-Type: text/pl", HTTP_READ_INCOMPLETE_HEADERS, "");
  check("closed before the blank line", OK + CHUNKED, HTTP_READ_INCOMPLETE_HEADERS, "");
  check("chunk cut short", OK + CHUNKED + "\r\n5\r\n6.4", HTTP_READ_INCOMPLETE_BODY, "");
  check("closed before the last chunk", OK + CHUNKED + "\r\n5\r\n6.4.0\r\n", HTTP_READ_INCOMPLETE_BODY, "");
  check("closed in the middle of a chunk size", OK + CHUNKED + "\r\n5\r\n6.4.0\r\n1", HTTP_READ_INCOMPLETE_BODY, "");
  check("Content-Length body cut short", OK + "Content-Length: 10\r\n\r\n6.4.0", HTTP_READ_INCOMPLETE_BODY, "");
  check("chunk size that is not hex", OK + CHUNKED + "\r\nzz\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("empty chunk size line", OK + CHUNKED + "\r\n\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("negative chunk size", OK + CHUNKED + "\r\n-5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("absurdly long chunk size", OK + CHUNKED + "\r\n100000000\r\nx\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("chunk longer than its size says", OK + CHUNKED + "\r\n3\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("chunk size one too big", OK + CHUNKED + "\r\n6\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("chunk size one too small", OK + CHUNKED + "\r\n4\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("chunk data followed by a bare LF", OK + CHUNKED + "\r\n5\r\n6.4.0\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("chunk size line that is too long", OK + CHUNKED + "\r\n5;" + std::string(300, 'e') + "\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_CHUNK, "");
  check("empty Transfer-Encoding", OK + "Transfer-Encoding:\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_UNSUPPORTED_ENCODING, "");
  check("empty Content-Encoding", OK + "Content-Encoding: \r\nContent-Length: 5\r\n\r\n6.4.0", HTTP_READ_UNSUPPORTED_ENCODING, "");
  check("folded Transfer-Encoding", OK + "Transfer-Encoding: identity,\r\n chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_HEADER, "");
  check("folded line hiding a Content-Length", OK + "X-Foo: a\r\n\tcontent-length: 3\r\n\r\n6.4.0", HTTP_READ_BAD_HEADER, "");
  check("Transfer-Encoding pushed past the kept part of the line", OK + "Transfer-Encoding:" + std::string(300, ' ') + "chunked\r\n\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_BAD_HEADER, "");
  check("Content-Length with a space before the colon", OK + "Content-Length : 3\r\n\r\n6.4.0", HTTP_READ_BAD_HEADER, "");
  check("two different Content-Lengths", OK + "Content-Length: 5\r\nContent-Length: 3\r\n\r\n6.4.0", HTTP_READ_BAD_CONTENT_LENGTH, "");
  check("the same Content-Length twice", OK + "Content-Length: 5\r\nContent-Length: 5\r\n\r\n6.4.0", HTTP_READ_OK, "6.4.0");
  check("Transfer-Encoding chunked twice", OK + CHUNKED + CHUNKED + "\r\n5\r\n6.4.0\r\n0\r\n\r\n", HTTP_READ_OK, "6.4.0");
  check("Content-Length that is not a number", OK + "Content-Length: five\r\n\r\n6.4.0", HTTP_READ_BAD_CONTENT_LENGTH, "");
  check("negative Content-Length", OK + "Content-Length: -1\r\n\r\n6.4.0", HTTP_READ_BAD_CONTENT_LENGTH, "");

  // A server that stops sending but keeps the connection open: give up at the deadline, return nothing.
  check("stalled in the headers", OK + "Content-Type: text/pl", HTTP_READ_INCOMPLETE_HEADERS, "", false);
  check("stalled in a chunk", OK + CHUNKED + "\r\n5\r\n6.4", HTTP_READ_INCOMPLETE_BODY, "", false);
  check("stalled before the last chunk", OK + CHUNKED + "\r\n5\r\n6.4.0\r\n", HTTP_READ_INCOMPLETE_BODY, "", false);
  check("stalled with no length given", OK + "\r\n6.4.0", HTTP_READ_INCOMPLETE_BODY, "", false);
  check("stalled before any reply", "", HTTP_READ_NO_STATUS_LINE, "", false);

  // Out of memory while collecting the body
  String::failConcatOver = 1000;
  check("body too big, chunked", OK + CHUNKED + "\r\n5dc\r\n" + std::string(1500, 'x') + "\r\n0\r\n\r\n", HTTP_READ_OUT_OF_MEMORY, "");
  check("body too big, Content-Length", OK + "Content-Length: 2000\r\n\r\n" + std::string(2000, 'y'), HTTP_READ_OUT_OF_MEMORY, "");
  check("body too big, until close", OK + "\r\n" + std::string(1300, 'z'), HTTP_READ_OUT_OF_MEMORY, "");
  String::failConcatOver = 0;

  // The deadline is honoured: a stalled server costs the timeout, not more.
  fakeMillis = 1000;
  FakeClient stalled(OK + CHUNKED + "\r\n5\r\n6.4", 0, false);
  String body;
  int status;
  readHttpResponse(stalled, TIMEOUT, body, status);
  checks++;
  if (fakeMillis - 1000 < TIMEOUT || fakeMillis - 1000 > TIMEOUT + 10) { failures++; printf("FAIL deadline: waited %lu ms\n", fakeMillis - 1000); }
  else printf("ok   stalled server costs exactly the timeout\n");

  // A server that never stops sending is cut off at the deadline too, whichever part of the reply it floods.
  const char* floods[] = { "", "HTTP/1.1 200 OK\r\n", "HTTP/1.1 200 OK\r\nX-Pad: ", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" };
  for (const char* start : floods) {
    for (const char* filler : { "a", "x: y\r\n", "1\r\na\r\n" }) {
      fakeMillis = 1000;
      EndlessClient endless(start, filler);
      String floodBody;
      HttpReadResult got = readHttpResponse(endless, TIMEOUT, floodBody, status);
      checks++;
      if (got == HTTP_READ_OK || floodBody.length() != 0 || fakeMillis - 1000 > TIMEOUT + 10) {
        failures++;
        printf("FAIL endless '%s' after '%s': '%s' after %lu ms\n", filler, start, httpReadResultToString(got), fakeMillis - 1000);
      }
    }
  }
  printf("ok   endless replies are cut off at the deadline\n");

  printf("\n%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}

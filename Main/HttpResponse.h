#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

// Reader for the HTTP/1.1 response to a "Connection: close" GET request.
//
// It lives in a header, as a template over the client type, so the same code
// that runs on the device (WiFiClientSecure) can be run against a scripted
// client on a computer: see tests/host/.
//
// The client only needs read(), read(uint8_t*, size_t) and connected().
// Its socket may be non-blocking (read() returns -1 when no data has arrived
// yet), so every read here waits until the deadline instead of assuming the
// whole response arrived in one piece.

#include <Arduino.h>

#define HTTP_MAX_HEADER_LINE_LENGTH 256 // longer header lines are consumed, but only this much is kept
#define HTTP_READ_BUFFER_SIZE 512

enum HttpReadResult {
  HTTP_READ_OK = 0,
  HTTP_READ_NO_STATUS_LINE,        // nothing (or no complete line) came back
  HTTP_READ_BAD_STATUS_LINE,       // first line is not "HTTP/x.y NNN ..."
  HTTP_READ_BAD_STATUS_CODE,       // status code is not 2xx
  HTTP_READ_INCOMPLETE_HEADERS,    // connection closed or timed out before the blank line
  HTTP_READ_BAD_HEADER,            // a header that decides how the body is read cannot be trusted
  HTTP_READ_UNSUPPORTED_ENCODING,  // compressed or otherwise encoded body
  HTTP_READ_BAD_CONTENT_LENGTH,    // Content-Length is not a plain number
  HTTP_READ_BAD_CHUNK,             // malformed chunked framing
  HTTP_READ_INCOMPLETE_BODY,       // connection closed or timed out before the announced end of the body
  HTTP_READ_OUT_OF_MEMORY          // body did not fit in memory
};

inline const char* httpReadResultToString(HttpReadResult result) {
  switch (result) {
    case HTTP_READ_OK: return "ok";
    case HTTP_READ_NO_STATUS_LINE: return "no status line received";
    case HTTP_READ_BAD_STATUS_LINE: return "malformed status line";
    case HTTP_READ_BAD_STATUS_CODE: return "status code is not 2xx";
    case HTTP_READ_INCOMPLETE_HEADERS: return "headers incomplete";
    case HTTP_READ_BAD_HEADER: return "malformed header";
    case HTTP_READ_UNSUPPORTED_ENCODING: return "unsupported transfer or content encoding";
    case HTTP_READ_BAD_CONTENT_LENGTH: return "malformed Content-Length";
    case HTTP_READ_BAD_CHUNK: return "malformed chunked body";
    case HTTP_READ_INCOMPLETE_BODY: return "body incomplete";
    case HTTP_READ_OUT_OF_MEMORY: return "body too big for memory";
  }
  return "unknown";
}

inline bool httpDeadlinePassed(unsigned long startMillis, unsigned long timeoutMillis) {
  return (unsigned long)(millis() - startMillis) >= timeoutMillis; // unsigned subtraction survives millis() rollover
}

// Read up to maxLength bytes, waiting for data until the deadline.
// Returns the number of bytes read; fewer than maxLength means the connection closed or the deadline passed.
template <typename ClientT>
size_t httpReadBytes(ClientT& client, uint8_t* buffer, size_t maxLength, unsigned long startMillis, unsigned long timeoutMillis) {
  size_t total = 0;
  // The deadline is checked on every pass, so a server that never stops sending cannot keep the device here.
  while (total < maxLength && !httpDeadlinePassed(startMillis, timeoutMillis)) {
    int readNow = client.read(buffer + total, maxLength - total);
    if (readNow > 0) {
      total += readNow;
      continue;
    }
    if (!client.connected()) {
      // The peer closed. Data that arrived just before the close may still be readable.
      readNow = client.read(buffer + total, maxLength - total);
      if (readNow > 0) {
        total += readNow;
        continue;
      }
      break;
    }
    delay(1);
  }
  return total;
}

// Read one line, up to and including "\n". The line is returned without its "\r\n" (or "\n").
// Returns false if the connection closed or the deadline passed before the "\n".
// truncated is set if the line was longer than HTTP_MAX_HEADER_LINE_LENGTH, so only its start was kept.
template <typename ClientT>
bool httpReadLine(ClientT& client, String& line, bool& truncated, unsigned long startMillis, unsigned long timeoutMillis) {
  line = "";
  truncated = false;
  while (true) {
    uint8_t c;
    if (httpReadBytes(client, &c, 1, startMillis, timeoutMillis) != 1) return false;
    if (c == '\n') break;
    if (line.length() < HTTP_MAX_HEADER_LINE_LENGTH) line += (char)c;
    else truncated = true;
  }
  if (!truncated && line.endsWith("\r")) line.remove(line.length() - 1);
  return true;
}

// True if the comma-separated list (already lowercase) names at least one coding and nothing but allowedA and allowedB.
inline bool httpListContainsOnly(const String& list, const char* allowedA, const char* allowedB, bool& sawA) {
  sawA = false;
  bool sawAny = false;
  int start = 0;
  while (start <= (int)list.length()) {
    int comma = list.indexOf(',', start);
    if (comma < 0) comma = list.length();
    String token = list.substring(start, comma);
    token.trim();
    if (token.length() > 0) sawAny = true;
    if (token == allowedA) sawA = true;
    else if (token.length() > 0 && token != allowedB) return false;
    start = comma + 1;
  }
  return sawAny; // an empty value says nothing about the body, so it is not trusted
}

// Append exactly length bytes of body. Returns HTTP_READ_OK, or why it could not.
template <typename ClientT>
HttpReadResult httpAppendBody(ClientT& client, String& body, unsigned long length, unsigned long startMillis, unsigned long timeoutMillis) {
  uint8_t buffer[HTTP_READ_BUFFER_SIZE];
  unsigned long remaining = length;
  while (remaining > 0) {
    size_t want = remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
    size_t got = httpReadBytes(client, buffer, want, startMillis, timeoutMillis);
    if (got > 0 && !body.concat((const char*)buffer, got)) return HTTP_READ_OUT_OF_MEMORY;
    if (got < want) return HTTP_READ_INCOMPLETE_BODY;
    remaining -= got;
  }
  return HTTP_READ_OK;
}

// Parse the size line of a chunk: hex digits, optionally followed by ";extension".
inline bool httpParseChunkSize(const String& sizeLine, unsigned long& size) {
  String digits = sizeLine;
  int semicolon = digits.indexOf(';');
  if (semicolon >= 0) digits = digits.substring(0, semicolon);
  digits.trim();
  if (digits.length() == 0 || digits.length() > 8) return false;
  size = 0;
  for (unsigned int i = 0; i < digits.length(); i++) {
    char c = digits.charAt(i);
    int value;
    if (c >= '0' && c <= '9') value = c - '0';
    else if (c >= 'a' && c <= 'f') value = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') value = c - 'A' + 10;
    else return false;
    size = (size << 4) | value;
  }
  return true;
}

// Read a whole HTTP response from a client on which the request was just sent.
// On HTTP_READ_OK, body holds the decoded body and statusCode the 2xx status.
// On anything else, body is empty: a body known to be partial or undecodable is never returned as if it were data.
template <typename ClientT>
HttpReadResult readHttpResponse(ClientT& client, unsigned long timeoutMillis, String& body, int& statusCode) {
  unsigned long startMillis = millis();
  body = "";
  statusCode = 0;
  String line;
  bool truncated;

  // Status line, e.g. "HTTP/1.1 200 OK"
  if (!httpReadLine(client, line, truncated, startMillis, timeoutMillis)) return HTTP_READ_NO_STATUS_LINE;
  int firstSpace = line.indexOf(' ');
  if (!line.startsWith("HTTP/") || firstSpace < 0) return HTTP_READ_BAD_STATUS_LINE;
  statusCode = line.substring(firstSpace + 1).toInt();
  if (statusCode < 200 || statusCode > 299) return HTTP_READ_BAD_STATUS_CODE;

  // Headers, until the blank line
  bool chunked = false;
  bool hasContentLength = false;
  unsigned long contentLength = 0;
  while (true) {
    if (!httpReadLine(client, line, truncated, startMillis, timeoutMillis)) return HTTP_READ_INCOMPLETE_HEADERS;
    if (line.length() == 0) break;
    // A line starting with whitespace continues the previous header (obsolete line folding). It could be
    // continuing one of the headers that decide how the body is read, so it is refused rather than skipped.
    if (line.charAt(0) == ' ' || line.charAt(0) == '\t') return HTTP_READ_BAD_HEADER;
    int colon = line.indexOf(':');
    if (colon <= 0) continue;
    String name = line.substring(0, colon);
    String value = line.substring(colon + 1);
    name.toLowerCase();
    value.trim();
    value.toLowerCase();
    String bareName = name;
    bareName.trim();
    bool decidesBody = bareName == "transfer-encoding" || bareName == "content-encoding" || bareName == "content-length";
    // These three must be read whole and spelled exactly ("Content-Length :" is not allowed), or the body cannot be trusted.
    if (decidesBody && (truncated || bareName != name)) return HTTP_READ_BAD_HEADER;
    if (name == "transfer-encoding") {
      bool sawChunked;
      if (!httpListContainsOnly(value, "chunked", "identity", sawChunked)) return HTTP_READ_UNSUPPORTED_ENCODING;
      if (sawChunked) chunked = true;
    } else if (name == "content-encoding") {
      bool unused;
      if (!httpListContainsOnly(value, "identity", "identity", unused)) return HTTP_READ_UNSUPPORTED_ENCODING;
    } else if (name == "content-length") {
      if (value.length() == 0 || value.length() > 9) return HTTP_READ_BAD_CONTENT_LENGTH;
      unsigned long length = 0;
      for (unsigned int i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        if (c < '0' || c > '9') return HTTP_READ_BAD_CONTENT_LENGTH;
        length = length * 10 + (c - '0');
      }
      if (hasContentLength && length != contentLength) return HTTP_READ_BAD_CONTENT_LENGTH; // two different lengths
      contentLength = length;
      hasContentLength = true;
    }
  }

  HttpReadResult result = HTTP_READ_OK;
  if (chunked) {
    // Chunked takes precedence over Content-Length (RFC 9112 section 6.3).
    // Each chunk is: size in hex, CRLF, that many bytes, CRLF. A size of 0 ends the body.
    while (result == HTTP_READ_OK) {
      unsigned long chunkSize;
      if (!httpReadLine(client, line, truncated, startMillis, timeoutMillis)) { result = HTTP_READ_INCOMPLETE_BODY; break; }
      if (truncated || !httpParseChunkSize(line, chunkSize)) { result = HTTP_READ_BAD_CHUNK; break; }
      if (chunkSize == 0) break; // any trailer fields after this are not needed
      result = httpAppendBody(client, body, chunkSize, startMillis, timeoutMillis);
      if (result != HTTP_READ_OK) break;
      uint8_t crlf[2];
      if (httpReadBytes(client, crlf, 2, startMillis, timeoutMillis) != 2) { result = HTTP_READ_INCOMPLETE_BODY; break; }
      if (crlf[0] != '\r' || crlf[1] != '\n') { result = HTTP_READ_BAD_CHUNK; break; } // a wrong chunk size shows up here
    }
  } else if (hasContentLength) {
    result = httpAppendBody(client, body, contentLength, startMillis, timeoutMillis);
  } else {
    // No length given: the body runs until the server closes the connection. This is the one case where a
    // connection that broke part-way cannot be told from a complete body, as the client reports both as closed.
    uint8_t buffer[HTTP_READ_BUFFER_SIZE];
    while (true) {
      size_t got = httpReadBytes(client, buffer, sizeof(buffer), startMillis, timeoutMillis);
      if (got > 0 && !body.concat((const char*)buffer, got)) { result = HTTP_READ_OUT_OF_MEMORY; break; }
      if (got < sizeof(buffer)) {
        if (client.connected()) result = HTTP_READ_INCOMPLETE_BODY; // deadline passed while the server was still sending
        break;
      }
    }
  }

  if (result != HTTP_READ_OK) body = "";
  return result;
}

#endif

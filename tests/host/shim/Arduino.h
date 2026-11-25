// Minimal stand-in for the Arduino core, just enough to run Main/HttpResponse.h on a computer.
#ifndef ARDUINO_SHIM_H
#define ARDUINO_SHIM_H

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

extern unsigned long fakeMillis;
inline unsigned long millis() { return fakeMillis; }
inline void delay(unsigned long ms) { fakeMillis += ms; }

class String {
  public:
    String() {}
    String(const char* s) : s_(s) {}
    String(const std::string& s) : s_(s) {}
    unsigned int length() const { return s_.size(); }
    const char* c_str() const { return s_.c_str(); }
    const std::string& std() const { return s_; }
    String& operator=(const char* s) { s_ = s; return *this; }
    String& operator+=(char c) { s_ += c; return *this; }
    bool operator==(const char* o) const { return s_ == o; }
    bool operator!=(const char* o) const { return s_ != o; }
    bool operator==(const String& o) const { return s_ == o.s_; }
    bool operator!=(const String& o) const { return s_ != o.s_; }
    bool concat(const char* data, unsigned int len) {
      if (failConcatOver && s_.size() + len > failConcatOver) return false; // simulate running out of memory
      s_.append(data, len);
      return true;
    }
    char charAt(unsigned int i) const { return i < s_.size() ? s_[i] : 0; }
    int indexOf(char c, unsigned int from = 0) const {
      size_t p = s_.find(c, from);
      return p == std::string::npos ? -1 : (int)p;
    }
    String substring(unsigned int from, unsigned int to) const {
      if (from > s_.size()) return String();
      if (to > s_.size()) to = s_.size();
      if (to < from) return String();
      return String(s_.substr(from, to - from));
    }
    String substring(unsigned int from) const { return substring(from, s_.size()); }
    bool startsWith(const char* p) const { return s_.rfind(p, 0) == 0; }
    bool endsWith(const char* p) const {
      std::string suffix(p);
      return s_.size() >= suffix.size() && s_.compare(s_.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    void remove(unsigned int index) { if (index < s_.size()) s_.erase(index); }
    void toLowerCase() { for (auto& c : s_) c = (char)tolower((unsigned char)c); }
    void trim() {
      size_t a = 0, b = s_.size();
      while (a < b && isspace((unsigned char)s_[a])) a++;
      while (b > a && isspace((unsigned char)s_[b - 1])) b--;
      s_ = s_.substr(a, b - a);
    }
    long toInt() const { return atol(s_.c_str()); }
    static size_t failConcatOver;
  private:
    std::string s_;
};

#endif

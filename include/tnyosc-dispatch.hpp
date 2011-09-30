// Copyright (c) 2011 Toshiro Yamada
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @file tnyosc-dispatch.hpp
/// @brief tnyosc dispatch header file
/// @author Toshiro Yamada
#ifndef __TNY_OSC_DISPATCH__
#define __TNY_OSC_DISPATCH__

#include <string>
#include <vector>
#include <list>
#include <tr1/memory>

#include <time.h>

namespace tnyosc {

struct Argument {
  char type;    // OSC type tag
  size_t size;  // size in byte
  union {
    int32_t i;  // int32
    float f;    // float32
    char* s;    // OSC-string 
    void* b;    // OSC-blob
    int64_t h;  // int64
    double d;   // float64
    uint64_t t; // OSC-timetag
    char* S;    // Alternate OSC-string, such as "symbols"
    char c;     // ASCII character
    uint32_t r; // 32-bit RGBA color
    struct {
      uint8_t port;
      uint8_t status;
      uint8_t data1;
      uint8_t data2;
    } m;        // MIDI data
  } data;

  Argument();
  Argument(const Argument& a) ;
  virtual ~Argument();
  Argument& operator=(const Argument& a);
};

typedef void (*osc_method)(const std::string& address, 
    const std::vector<Argument>& argv, void* user_data);

// structure to hold method handles
struct MethodTemplate {
  std::string address; // OSC-Address
  std::string types; // OSC-types as a string
  void* user_data; // user data
  osc_method method; // OSC-Methods to call
};

struct ParsedMessage {
  struct timeval timetag; 
  std::string address;
  std::string types;
  std::vector<Argument> argv;
};

// structure to hold callback function for a given OSC packet
struct Callback {
  struct timeval timetag; // OSC-timetag to determine when to call the method
  std::string address;
  std::vector<Argument> argv;
  void* user_data; // user data
  osc_method method; // matched method to call
};

typedef std::tr1::shared_ptr<Callback> CallbackRef;
// use to sort list<Callback> according to their timetag

class Dispatcher {
 public:
  Dispatcher();
  ~Dispatcher();

  /// Add a method template that may respond to an incoming Open Sound Control 
  /// message. Use match_methods to deserialize a raw OSC message and match 
  /// with the added methods.
  void add_method(const char* address, const char* types, 
      osc_method method, void* user_data);

  /// Deserializes a raw Open Sound Control message (as coming from a network)
  /// and returns a list of CallbackRef that matches with the registered method
  /// tempaltes.
  std::list<CallbackRef> match_methods(const char* data, size_t size);

  /// decode_data is called inside match_methods to extract the OSC data from
  /// a raw data.
  static bool decode_data(const char* data, size_t size, 
      std::list<ParsedMessage>& messages, struct timeval timetag=kZeroTimetag);

 private:
  static const struct timeval kZeroTimetag;
  static bool decode_osc(const char* data, size_t size, 
      std::list<ParsedMessage>& messages, struct timeval timetag);
  static bool pattern_match(const std::string& lhs, const std::string& rhs);

  std::list<MethodTemplate> methods_;
};

} // namespace tnyosc

#endif // __TNY_OSC_DISPATCH__


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

/// @file tnyosc.hpp
/// @brief tnyosc main (and only) header file
/// @author Toshiro Yamada
///
/// tnyosc is a header-only Open Sound Control library written in C++ for
/// creating OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and
/// 1.1 types and other nonstandard types, and bundles. Note that tnyosc does not
/// include code to actually send or receive OSC messages.
#ifndef __TNY_OSC__
#define __TNY_OSC__

#if defined(_WIN32)
#include <time.h>
#include <Windows.h>
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
#else
#include <sys/time.h> // gettimeofday
#include <arpa/inet.h> // htonl
#endif
#include <cstddef> // size_t
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#ifdef TNYOSC_WITH_BOOST
#include <boost/shared_ptr.hpp>
#endif

#if defined(_WIN32)
  #if (_MSC_VER < 1300)
    typedef signed char       int8_t;
    typedef signed short      int16_t;
    typedef signed int        int32_t;
    typedef unsigned char     uint8_t;
    typedef unsigned short    uint16_t;
    typedef unsigned int      uint32_t;
  #else
    typedef signed __int8     int8_t;
    typedef signed __int16    int16_t;
    typedef signed __int32    int32_t;
    typedef unsigned __int8   uint8_t;
    typedef unsigned __int16  uint16_t;
    typedef unsigned __int32  uint32_t;
  #endif
  typedef signed __int64    int64_t;
  typedef unsigned __int64  uint64_t;
#else
  #include <inttypes.h> // *int#_t
#endif

// Following ntohll() and htonll() code snipits were taken from
// http://www.codeproject.com/KB/cpp/endianness.aspx?msg=1661457
#ifndef ntohll
/// Convert 64-bit little-endian integer to a big-endian network format
#define ntohll(x) (((int64_t)(ntohl((int32_t)((x << 32) >> 32))) << 32) | \
  (uint32_t)ntohl(((int32_t)(x >> 32))))
#endif
#ifndef htonll
/// Convert 64-bit big-endian network format to a little-endian integer
#define htonll(x) ntohll(x)
#endif

namespace tnyosc {

/// Convert 32-bit float to a big-endian network format
inline int32_t htonf(float x) { return (int32_t)htonl(*(int32_t*)&x); }
/// Convert 64-bit float (double) to a big-endian network format
inline int64_t htond(double x) { return (int64_t)htonll(*(int64_t*)&x); }
/// Convert 32-bit big-endian network format to float
inline double ntohf(int32_t x) { x = ntohl(x); return *(float*)&x; }
/// Convert 64-bit big-endian network format to double
inline double ntohd(int64_t x) { return (double)ntohll(x); }

/// A byte array type internally used in the tnyosc library.
typedef std::vector<char> ByteArray;

/// Returns a pointer to the buffer for the array.
///
/// @param[in] array An array as type ByteArray.
/// @return A pointer to the buffer as char*.
inline const char* get_pointer(const ByteArray& array)
{
  if (array.size() > 0) {
    return &array[0];
  } else {
    return NULL;
  }
}

#ifdef _WIN32
// Windows doesn't have gettimeofday, so here's an equivalent version.
// http://stackoverflow.com/questions/2494356/how-to-use-gettimeofday-or-something-equivalent-with-visual-studio-c-2008
struct timeval {
  time_t tv_sec;  /* seconds since Jan. 1, 1970 */
  int32_t tv_usec; /* and microseconds */
};

struct timezone {
  int32_t tz_minuteswest; /* of Greenwich */
  int32_t tz_dsttime;     /* type of dst correction to apply */
};

/// A windows equivalent function for Unix gettimeofday function.
/// @
inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  int64_t tmpres = 0;
  TIME_ZONE_INFORMATION tz_winapi;
  int rez = 0;

  if (!tv && !tz) {
    return -1;
  }

  ZeroMemory(&ft, sizeof(ft));
  ZeroMemory(&tz_winapi, sizeof(tz_winapi));

  GetSystemTimeAsFileTime(&ft);
  tmpres = ft.dwHighDateTime;
  tmpres <<= 32;
  tmpres |= ft.dwLowDateTime;
  // converting file time to unix epoch
  tmpres /= 10;  // convert into microseconds
  tmpres -= DELTA_EPOCH_IN_MICROSECS;

  if (tv) {
    tv->tv_sec = (time_t)(tmpres * 0.000001);
    tv->tv_usec =(tmpres % 1000000);
  }

  // _tzset(), don't work properly, so we use GetTimeZoneInformation
  if (tz) {
    rez = GetTimeZoneInformation(&tz_winapi);
    tz->tz_dsttime = rez;
    tz->tz_minuteswest = tz_winapi.Bias +
      (rez == 2 ? tz_winapi.DaylightBias : 0);
  }

  return 0;
}
#endif

/// Get the current NTP timestamp by calling gettimeofday.
///
/// @ref [http://stackoverflow.com/questions/2641954/create-ntp-time-stamp-from-gettimeofday]
inline uint64_t get_current_ntp_time()
{
  // time between 1-1-1900 and 1-1-1950
  static const uint64_t epoch = 2208988800UL;
  // max value of NTP fractional part
  static const uint64_t ntp_scale = 4294967295UL;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t tv_ntp = tv.tv_sec + epoch;
  // convert tv_usec to a fraction of a second
  uint64_t tv_usecs = (tv.tv_usec * ntp_scale) / 1000000UL;
  return ((tv_ntp << 32) | tv_usecs);
}


/// This class represents an Open Sound Control message. It supports Open Sound
/// Control 1.0 and 1.1 specifications and extra non-standard arguments listed
/// in http://opensoundcontrol.org/spec-1_0.
class Message {
 public:
#ifdef TNYOSC_WITH_BOOST
  typedef boost::shared_ptr<Message> Ptr;
#endif

  /// Create an OSC message. If address is not given, default OSC address is set
  /// to "/tnyosc".
  explicit Message(const std::string& address="/tnyosc")
    : address_(address), types_(1), is_cached_(false) { types_[0] = ','; }

  /// Create an OSC message. This function is called if Message is created with
  /// a C string.
  explicit Message(const char* address)
    : address_(address), types_(1), is_cached_(false) { types_[0] = ','; }

  ~Message() {}

  // @{
  /// @name Functions for adding OSC  1.0 types
  // int32
  void append(int32_t v) {
    is_cached_ = false;
    types_.push_back('i');
    int32_t a = htonl(v);
    ByteArray b(4);
    memcpy(&b[0], (char*)&a, 4);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // float32
  void append(float v) {
    is_cached_ = false;
    types_.push_back('f');
    int32_t a = htonf(v);
    ByteArray b(4);
    memcpy(&b[0], (char*)&a, 4);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // OSC-string
  void append(const std::string& v) {
    is_cached_ = false;
    types_.push_back('s');
    data_.insert(data_.end(), v.begin(), v.end());
    data_.resize(data_.size() + 4 - (v.size() % 4)); }
  // TODO: use wstring for Windows
  //void append(const std::wstring& v) { }
  void append_cstring(const char* v, size_t len) {
    if (!v || len == 0) return;
    is_cached_ = false;
    types_.push_back('s');
    ByteArray b(v, v+len);
    b.resize(len + (4 - len % 4));
    data_.insert(data_.end(), b.begin(), b.end()); }
  // OSC-blob
  void append_blob(void* blob, uint32_t size) {
    is_cached_ = false;
    types_.push_back('b');
    int32_t a = htonl(size);
    char zeros = (size % 4) != 0 ? 4 - (size % 4) : 0;
    ByteArray b(4 + size + zeros, 0);
    std::copy((uint8_t*)&a, (uint8_t*)&a + 4, b.begin());
    std::copy((uint8_t*)blob, (uint8_t*)blob + size, b.begin() + 4);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // @}

  // @{
  /// @name Functions for adding OSC 1.1 types
  // OSC-timetag (NTP format)
  void append_time(uint64_t v) {
    is_cached_ = false;
    types_.push_back('t');
    uint64_t sec = htonl((uint32_t)(v >> 32));
    uint64_t frac = htonl((uint32_t)v);
    uint64_t a = sec << 32 | frac;
    ByteArray b(8);
    memcpy(&b[0], (char*)&a, 8);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // appends the current UTP timestamp
  void append_current_time() { append_time(get_current_ntp_time()); }
  // True
  void append_true() { is_cached_ = false; types_.push_back('T'); }
  // False
  void append_false() { is_cached_ = false; types_.push_back('F'); }
  // Null (or nil)
  void append_null() { is_cached_ = false; types_.push_back('N'); }
  // Impulse (or Infinitum)
  void append_impulse() { is_cached_ = false; types_.push_back('I'); }
  // @}

  // @{
  /// @name Functions for adding  nonstandard types
  // int64
  void append(int64_t v) {
    is_cached_ = false;
    types_.push_back('h');
    int64_t a = htonll(v);
    ByteArray b(8);
    memcpy(&b[0], (char*)&a, 8);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // float64 (or double)
  void append(double v) {
    is_cached_ = false;
    types_.push_back('d');
    int64_t a = htond(v);
    ByteArray b(8);
    memcpy(&b[0], (char*)&a, 8);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // ascii character
  void append(char v) {
    is_cached_ = false;
    types_.push_back('c');
    int32_t a = htonl(v);
    ByteArray b(4);
    memcpy(&b[0], (char*)&a, 4);
    data_.insert(data_.end(), b.begin(), b.end()); }
  // midi
  void append_midi(uint8_t port, uint8_t status, uint8_t data1, uint8_t data2) {
    is_cached_ = false;
    types_.push_back('m');
    ByteArray b(4);
    b[0] = port;
    b[1] = status;
    b[2] = data1;
    b[3] = data2;
    data_.insert(data_.end(), b.begin(), b.end()); }
  // array
  void append_array(void* array, size_t size) {
    if (!array || size == 0) return;
    is_cached_ = false;
    types_.push_back('[');
    types_.insert(types_.end(), (uint8_t*)&array, (uint8_t*)&array + size);
    types_.push_back(']'); }
  // @}

  /// Sets the OSC address of this message.
  /// @param[in] address The new OSC address.
  void set_address(const std::string& address) {
    is_cached_ = false;
    address_ = address; }
  /// @copydoc set_address(const std::string&)
  void set_address(const char* address) { set_address(std::string(address)); }

  /// Returns the OSC address of this message.
  const std::string& address() const { return address_; }

  /// Returns a complete byte array of this OSC message as a ByteArray type.
  /// The byte array is constructed lazily and is cached until the cache is
  /// obsolete. Call to |data| and |size| perform the same caching.
  ///
  /// @return The OSC message as a ByteArray.
  /// @see data
  /// @see size
  const ByteArray& byte_array() const {
    if (is_cached_) return cache_;
    else return create_cache(); }

  /// Returns a complete byte array of this OSC message as a char
  /// pointer. This call is convenient for actually sending this OSC messager.
  ///
  /// @return The OSC message as an char*.
  /// @see byte_array
  /// @see size
  ///
  /// <pre>
  ///   int sockfd; // initialize a socket...
  ///   tnyosc::Message* msg; // create a OSC message...
  ///   send_to(sockfd, msg->data(), msg->size(), 0);
  /// </pre>
  ///
  const char* data() const { return get_pointer(byte_array()); }

  /// Returns the size of this OSC message.
  ///
  /// @return Size of the OSC message in bytes.
  /// @see byte_array
  /// @see data
  size_t size() const { return byte_array().size(); }

  /// Clears the message.
  void clear() {
    is_cached_ = false;
    address_.clear();
    types_.clear();
    data_.clear(); }

 private:
  std::string address_;
  ByteArray types_;
  ByteArray data_;
  mutable bool is_cached_;
  mutable ByteArray cache_;

  /// Create the OSC message and store it in cache.
  const ByteArray& create_cache() const {
    is_cached_ = true;
    std::string address(address_);
    if (address.size() == 0) address.assign("/tnyosc");
    size_t addr_len = address.size() + (4 - address.size() % 4);
    size_t types_len = types_.size() + (4 - types_.size() % 4);
    cache_.resize(addr_len + types_len + data_.size());
    std::copy(address_.begin(), address_.end(), cache_.begin());
    std::copy(types_.begin(), types_.end(), cache_.begin() + addr_len);
    std::copy(data_.begin(), data_.end(), cache_.begin() + addr_len + types_len);
    return cache_; }
};

/// This class represents an Open Sound Control bundle message. A bundle can
/// contain any number of Message and Bundle.
class Bundle {
 public:
#ifdef TNYOSC_WITH_BOOST
  typedef boost::shared_ptr<Bundle> Ptr;
#endif

  /// Creates a OSC bundle with timestamp set to immediate. Call set_timetag to
  /// set a custom timestamp.
  Bundle() {
    static std::string id = "#bundle";
    data_.resize(16);
    std::copy(id.begin(), id.end(), data_.begin());
    data_[15] = 1;
  }
  ~Bundle() {}

  // @{
  /// @name Functions for adding Message or Bundle.

  /// Appends an OSC message to this bundle. The message is immediately copied
  /// into this bundle and any changes to the message after the call to this
  /// function does not affect this bundle.
  ///
  /// @param[in] message A pointer to tnyosc::Message.
  void append(const Message* message) { append_data(message->byte_array()); }

  /// Appends an OSC bundle to this bundle. The bundle may include any number
  /// of messages or bundles and are immediately copied into this bundle. Any
  /// changes to the bundle
  void append(const Bundle* bundle) { append_data(bundle->byte_array()); }
  void append(const Message& message) { append_data(message.byte_array()); }
  void append(const Bundle& bundle) { append_data(bundle.byte_array()); }
#ifdef TNYOSC_WITH_BOOST
  void append(const Message::Ptr message) { append_data(message->byte_array()); }
  void append(const Bundle::Ptr bundle) { append_data(bundle->byte_array()); }
#endif
  // @}

  /// Sets timestamp of the bundle.
  ///
  /// @param[in] ntp_time NTP Timestamp
  /// @see get_current_ntp_time
  void set_timetag(uint64_t ntp_time) {
    uint64_t sec = htonl((uint32_t)(ntp_time >> 32));
    uint64_t frac = htonl((uint32_t)ntp_time);
    uint64_t a = sec << 32 | frac;
    ByteArray b(8);
    memcpy(&b[0], (char*)&a, 8);
    data_.insert(data_.begin()+8, b.begin(), b.end()); }

  /// Returns a complete byte array of this OSC bundle as a tnyosc::ByteArray
  /// type.
  ///
  /// @return The OSC bundle as a tnyosc::ByteArray.
  /// @see data
  /// @see size
  const ByteArray& byte_array() const { return data_; }

  /// Returns a pointer to the byte array of this OSC bundle. This call is
  /// convenient for actually sending this OSC bundle.
  ///
  /// @return The OSC bundle as an char*.
  /// @see byte_array
  /// @see size
  ///
  /// <pre>
  ///   int sockfd; // initialize UDP socket...
  ///   tnyosc::Bundle* bundle; // create a OSC bundle...
  ///   send_to(sockfd, bundle->data(), bundle->size(), 0);
  /// </pre>
  ///
  const char* data() const { return get_pointer(data_); }

  /// Returns the size of this OSC bundle.
  ///
  /// @return Size of the OSC bundle in bytes.
  /// @see byte_array
  /// @see data
  size_t size() const { return data_.size(); }

  /// Clears the bundle.
  void clear() { data_.clear(); }

 private:
  ByteArray data_;

  void append_data(const ByteArray& data) {
    int32_t a = htonl(data.size());
    ByteArray b(4 + data.size());
    memcpy(&b[0], (char*)&a, 4);
    std::copy(data.begin(), data.end(), b.begin() + 4);
    data_.insert(data_.end(), b.begin(), b.end()); }
};

} // namespace tnyosc

#endif // __TNY_OSC__


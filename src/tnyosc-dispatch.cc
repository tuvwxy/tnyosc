#include "tnyosc-dispatch.hpp"
#include "tnyosc.hpp"

#include <algorithm>
#include <iostream>

#include <assert.h>
#include <arpa/inet.h>
#include <stdio.h>

using namespace tnyosc;

void print_bytes(const char* bytes, size_t size)
{
  size_t i;
	for (i = 0; i < size; ++i) {
		printf("%02x ", bytes[i]);
		if (i % 16 == 15) {
			printf("\n");
		} else if (i % 4 == 3) {
      printf(" ");
    }
  }
  if (i % 16 != 0) {
    printf("\n");
  }
  printf("\n");
}

bool compare_callback_timetag(const CallbackRef first, const CallbackRef second)
{
  if (first->timetag.tv_sec == second->timetag.tv_sec) {
    return first->timetag.tv_usec <= second->timetag.tv_usec;
  } else {
    return first->timetag.tv_sec < second->timetag.tv_sec;
  }
}

Argument::Argument() 
{
  type = 0;
  size = 0;
  memset(&data, 0, sizeof(data));
}

Argument::Argument(const Argument& a) 
{
  type = a.type;
  size = a.size;
  switch(type) {
    case 's':
    case 'S':
      data.s = strndup(a.data.s, a.size);
      break;
    case'b':
      data.b = malloc(sizeof(size));
      memcpy(data.b, a.data.b, size);
      break;
    default:
      data = a.data;
      break;
  }
}

Argument::~Argument() 
{
  switch(type) {
    case 's':
    case 'S':
    case'b':
      free(data.s);
  }
}

Argument& Argument::operator=(const Argument& a) 
{
  if (this == &a) return *this;
  switch (type) {
    case 's':
    case 'S':
    case 'b':
      free(data.s);
  }

  type = a.type;
  size = a.size;
  switch(type) {
    case 's':
    case 'S':
      data.s = strndup(a.data.s, a.size);
      break;
    case'b':
      data.b = malloc(sizeof(size));
      memcpy(data.b, a.data.b, size);
      break;
    default:
      data = a.data;
  }
  return *this;
}

Dispatcher::Dispatcher() 
  : methods_(0)
{
}

Dispatcher::~Dispatcher() 
{
}

void Dispatcher::add_method(const char* address, const char* types, 
    osc_method method, void* user_data) 
{
  MethodTemplate m;
  m.address = address == NULL ? "" : address;
  m.types = types == NULL ? "" : types;
  m.user_data = user_data;
  m.method = method;
  methods_.push_back(m);
}

std::list<CallbackRef> Dispatcher::match_methods(const char* data, size_t size)
{
  std::list<ParsedMessage> parsed_messages;
  std::list<CallbackRef> callback_list;
  if (!decode_data(data, size, parsed_messages)) return callback_list;
#if TNYOSC_DEBUG
  std::cerr << __FUNCTION__ << ": decode success" << std::endl;
#endif // TNYOSC_DEBUG
  assert(parsed_messages.size() > 0);

  // iterate through all the messages and find matches with registered methods
  std::list<ParsedMessage>::iterator msg_iter = parsed_messages.begin();
  for (; msg_iter != parsed_messages.end(); ++msg_iter) {
#if TNYOSC_DEBUG
    std::cerr << __FUNCTION__ << ": matching " << msg_iter->address << "\n";
#endif // TNYOSC_DEBUG
    std::list<MethodTemplate>::const_iterator method_iter = methods_.begin();
    for (; method_iter != methods_.end(); ++method_iter) {
      if (pattern_match(msg_iter->address, method_iter->address)) {
#if TNYOSC_DEBUG
        std::cerr << "   matched " << method_iter->address << "\n";
#endif // TNYOSC_DEBUG
        // if a method specifies a type, make sure it matches
        if (method_iter->types.empty() ||
            !msg_iter->types.compare(method_iter->types)) {
          CallbackRef callback = CallbackRef(new Callback());
          callback->timetag = msg_iter->timetag;
          callback->address = msg_iter->address;
          callback->argv = msg_iter->argv;
          callback->user_data = method_iter->user_data;
          callback->method = method_iter->method;
          callback_list.push_back(callback);
        }
      }
    }
  }

  callback_list.sort(compare_callback_timetag);

  return callback_list;
}

struct timeval ntp_to_unixtime(uint32_t sec, uint32_t frac)
{
  // time between 1-1-1900 and 1-1-1950
  static const uint64_t epoch = 2208988800UL;

  struct timeval tv;
  if (sec == 0 && frac == 1) {
    memset(&tv, 0, sizeof(tv));
  } else {
    tv.tv_sec = sec - epoch;
    tv.tv_usec = (suseconds_t)((double)frac * 0.0002328306437080);
  }

  return tv;
}

const struct timeval Dispatcher::kZeroTimetag = {0, 0};

bool Dispatcher::decode_data(const char* data, size_t size, 
    std::list<ParsedMessage>& messages, struct timeval timetag)
{
  if (!memcmp(data, "#bundle\0", 8)) {
    // found a bundle
#if TNYOSC_DEBUG
    std::cerr << __FUNCTION__ << ": bundle" << std::endl;
#endif // TNYOSC_DEBUG
    data += 8; size -= 8;

    uint32_t sec, frac;
    memcpy(&sec, data, 4); data += 4; size -= 4;
    memcpy(&frac, data, 4); data += 4; size -= 4;
    sec = ntohl(sec);
    frac = ntohl(frac);

    struct timeval new_timetag = ntp_to_unixtime(sec, frac);

    while (size != 0) {
      uint32_t seg_size;
      memcpy(&seg_size, data, 4); data += 4; size -= 4;
      seg_size = ntohl(seg_size);
      if (seg_size > size) return false;
      if (!decode_data(data, seg_size, messages, new_timetag)) return false;
      data += seg_size; size -= seg_size;
    }
  } else {
#if TNYOSC_DEBUG
    std::cerr << __FUNCTION__ << ": osc" << std::endl;
#endif // TNYOSC_DEBUG
    if (!decode_osc(data, size, messages, timetag)) return false;
  }

  return true;
}

bool Dispatcher::decode_osc(const char* data, size_t size,
    std::list<ParsedMessage>& messages, struct timeval timetag)
{
  const char* head;
  const char* tail;
  unsigned int i = 0;
  size_t remain = size;

  ParsedMessage m;
  m.timetag = timetag;

  // extract address
  head = tail = data;
  while (tail[i] != '\0' && ++i < remain);
  if (i == remain) return false;
  m.address.resize(i);
  std::copy(head, head+i, m.address.begin());
  head += i + (4 - i % 4);
  remain = size - (head - data);
#if TNYOSC_DEBUG
  std::cerr << __FUNCTION__ << ": address = " << m.address << std::endl;
#endif // TNYOSC_DEBUG

  // extract types
  i = 0; 
  tail = head;
  if (head[i++] != ',') return false;
  while (tail[i] != '\0' && ++i < remain);
  if (i == remain) return false;
  m.types.resize(i-1);
  std::copy(head+1, head+i, m.types.begin());
  head += i + (4 - i % 4);
  remain = size - (head - data);
#if TNYOSC_DEBUG
  std::cerr << __FUNCTION__ << ": types = " << m.types << std::endl;
#endif // TNYOSC_DEBUG

  // extract data
  uint32_t int32;
  uint64_t int64;
  m.argv.resize(m.types.size());
  for (unsigned int j = 0; j < m.types.size(); j++) {
    m.argv[j].type = m.types[j];
    switch (m.types[j]) {
      case 'i':
      case 'f':
      case 'r':
        memcpy(&int32, head, 4); 
        int32 = htonl(int32);
        memcpy(&m.argv[j].data.i, &int32, 4);
        m.argv[j].size = 4;
        head += 4; 
        remain -= 4;
        break;
      case 'b':
        memcpy(&int32, head, 4);
        head += 4; 
        remain -= 4;
        int32 = htonl(int32);
        if (int32 > remain) return false;
        m.argv[j].data.b = malloc(int32);
        memcpy(m.argv[j].data.b, head, int32);
        m.argv[j].size = int32;
        head += int32; 
        remain -= int32;
        break;
      case 's':
      case 'S':
        tail = head;
        i = 0;
        while (tail[i] != '\0' && ++i < remain);
        m.argv[j].data.s = strndup((char*)head, i);
        m.argv[j].size = i;
        i += 4 - i % 4;
        head += i;
        remain -= i;
        break;
      case 'h':
      case 'd':
      case 't':
        memcpy(&int64, head, 8);
        int64 = htonll(int64);
        memcpy(&m.argv[j].data.i, &int64, 8);
        m.argv[j].size = 8;
        head += 8;
        remain -= 8;
        break;
      case 'c':
        memcpy(&int32, head, 4);
        m.argv[j].data.c = (char)htonl(int32);
        m.argv[j].size = 1;
        head += 4;
        remain -= 8;
        break;
      case 'm':
        memcpy(&m.argv[j].data.m, head, 4);
        m.argv[j].size = 4;
        head += 4;
        remain -= 4;
        break;
    }
  }

  messages.push_back(m);
#if TNYOSC_DEBUG
  std::cerr << __FUNCTION__ << ": success" << std::endl;
#endif // TNYOSC_DEBUG
  return true;
}

// pattern_match compares two strings and returns true or false depending on if
// they match according to OSC's pattern matching guideline
//
// OSC Pattern Matching Guideline:
// 
//   1. '?' in the OSC Address Pattern matches any single character.
//   2. '*' in the OSC Address Pattern matches any sequence of zero or more 
//      characters.
//   3. A string of characters in square brackets (e.g., "[string]") in the 
//      OSC Address Pattern matches any character in the string. Inside square
//      brackets, the minus sign (-) and exclamation point (!) have special 
//      meanings:
//        o two characters separated by a minus sign indicate the range of 
//          characters between the given two in ASCII collating sequence. (A 
//          minus sign at the end of the string has no special meaning.)
//        o An exclamation point at the beginning of a bracketed string 
//          negates the sense of the list, meaning that the list matches any 
//          character not in the list. (An exclamation point anywhere besides 
//          the first character after the open bracket has no special meaning.)
//   4. A comma-separated list of strings enclosed in curly braces 
//      (e.g., "{foo,bar}") in the OSC Address Pattern matches any of the 
//      strings in the list.
//   5. Any other character in an OSC Address Pattern can match only the same 
//      character.
//
// @param lhs incoming OSC address pattern to match it with rhs's pattern
// @param rhs method address pattern that may contain special characters
bool Dispatcher::pattern_match(const std::string& lhs, const std::string& rhs)
{
  bool negate = false;
  bool mismatched = false;
  std::string::const_iterator seq_tmp;
  std::string::const_iterator seq = lhs.begin();
  std::string::const_iterator seq_end = lhs.end();
  std::string::const_iterator pattern = rhs.begin();
  std::string::const_iterator pattern_end = rhs.end();
  while (seq != seq_end && pattern != pattern_end) {
    switch (*pattern) {
      case '?':
        break;
      case '*':
        // if * is the last pattern, return true
        if (++pattern == pattern_end) return true;
        while (*seq != *pattern && seq != seq_end) ++seq;
        // if seq reaches to the end without matching pattern
        if (seq == seq_end) return false;
        break;
      case '[':
        negate = false;
        mismatched = false;
        if (*(++pattern) == '!') {
          negate = true;
          ++pattern;
        }
        if (*(pattern+1) == '-') {
          // range matching
          char c_start = *pattern; ++pattern;
          //assert(*pattern == '-');
          char c_end = *(++pattern); ++pattern;
          //assert(*pattern == ']');
          // swap c_start and c_end if c_start is larger
          if (c_start > c_end) {
            char tmp = c_start;
            c_end = c_start;
            c_start = tmp;
          }
          mismatched = (c_start <= *seq && *seq <= c_end) ? negate : !negate;
          if (mismatched) return false;
        } else {
          // literal matching
          while (*pattern != ']') {
            if (*seq == *pattern) {
              mismatched = negate;
              break;
            }
            ++pattern;
          }
          if (mismatched) return false;
          while (*pattern != ']') ++pattern;
        }
        break;
      case '{':
        seq_tmp = seq;
        mismatched = true;
        while (*(++pattern) != '}') {
          // this assumes that there's no sequence like "{,a}" where ',' is
          // follows immediately after '{', which is illegal.
          if (*pattern == ',') { 
            mismatched = false;
            break;
          } else if (*seq != *pattern) {
            // fast forward to the next ',' or '}'
            while (*(++pattern) != ',' && *pattern != '}');
            if (*pattern == '}') return false;
            // redo seq matching
            seq = seq_tmp;
            mismatched = true;
          } else {
            // matched 
            ++seq;
            mismatched = false;
          }
        }
        if (mismatched) return false;
        while (*pattern != '}') ++pattern;
        --seq;
        break;
      default: // non-special character
        if (*seq != *pattern) return false;
        break;
    }
    ++seq; ++pattern;
  }
  if (seq == seq_end && pattern == pattern_end) return true;
  else return false;
}


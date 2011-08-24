#include <assert.h>
#include <string>
#include <iostream>

bool pattern_match(const std::string& lhs, const std::string& rhs)
{
  std::cout << std::endl;
  std::cout << "pattern_match(" << lhs << ", " << rhs << ")\n";

  bool negate = false;
  bool mismatched = false;
  std::string::const_iterator seq = lhs.begin();
  std::string::const_iterator seq_end = lhs.end();
  std::string::const_iterator pattern = rhs.begin();
  std::string::const_iterator pattern_end = rhs.end();
  while (seq != seq_end && pattern != pattern_end) {
    std::cout << "seq = " << *seq << ", pattern = " << *pattern << "\n";
    std::string::const_iterator seq_tmp;
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

int main()
{
  std::string seq = "/abc/d";
  std::string pattern;

  // test no special character pattern
  pattern = "/abc/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/ab/d";
  assert(pattern_match(seq, pattern) == false);
  pattern = "/abc/de";
  assert(pattern_match(seq, pattern) == false);

  // test ?
  pattern = "/abc/?";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/?bc/?";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/?/d";
  assert(pattern_match(seq, pattern) == false);

  // test *
  pattern = "/*/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/*/?";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/*/*";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/a*c/d";
  assert(pattern_match(seq, pattern) == true);

  // test []
  pattern = "/[abc]bc/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/[!abc]bc/d";
  assert(pattern_match(seq, pattern) == false);
  pattern = "/abc/[d]";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/abc/[!abc]";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/abc/[!a-c]";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/a[a-c]c/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/[1-9]bc/d";
  assert(pattern_match(seq, pattern) == false);

  // test {}
  pattern = "/{bed,abc,foo}/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/{abc,foo}/d";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/{abcd,foo}/d";
  assert(pattern_match(seq, pattern) == false);
  pattern = "/abc/{a,b,d}";
  assert(pattern_match(seq, pattern) == true);
  pattern = "/abc/{a,b,c}";
  assert(pattern_match(seq, pattern) == false);
  pattern = "/a{blah,bc}/d";
  assert(pattern_match(seq, pattern) == true);
}


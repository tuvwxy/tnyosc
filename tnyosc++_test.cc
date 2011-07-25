#include "tnyosc++.h"
#include <cstdlib>
#include <assert.h>

void print_bytes(const unsigned char* bytes, size_t size)
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

void test_message_data_types()
{
  std::string test_string = "tnyosc";
  char test_cstring[] = "test";
  int array[] = {1, 2, 3, 4, 5};

  tnyosc::Message msg;
  // OSC 1.0 types
  msg.append(1);
  msg.append(3.0f);
  msg.append(test_string);
  msg.append_cstring(test_cstring, strlen(test_cstring));
  msg.append_blob(test_cstring, strlen(test_cstring));
  // OSC 1.1 types
  msg.append_current_time();
  msg.append_true();
  msg.append_false();
  msg.append_null();
  msg.append_impulse();
  // nonstandard types
  msg.append((long long)2);
  msg.append((double)4.0);
  msg.append('!');
  msg.append_midi(1, 0, 0, 255);
  msg.append_array((void*)array, sizeof(array));
  print_bytes(msg.data(), msg.size());
}

void test_message_set_address()
{
  char addr1[] = "test";
  std::string addr2 = "new_string";

  tnyosc::Message msg(addr1);
  assert(msg.address().compare(addr1) == 0);

  msg.set_address(addr2);
  assert(msg.address().compare(addr2) == 0);
}

void test_message_large_data() 
{
  tnyosc::Message msg;
  for (int i = 0; i < 1000; i++) {
    msg.append(i);
  }
  print_bytes(msg.data(), msg.size());
}

void test_message_ptr()
{
  tnyosc::Message* msg = new tnyosc::Message("/msg/ptr");
  msg->append_cstring("pointer", strlen("pointer"));
  print_bytes(msg->data(), msg->size());
  delete msg;
}

#ifdef TNYOSC_WITH_BOOST
void test_message_boost_ptr()
{
  tnyosc::Message::Ptr msg(new tnyosc::Message("/boost/test"));
  for (int i = 0; i < 10; i++) {
    msg->append(i);
  }
  print_bytes(msg->data(), msg->size());
}

void test_bundle_boost_ptr()
{
  tnyosc::Bundle::Ptr bundle(new tnyosc::Bundle());
  bundle->append(tnyosc::Message());
  print_bytes(bundle->data(), bundle->size());
}
#endif

int main(int argc, const char* argv[])
{
  test_message_data_types(); 
  test_message_ptr();
  //test_message_large_data();
#ifdef TNYOSC_WITH_BOOST
  test_message_boost_ptr();
  test_bundle_boost_ptr();
#endif
  
}


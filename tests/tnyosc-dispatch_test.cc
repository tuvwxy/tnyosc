#include "tnyosc-dispatch.hpp"
#include "tnyosc.hpp"

#include <iostream>
#include <assert.h>

#include <UnitTest++/UnitTest++.h>

const std::string TEST1_ADDRESS = "/test1";

void test_method1(const std::string& address, 
    const std::vector<tnyosc::Argument>& argv, 
    void* user_data)
{
  std::cerr << __PRETTY_FUNCTION__ << ": address = " << address << std::endl;
  std::cerr << __PRETTY_FUNCTION__ << ": argv.size = " << argv.size() << std::endl;
  CHECK(address.compare(TEST1_ADDRESS) == 0);
  CHECK(argv.size() == 1);
  CHECK(user_data == NULL);
}

TEST(CallMethodTestMessage)
{
  using namespace tnyosc;
  Message msg("/test1");
  msg.append(1000);
  Bundle bundle;
  bundle.append(msg);
  bundle.append(msg);

  Dispatcher dispatcher;
  dispatcher.add_method(TEST1_ADDRESS.c_str(), NULL, &test_method1, NULL);
  dispatcher.add_method("/test[1-9]", NULL, &test_method1, NULL);
  dispatcher.add_method("/test?", NULL, &test_method1, NULL);
  dispatcher.add_method("/*1", NULL, &test_method1, NULL);
  dispatcher.add_method("/test{1,2,3,4}", NULL, &test_method1, NULL);
  dispatcher.add_method("/test{2,3,4}", NULL, &test_method1, NULL);

  std::list<CallbackRef> callback_list = 
    dispatcher.match_methods(bundle.data(), bundle.size());

  std::cerr << "callback_list.size() = " << callback_list.size() << std::endl;
  CHECK(callback_list.size() == 10);

  std::list<CallbackRef>::iterator it = callback_list.begin();
  for (; it != callback_list.end(); ++it) {
    (*it)->method((*it)->address, (*it)->argv, (*it)->user_data);
  }
}

int main()
{
  return UnitTest::RunAllTests();
}


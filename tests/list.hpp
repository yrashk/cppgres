#pragma once

#include "tests.hpp"

namespace tests {

add_test(list_default_constructor, [](test_case &) {
  bool result = true;

  cppgres::list empty_list;
  result = result && _assert(empty_list.empty());
  result = result && _assert(empty_list.length() == 0);
  result = result && _assert(empty_list.get() == NIL);

  return result;
});
add_test(list_single_element_construction, [](test_case &) {
  bool result = true;

  auto str = cppgres::string("test");
  cppgres::list single_list(str);
  result = result && _assert(!single_list.empty());
  result = result && _assert(single_list.length() == 1);
  result = result && _assert(single_list.get() != NIL);
  result = result && _assert(linitial(single_list) == str);

  return result;
});

add_test(list_variadic_construction, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string("first");
  auto str2 = cppgres::string("second");
  auto str3 = cppgres::string("third");

  cppgres::list multi_list(str1, str2, str3);
  result = result && _assert(multi_list.length() == 3);
  result = result && _assert(linitial(multi_list) == str1);
  result = result && _assert(lsecond(multi_list) == str2);
  result = result && _assert(lthird(multi_list) == str3);

  return result;
});

/*
add_test(list_initializer_list_construction, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("alpha");
  auto str2 = cppgres::string_box("beta");
  auto str3 = cppgres::string_box("gamma");

  cppgres::list init_list{str1, str2, str3};
  result = result && _assert(init_list.length() == 3);
  result = result && _assert(linitial(init_list) == str1);
  result = result && _assert(lsecond(init_list) == str2);
  result = result && _assert(lthird(init_list) == str3);

  return result;
});
 */

add_test(list_empty_initializer_list, [](test_case &) {
  bool result = true;

  cppgres::list empty_init = {};
  result = result && _assert(empty_init.empty());
  result = result && _assert(empty_init.length() == 0);

  return result;
});

add_test(list_large_construction, [](test_case &) {
  bool result = true;

  std::vector<String *> strings;
  for (int i = 0; i < 10; ++i) {
    strings.push_back(cppgres::string(const_cast<char *>(std::to_string(i).c_str())));
  }

  cppgres::list large_list(strings);
  result = result && _assert(large_list.length() == 10);

  // Verify all elements are correct
  for (int i = 0; i < 10; ++i) {
    auto *elem = static_cast<String *>(list_nth(large_list, i));
    result = result && _assert(elem == strings[i]);
  }

  return result;
});

add_test(list_copy_constructor, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string("copy");
  auto str2 = cppgres::string("test");

  cppgres::list original(str1, str2);
  cppgres::list copied(original);

  result = result && _assert(copied.length() == original.length());
  result = result && _assert(copied.get() != original.get()); // Different lists
  result = result && _assert(linitial(copied) == linitial(original));
  result = result && _assert(lsecond(copied) == lsecond(original));

  return result;
});
/*

add_test(list_move_constructor, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("move");
  auto str2 = cppgres::string_box("test");

  cppgres::list original(str1, str2);
  auto *original_ptr = original.get();

  cppgres::list moved(std::move(original));

  result = result && _assert(moved.length() == 2);
  result = result && _assert(moved.get() == original_ptr);
  result = result && _assert(original.get() == NIL); // Moved from
  result = result && _assert(original.empty());

  return result;
});

add_test(list_copy_assignment, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("assign");
  cppgres::list original(str1);

  cppgres::list assigned;
  assigned = original;

  result = result && _assert(assigned.length() == 1);
  result = result && _assert(assigned.get() != original.get());
  result = result && _assert(linitial(assigned) == str1);

  return result;
});

add_test(list_move_assignment, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("move_assign");
  cppgres::list original(str1);
  auto *original_ptr = original.get();

  cppgres::list assigned;
  assigned = std::move(original);

  result = result && _assert(assigned.length() == 1);
  result = result && _assert(assigned.get() == original_ptr);
  result = result && _assert(original.get() == NIL);

  return result;
});

add_test(list_self_assignment_safety, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("self");
  cppgres::list self_list(str1);
  auto *original_ptr = self_list.get();

  self_list = self_list; // Copy self-assignment
  result = result && _assert(self_list.get() != NIL);
  result = result && _assert(self_list.length() == 1);

  self_list = std::move(self_list); // Move self-assignment
  result = result && _assert(self_list.get() != NIL);
  result = result && _assert(self_list.length() == 1);

  return result;
});

add_test(list_append_method, [](test_case &) {
  bool result = true;

  cppgres::list dynamic_list;
  auto str1 = cppgres::string_box("first");
  auto str2 = cppgres::string_box("second");

  dynamic_list.append(str1);
  result = result && _assert(dynamic_list.length() == 1);
  result = result && _assert(linitial(dynamic_list) == str1);

  dynamic_list.append(str2);
  result = result && _assert(dynamic_list.length() == 2);
  result = result && _assert(lsecond(dynamic_list) == str2);

  return result;
});

add_test(list_prepend_method, [](test_case &) {
  bool result = true;

  cppgres::list prepend_list;
  auto str1 = cppgres::string_box("second");
  auto str2 = cppgres::string_box("first");

  prepend_list.append(str1);
  prepend_list.prepend(str2);

  result = result && _assert(prepend_list.length() == 2);
  result = result && _assert(linitial(prepend_list) == str2);
  result = result && _assert(lsecond(prepend_list) == str1);

  return result;
});

add_test(list_method_chaining, [](test_case &) {
  bool result = true;

  cppgres::list chain_list;
  auto str1 = cppgres::string_box("one");
  auto str2 = cppgres::string_box("two");
  auto str3 = cppgres::string_box("three");

  chain_list.append(str1).append(str2).append(str3);

  result = result && _assert(chain_list.length() == 3);
  result = result && _assert(linitial(chain_list) == str1);
  result = result && _assert(lsecond(chain_list) == str2);
  result = result && _assert(lthird(chain_list) == str3);

  return result;
});

add_test(list_implicit_conversion, [](test_case &) {
  bool result = true;

  auto str1 = cppgres::string_box("convert");
  cppgres::list convert_list(str1);

  // This should compile and work due to implicit conversion
  ::List *pg_list = convert_list;
  result = result && _assert(pg_list != NIL);
  result = result && _assert(list_length(pg_list) == 1);
  result = result && _assert(linitial(pg_list) == str1);

  return result;
});

add_test(list_oid_factory, [](test_case &) {
  bool result = true;

  Oid oid1 = 16384;
  Oid oid2 = 16385;
  Oid oid3 = 16386;

  auto oid_list = cppgres::list::make_oid_list(oid1, oid2, oid3);

  result = result && _assert(oid_list.length() == 3);
  result = result && _assert(oid_list.get()->type == T_OidList);
  result = result && _assert(linitial_oid(oid_list) == oid1);
  result = result && _assert(lsecond_oid(oid_list) == oid2);
  result = result && _assert(lthird_oid(oid_list) == oid3);

  return result;
});

add_test(list_int_factory, [](test_case &) {
  bool result = true;

  int int1 = 42;
  int int2 = 24;
  int int3 = 99;

  auto int_list = cppgres::list::make_int_list(int1, int2, int3);

  result = result && _assert(int_list.length() == 3);
  result = result && _assert(int_list.get()->type == T_IntList);
  result = result && _assert(linitial_int(int_list) == int1);
  result = result && _assert(lsecond_int(int_list) == int2);
  result = result && _assert(lthird_int(int_list) == int3);

  return result;
});

add_test(list_empty_typed_lists, [](test_case &) {
  bool result = true;

  auto empty_oids = cppgres::list::make_oid_list();
  auto empty_ints = cppgres::list::make_int_list();

  result = result && _assert(empty_oids.empty());
  result = result && _assert(empty_ints.empty());

  return result;
});

add_test(list_memory_allocation_optimization, [](test_case &) {
  bool result = true;

  // Small list should get power-of-2 allocation
  auto str = cppgres::string_box("small");
  cppgres::list small_list(str);

  ::List *pg_list = small_list.get();
  result = result && _assert(pg_list->max_length >= pg_list->length);

  // For very small lists, should get at least minimum allocation
  result = result && _assert(pg_list->max_length >= 4); // Minimum after overhead

  return result;
});

add_test(list_range_constructor, [](test_case &) {
  bool result = true;

  std::vector<String *> strings = {cppgres::string_box("range1"), cppgres::string_box("range2"),
                                   cppgres::string_box("range3")};

  cppgres::list range_list(strings.begin(), strings.end());

  result = result && _assert(range_list.length() == 3);
  result = result && _assert(linitial(range_list) == strings[0]);
  result = result && _assert(lsecond(range_list) == strings[1]);
  result = result && _assert(lthird(range_list) == strings[2]);

  return result;
});

add_test(list_container_constructor, [](test_case &) {
  bool result = true;

  std::vector<String *> strings = {cppgres::string_box("container1"),
cppgres::string_box("container2")};

  cppgres::list container_list(strings);

  result = result && _assert(container_list.length() == 2);
  result = result && _assert(linitial(container_list) == strings[0]);
  result = result && _assert(lsecond(container_list) == strings[1]);

  return result;
});

add_test(list_mixed_pointer_types, [](test_case &) {
  bool result = true;

  auto str = cppgres::string_box("mixed");
  auto int_val = makeInteger(42);
  auto float_val = makeFloat(3.14);

  cppgres::list mixed_list(str, int_val, float_val);

  result = result && _assert(mixed_list.length() == 3);
  result = result && _assert(linitial(mixed_list) == str);
  result = result && _assert(lsecond(mixed_list) == int_val);
  result = result && _assert(lthird(mixed_list) == float_val);

  return result;
});

add_test(list_large_oid_list, [](test_case &) {
  bool result = true;

  // Use variadic expansion (limited by template instantiation)
  auto large_oid_list = cppgres::list::make_oid_list(16384, 16385, 16386, 16387, 16388);

  result = result && _assert(large_oid_list.length() == 5);
  result = result && _assert(large_oid_list.get()->type == T_OidList);

  for (int i = 0; i < 5; ++i) {
    Oid expected = 16384 + i;
    Oid actual = list_nth_oid(large_oid_list, i);
    result = result && _assert(actual == expected);
  }

  return result;
});

add_test(list_destruction_safety, [](test_case &) {
  bool result = true;

  // Test that destructors work correctly in various scenarios
  {
    cppgres::list temp_list(cppgres::string_box("temp"));
    // Should destroy cleanly when going out of scope
  }

  {
    cppgres::list temp1(cppgres::string_box("temp1"));
    cppgres::list temp2 = temp1;            // Copy
    cppgres::list temp3 = std::move(temp1); // Move
                                            // All should destroy cleanly
  }

  result = result && _assert(true); // If we get here, no crashes occurred

  return result;
});
 */

} // namespace tests

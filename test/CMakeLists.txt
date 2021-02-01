include_directories(..)

add_executable(test_empty test_empty.c)
add_test(test_empty test_empty)
add_executable(test_jan_dead test_deadLock_test.c)
add_test(test_jan_dead test_jan_dead)

set_tests_properties(test_empty PROPERTIES TIMEOUT 1)

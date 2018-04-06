#ifndef TEST_REGISTRY_HPP
#define TEST_REGISTRY_HPP

#include <string>

void
register_test(const std::string & name, int (*test)(const std::string &));

int
run_test(const std::string & name);

#define TEST_REGISTER(name, test) \
    static void __attribute__((constructor)) register_test(void) \
    { \
        register_test(name, test); \
    }

#endif

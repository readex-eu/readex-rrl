#include "test-registry.hpp"

#include <assert.h>

int main(int argc, char **argv)
{
    assert(argc == 2);
    return run_test(argv[1]);
}

#include "test-registry.hpp"

#include <assert.h>
#include <memory>
#include <unordered_map>

class unit_test
{
public:
    unit_test(int (*t)(const std::string &file_path), const std::string &file_path)
        : file_path_(file_path), test_(t)
    {
    }

    inline int test() const
    {
        return test_(file_path_);
    }

private:
    std::string file_path_;
    int (*test_)(const std::string &file_path);
};

static std::unordered_map<std::string, std::unique_ptr<unit_test>> test_map;

void register_test(const std::string &name, int (*test)(const std::string &))
{
    std::string file(__FILE__);
    size_t found = file.find_last_of("/\\");
    std::string file_path = file.substr(0, found) + "/" + name + ".json";
    test_map.insert(std::make_pair(name, std::make_unique<unit_test>(test, file_path)));
}

int run_test(const std::string &name)
{
    assert(test_map.find(name) != test_map.end());
    return test_map[name]->test();
}

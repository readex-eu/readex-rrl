#include <util/environment.hpp>

#include <fstream>
#include <string>
#include <vector>

namespace rrl
{

/** This class is used to filter certain regions,
 * which are not required in the measurement.
 */
class filter
{
public:
    filter();
    ~filter();

    void parseFilterFile(std::vector<std::string> rawData);

    std::vector<std::string> splithelper(std::string str);

    bool check_region(std::string region_name);

	std::string removeSpaces(std::string str);

private:
    std::string file_name;
    std::ifstream inFile;

    bool no_filtering = false; /** Disables filtering if true*/
    bool include_all = true;  /**If true regions need to be excluded explicitly*/

    /** Holds all region names, that should be excluded.
     * Empty if all regions should be excluded and only some included.
     **/
    std::vector<std::string> excluded_regions;
    /** Holds all region names, that should be included.
     * Empty if all regions should be included and only some are excluded.
     **/
    std::vector<std::string> included_regions;
};
}

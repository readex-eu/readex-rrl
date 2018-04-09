#include <rrl/filter.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fnmatch.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace rrl
{
/**
 * Constructor
 *
 * @brief Creates and initializes the filter.
 * It already parses the filtering file.
 *
 *
 **/
filter::filter()
{
    logging::debug("FILTER") << "Filter initialization";
    file_name = environment::get("FILTERING_FILE", "");
    if (file_name.empty())
    {
        no_filtering = true;
        logging::debug("FILTER") << "no filter file found!!";
    }
    std::string state = no_filtering ? "disabled." : "enabled.";
    logging::debug("FILTER") << "Filtering is " << (no_filtering ? "disabled." : "enabled.");

    if (!no_filtering)
    {
        inFile.open(file_name, std::ifstream::in);
        if (!inFile)
        {
            logging::warn("FILTER") << "can not open filter file";
            no_filtering = true;
            return;
        }

        std::string rawInput;
        std::vector<std::string> rawData;
        while (!inFile.eof())
        {
            std::getline(inFile, rawInput);
            rawData.push_back(rawInput);
        }
        rawData.pop_back();

        filter::parseFilterFile(rawData);
    }
}

/**
 * Destructor
 *
 * Deletes the filter object.
 *
 *
 **/

filter::~filter()
{
}

/**
 * This function implements parsing of the filtering file input
 * and puts the region names in the include or exclude vector.
 * @brief This function implements parsing of the filtering file input and
 * inserts the region names in the include or exclude vector.
 *
 * @param rawData raw input read from the filter file.
 *
 *
 **/
void filter::parseFilterFile(std::vector<std::string> rawData)
{
    // check if right filter file syntax
    if (rawData.front().find("SCOREP_REGION_NAMES_BEGIN") == std::string::npos ||
        rawData.back().find("SCOREP_REGION_NAMES_END") == std::string::npos)
    {
        logging::warn("FILTER") << "The file seems to have the wrong syntax.";
        return;
    }
    rawData.erase(rawData.begin());
    rawData.erase(rawData.end());
    bool excluding_names = true;
    for (auto &elem : rawData)
    {
        // removing comments
        size_t pos;
        pos = elem.find("#");
        if (pos != std::string::npos)
        {
			//special case if '#' is part of a name and it is escaped with a backslash 
			if(elem.find("\\#") == std::string::npos)
			{
				elem.erase(pos, elem.size());
			}
        }
        // removing '\t' and spaces 
        elem.erase(std::remove(elem.begin(), elem.end(), '\t'), elem.end());
		elem = removeSpaces(elem);

        if (elem.find("EXCLUDE") != std::string::npos)
        {
            if (elem.find("*") != std::string::npos)
            {
                logging::debug("FILTER") << "set on EXCLUDE everything";
                include_all = false;
                // temp variable for sorting regionnames in the right vectors
                excluding_names = false;
                continue;
            }
            else
            {
                excluding_names = true;
                continue;
            }
        }
        else if (elem.find("INCLUDE") != std::string::npos)
        {
            // check for comments and first region names
            if (elem.find("*") != std::string::npos)
            {
                include_all = true;
                excluding_names = true;
                continue;
            }
            else
            {
                excluding_names = false;
				include_all = false;
                continue;
            }
        }
        else
        {
            if (excluding_names)
            {
                excluded_regions.push_back(elem);
            }
            else
            {
                included_regions.push_back(elem);
            }
        }
    }
	if((include_all && !included_regions.empty()) ||
			(!include_all && !excluded_regions.empty()))
	{
		logging::warn("FILTER") << "false use of filterfile";
		no_filtering = true;
	}
}

/**
 * This function checks if the given region should be included or excluded.
 *
 * @brief This function checks with a given region name, if a region
 * should be included or excluded and returns true for include and false for exclude.
 *
 * @param region_name name of the region that should be checked
 *
 *
 **/
bool filter::check_region(std::string region_name)
{
    if (no_filtering)
	{
        return true;
	}
    if (include_all == true)
    {
        for (std::string member : excluded_regions)
        {
            if (fnmatch(member.c_str(), region_name.c_str(), 0) == 0)
			{
                return false;
			}
        }
        return true;
    }
    else
    {
        for (std::string member : included_regions)
        {
            if (fnmatch(member.c_str(), region_name.c_str(), 0) == 0)
            {
                return true;
            }
        }
        return false;
    }
    return false;
}
/**
 * This helper function is used to remove unnecessary leading and tailing spaces 
 * of entries in the filter file.
 *
 * @brief This helper function is used to remove unnecessary leading and tailing spaces 
 * of entries in the filter file 
 *
 * @param entry of the filter file
 *
 **/
std::string filter::removeSpaces(std::string str)
{
	for(unsigned int idx = 0; idx < str.length();++idx)
	{
		if(str[idx] != ' ')
		{
			if(idx == 0)
			{
				break;
			}
			str.erase(str.begin(), str.begin()+idx);
			break;
		}
	}

	for(unsigned int idx = 0; idx < str.length(); ++idx)
	{
		if(str[str.length()-1 - idx] != ' ')
		{
			if(idx == 0)
			{
				break;
			}
			str.erase(str.end()-idx, str.end());
			break;
		}
	}
	return str;
}
}

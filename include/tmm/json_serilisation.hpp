/** Json serilisation
 * https://github.com/nlohmann/json#basic-usage
 */

#ifndef INCLUDE_TMM_JSON_SERILISATION_HPP_
#define INCLUDE_TMM_JSON_SERILISATION_HPP_

#include <json.hpp>

#include <tmm/identifiers.hpp>
#include <tmm/simple_callpath.hpp>
#include <tmm/tuning_model_manager.hpp>

namespace rrl
{
namespace tmm
{

class serialisation_error : public std::exception
{
public:
    serialisation_error(const std::string &what_arg, std::vector<simple_callpath_element> &elem)
    {
        std::stringstream ss;
        ss << elem;
        message = what_arg;
        message += " for RTS: ";
        message += ss.str();
    }

    const char *what() const noexcept override
    {
        return message.c_str();
    }

private:
    std::string message;
};

template <typename T> inline void to_json(nlohmann::json &j, const identifier<T> &i)
{
    j = nlohmann::json{{"id", i.id}, {"value", i.value}};
}

template <typename T> inline void from_json(const nlohmann::json &j, identifier<T> &i)
{
    i.id = j.at("id").get<std::size_t>();
    i.value = j.at("ints").get<T>();
}

inline void to_json(nlohmann::json &j, const identifier_set &i)
{
    j = nlohmann::json{{"uints", i.uints}, {"ints", i.ints}, {"strings", i.strings}};
}

inline void from_json(const nlohmann::json &j, identifier_set &i)
{
    i.uints = j.at("uints").get<std::vector<identifier<std::uint64_t>>>();
    i.ints = j.at("ints").get<std::vector<identifier<std::int64_t>>>();
    i.strings = j.at("strings").get<std::vector<identifier<std::string>>>();
}

inline void to_json(nlohmann::json &j, const simple_callpath_element &s)
{
    auto tmm = tmm::get_tuning_model_manager("");
    j = nlohmann::json{{"id_set", s.id_set},
        {"calibrate", s.calibrate},
        {"fn_name", tmm->get_name_from_region_id(s.region_id)}};
}

inline void from_json(const nlohmann::json &j, simple_callpath_element &s)
{
    auto tmm = tmm::get_tuning_model_manager("");
    s.region_id = tmm->get_id_from_region_name(j.at("fn_name").get<std::string>());
    s.id_set = j.at("id_set").get<identifier_set>();
    s.calibrate = j.at("calibrate").get<bool>();
}

}; // namespace tmm
}; // namespace rrl

#endif /* INCLUDE_TMM_JSON_SERILISATION_HPP_ */

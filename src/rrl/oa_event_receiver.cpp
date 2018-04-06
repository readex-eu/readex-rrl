/*
 * oa_event_receiver.cpp
 *
 *  Created on: 10.08.2016
 *      Author: andreas
 */

#include <json.hpp>
#include <rrl/oa_event_receiver.hpp>
#include <unordered_map>
#include <util/log.hpp>

namespace rrl
{

using namespace nlohmann; // ok I don't like this, but it make thinks a lot easier.

/** Basic initialiser
 *
 * Initializes the oa_event receiver.
 *
 * @param tmm shared_ptr to the tuning model manager.
 *
 *
 */
oa_event_receiver::oa_event_receiver(std::shared_ptr<tmm::tuning_model_manager> tmm) : tmm_(tmm)
{
    logging::debug("OA") << "init";
}

/**
 * Basic destructor
 */
oa_event_receiver::~oa_event_receiver()
{
    logging::debug("OA") << "fini";
}

/** parses the oa command
 *
 * parese a OA command in the form:
 *
 * {
 *  "genericEventType":"tuningRequest",
 *  "genericEventTypeVersion":0.1,
 *  "data":
 *  {
 *      ...
 *  }
 * }
 *
 * Dependig on genericEventTypeVersion different functions to parse the
 * data are called. The functions save the confguration in the
 * \ref tmm::tuning_model_manager
 *
 * V 0.1 calles parse_command_v0_1()
 * V 0.2 calles parse_command_v0_2()
 *
 * @param command json formated oa command
 *
 */
void oa_event_receiver::parse_command(std::string command)
{
    logging::debug("OA") << "parsing commnad:\n" << command;

    json command_json;

    /*
     * nasty error checking ... well, it's json ...
     */
    try
    {
        command_json = json::parse(command);

        if (command_json["genericEventType"] != "tuningRequest")
        {
            // Not a oa tuning command. I don't care about this one.
            return;
        }
        if (command_json["genericEventTypeVersion"].get<double>() == 0.1)
        {
            parse_command_v0_1(command_json);
        }
        else if (command_json["genericEventTypeVersion"].get<double>() == 0.2)
        {
            parse_command_v0_2(command_json);
        }
        else
        {
            logging::warn("OA") << "Mismatching genericEventTypeVersion."
                                   " Found: "
                                << command_json["genericEventTypeVersion"].get<double>()
                                << " Required: 0.1 or 0.2";
        }
    }
    catch (std::exception &e)
    {
        /* if anything wents wrong lets hope for an exception.
         * Lets catch it and print an error (avoiding killing the whole
         * program.
         */
        logging::error("OA") << "Maleformed generic command."
                                "Please check! Command was:\n"
                             << command << "Error was: " << e.what();
        return;
    }
}

/** parses the genericEventTypeVersion of 0.1
 *
 *  Data is expected to have the form:
 *
 *  "data":
 *  {
 *      "RegionName":"\<Region Name\>",
 *      "RegionID":\<Region Id\>,
 *      "TuningParametes":
 *      {
 *          "coreFrequnecy":2500000,
 *          "ompNumThreads":24
 *      }
 *  }
 *
 * Deletes the old configuration and saves the parsed configuration in the Tuning Model Manager
 *
 * Therfore the by parameter_name_hash() hashed name is passend to the
 * \ref tmm::tuning_model_manager together with the value and the affected
 * region id
 *
 * @param command_json nlohmann::json parsed json version
 *
 */
void oa_event_receiver::parse_command_v0_1(const json &command_json)
{
    auto tuning_parameters =
        command_json["data"]["TuningParameters"].get<std::unordered_map<std::string, json>>();

    std::vector<tmm::parameter_tuple> tuning_parameters_for_tmm;
    for (auto &tuning_parameter : tuning_parameters)
    {
        if (!tuning_parameter.second.is_number_integer())
        {
            // skip this parameter
            continue;
        }
        logging::debug("OA") << "hash of \"" << tuning_parameter.first << "\" is ("
                             << parameter_name_hash(tuning_parameter.first) << ")";
        tuning_parameters_for_tmm.push_back(tmm::parameter_tuple(
            parameter_name_hash(tuning_parameter.first), tuning_parameter.second.get<int>()));
    }

    auto elem = tmm::simple_callpath_element(
        static_cast<uint32_t>(command_json["data"]["RegionID"].get<int>()), tmm::identifier_set());

    logging::debug("OA") << "clean old tmm";
    tmm_->clear_configurations();
    /*
     * now send everything to the tmm
     */
    logging::debug("OA") << "save new tmm";
    tmm_->store_configuration(std::vector<tmm::simple_callpath_element>(1, elem),
        tuning_parameters_for_tmm,
        std::chrono::milliseconds::max());
}

/** parses the genericEventTypeVersion of 0.2
 *
 *  Data is expected to have the form:
 *
 * "data":
 *
 * {
 *      "RegionCallpath":
 *      [
 *          {
 *              "RegionID":<Region Id>,
 *              "AdditionalIdentifiersInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersUInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersString":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              }
 *          },
 *          {
 *              "RegionID":<Region Id>,
 *              "AdditionalIdentifiersInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersUInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersString":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              }
 *          },
 *          {
 *              "RegionID":<Region Id>,
 *              "AdditionalIdentifiersInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersUInt":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              },
 *              "AdditionalIdentifiersString":
 *              {
 *                  <Name1>:<Value1>,
 *                  <Name2>:<Value2>,
 *                  <Name3>:<Value3>
 *              }
 *          }
 *      ],
 *      "TuningParametes":
 *      {
 *          "coreFrequnecy":2500000,
 *          "ompNumThreads":24
 *      }
 *  }
 *
 * Deletes the old config from the tmm and saves the parsed configuration in the Tuning Model
 * Manager
 *
 * Therfore the by parameter_name_hash() hashed name is passend to the
 * \ref tmm::tuning_model_manager together with the value and the affected
 * region id
 *
 * @param command_json nlohmann::json parsed json version
 *
 */
void oa_event_receiver::parse_command_v0_2(const json &command_json)
{

    std::hash<std::string> hash_add_id_name;
    auto callpath_json = command_json["data"]["RegionCallpath"].get<std::vector<json>>();

    std::vector<tmm::simple_callpath_element> callpath;

    for (auto &call_elem_json : callpath_json)
    {
        tmm::identifier_set add_ids;

        auto add_ids_json_int =
            call_elem_json["AdditionalIdentifiersInt"].get<std::map<std::string, std::int64_t>>();
        for (auto &id : add_ids_json_int)
        {
            add_ids.ints.push_back(
                tmm::identifier<std::int64_t>(hash_add_id_name(id.first), id.second));
        }

        auto add_ids_json_uint =
            call_elem_json["AdditionalIdentifiersUInt"].get<std::map<std::string, std::uint64_t>>();
        for (auto &id : add_ids_json_uint)
        {
            add_ids.uints.push_back(
                tmm::identifier<std::uint64_t>(hash_add_id_name(id.first), id.second));
        }

        auto add_ids_json_string =
            call_elem_json["AdditionalIdentifiersString"].get<std::map<std::string, json>>();
        for (auto &id : add_ids_json_string)
        {
            add_ids.strings.push_back(
                tmm::identifier<std::string>(hash_add_id_name(id.first), id.second));
        }

        callpath.push_back(tmm::simple_callpath_element(call_elem_json["RegionID"], add_ids));
    }

    auto tuning_parameters_json =
        command_json["data"]["TuningParameters"].get<std::unordered_map<std::string, json>>();

    std::vector<tmm::parameter_tuple> tuning_parameters;
    for (auto &tuning_parameter : tuning_parameters_json)
    {
        logging::debug("OA") << "hash of \"" << tuning_parameter.first << "\" is ("
                             << parameter_name_hash(tuning_parameter.first) << ")";

        tuning_parameters.push_back(tmm::parameter_tuple(
            parameter_name_hash(tuning_parameter.first), tuning_parameter.second.get<int>()));
    }

    logging::debug("OA") << "clean old tmm";
    tmm_->clear_configurations();
    logging::debug("OA") << "save new tmm";
    tmm_->store_configuration(callpath, tuning_parameters, std::chrono::milliseconds::max());
}
}

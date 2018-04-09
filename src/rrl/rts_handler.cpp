#include <nitro/dl/dl.hpp>
#include <rrl/rts_handler.hpp>
#include <util/environment.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace rrl
{
/**
 * Constructor
 *
 * @brief Constructor
 *
 * Initializes the rts handler.
 * It gets the reference to the instance of tuning model manager.
 *
 * @param tmm shared pointer to a tuning model manager instance
 *
 **/
rts_handler::rts_handler(
    std::shared_ptr<tmm::tuning_model_manager> tmm, std::shared_ptr<cal::calibration> cal)
    : is_inside_root(false),
      tmm_(tmm),
      cal_(cal),
      pc_(parameter_controller::instance()),
      call_tree_(std::make_unique<call_tree::region_node>(
          nullptr, call_tree::node_info(0, call_tree::node_type::root))),
      current_calltree_elem_(call_tree_.get()),
      region_status_(tmm::insignificant)

{
    logging::debug("RTS") << " initializing";

    try
    {
        significant_duration = std::chrono::milliseconds(
            std::stoi(rrl::environment::get("SIGNIFICANT_DURATION_MS", "100")));
    }
    catch (std::invalid_argument &e)
    {
        logging::fatal("RTS")
            << "Invalid argument given for SCOREP_RRL_SIGNIFICANT_DURATION_MS. Value "
               "hase to be a positiv int.";
        throw;
    }
    catch (std::out_of_range &e)
    {
        logging::fatal("RTS") << "Value of SCOREP_RRL_SIGNIFICANT_DURATION out of range.";
        throw;
    }

    auto input_id_file = rrl::environment::get("INPUT_IDENTIFIER_SPEC_FILE", "");
    if (!input_id_file.empty())
    {
        parse_input_identifier_file(input_id_file);
    }
}

/**
 * Destructor
 *
 * @brief Destructor
 * Deletes the rts handler
 *
 **/
rts_handler::~rts_handler()
{
    logging::debug("RTS") << " finalizing";
}

/** Checks if all information for loading the config are present, and loads the
 * current configuration.
 *
 * This function has three main purposes:
 * * check if all expected additional identifiers are collected
 * * check if the region duration is sufficient long
 * * check if callibration is needed.
 * The check have to occure in this order. Just when all additional identifiers are collected, we
 * can request the excecution time. The execution time again indicate if a region needs
 * recallibration. However this is currently not implemented.
 *
 *
 */
void rts_handler::load_config()
{
    if (tmm_->has_changed())
    {
        call_tree_->reset_state();
    }
    if (current_calltree_elem_->info.state == call_tree::node_state::unknown)
    {
        logging::trace("RTS") << "ENTER State: call_tree::node_state::unknown.";
        
        if (tmm_->is_significant(current_calltree_elem_->info.region_id) == tmm::significant)
        {
            auto call_path = current_calltree_elem_->build_callpath();
            current_calltree_elem_->set_configuration(
                tmm_->get_current_rts_configuration(call_path, input_identifiers_));
            if (current_calltree_elem_->get_configuration().size() != 0)
            {
                /** Workaround. There is an assertion that the callpath is in the TM, what is not
                 * neccessary the case. The node measurs its duration itslef.
                 **/
                current_calltree_elem_->info.duration = tmm_->get_exectime(call_path);
            }
            current_calltree_elem_->info.state = call_tree::node_state::known;
            logging::trace("RTS") << "ENTER Change State to : call_tree::node_state::known.";
        }
        else
        {
            current_calltree_elem_->info.state = call_tree::node_state::measure_duration;
            logging::trace("RTS") << "ENTER Change State to : call_tree::node_state::measure_duration.";
            current_calltree_elem_->start_measurment();
        }
    }

    if (current_calltree_elem_->info.state == call_tree::node_state::known)
    {
        logging::trace("RTS") << "ENTER State: call_tree::node_state::known.";
        if ((current_calltree_elem_->get_configuration().size() > 0) &&
            (current_calltree_elem_->info.duration > significant_duration))
        {
            pc_.set_parameters(current_calltree_elem_->get_configuration());
            current_calltree_elem_->info.configs_set++;
        }
    }
    else if (current_calltree_elem_->info.state == call_tree::node_state::calibrate)
    {
        logging::trace("RTS") << "ENTER State: call_tree::node_state::calibrate.";
        /* If the parent node decided to calibrate, we better don't. Similar if the parente has not
         * decided yet and is still in measrument.
         * 
         * This is a bead idea if the parent measures for ever, like when main is to be measured.
         *
         */
/*        
        logging::trace("RTS") << "current_calltree_elem_->parent_->info.state != call_tree::node_state::calibrate : " << (current_calltree_elem_->parent_->info.state != call_tree::node_state::calibrate?"true":"false");
        logging::trace("RTS") << "current_calltree_elem_->parent_->info.state != call_tree::node_state::measure_duration : " << (current_calltree_elem_->parent_->info.state != call_tree::node_state::measure_duration?"true":"false");
        if ((current_calltree_elem_->parent_->info.state != call_tree::node_state::calibrate) and
            (current_calltree_elem_->parent_->info.state !=
                call_tree::node_state::measure_duration))
           //Just in case someone wants to know if I know what I am doing ... I don't :D.
           //But at least for training this needs to be commented out. Otherwise the freq will never be changed.
           //Simply do whatever cal decides.
*/                
        {
            auto conf = cal_->calibrate_region(current_calltree_elem_->info.region_id);
            current_calltree_elem_->set_configuration(conf);
            pc_.set_parameters(current_calltree_elem_->get_configuration());
            current_calltree_elem_->info.configs_set++;
        }
    }
}

/**This function handles the enter regions.
 *
 * This function handles the enter regions.
 * It gets the region id and maintains the callpath.
 *
 * It passes region id to tuning model manger to detect if a region is a
 * significant rts.
 *
 * After detecting the rts, configurations for the respective rts is requested
 * from tuning model manger. This configuration is passed to parameter
 * controller for setting the parameters.
 *
 * @param region_id Score-P region identifier
 * @param locationData can be used with scorep::location_get_id() and
 *        scorep::location_get_gloabl_id() to obtain the location of the call.
 **/
void rts_handler::enter_region(uint32_t region_id, SCOREP_Location *locationData)
{
    auto elem = tmm::simple_callpath_element(region_id, tmm::identifier_set());
    if (!is_inside_root)
    {
        if (tmm_->is_root(elem))
        {
            is_inside_root = true;
            logging::trace("RTS") << "tmm_->is_root(elem) = true";
        }
        else
        {
            logging::trace("RTS") << "tmm_->is_root(elem) = false";
            return;
        }
    }
    logging::trace("RTS") << "is_inside_root = true";

    current_calltree_elem_ = current_calltree_elem_->enter_node(region_id);

    load_config();
}

/**This function handles the exit regions.
 *
 * This function handles the exit regions.
 * It gets the region id and maintains the callpath.
 *
 * @param region_id Score-P region identifier
 * @param locationData can be used with scorep::location_get_id() and
 *        scorep::location_get_gloabl_id() to obtain the location of the call.
 **/
void rts_handler::exit_region(uint32_t region_id, SCOREP_Location *locationData)
{
    if (!is_inside_root)
    {
        return;
    }

    auto elem = tmm::simple_callpath_element(region_id, tmm::identifier_set());
    if ((current_calltree_elem_->parent_->info.type == call_tree::node_type::root) &&
        (tmm_->is_root(elem) == true))
    {
        is_inside_root = false;
    }

    if (current_calltree_elem_->info.region_id != elem.region_id)
    {
        logging::error("RTS") << "callpath not intact. RegionId removed from "
                              << "the callpath: " << current_calltree_elem_->info.region_id
                              << " RegionId "
                              << "passed by Score-P: " << region_id;
        logging::error("RTS") << "full callpath_elem on stack:\n"
                              << current_calltree_elem_->build_callpath();
        logging::error("RTS") << "full elem from region:\n" << elem;
        logging::error("RTS") << "won't change the callpath.";
    }

    if (current_calltree_elem_->info.state == call_tree::node_state::calibrate)
    {
        logging::trace("RTS") << "EXIT State: call_tree::node_state::calibrate.";
        /* If the parent node decided to calibrate, we didn't. Similar if the parente has not
         * decided yet and is still in measrument.
         */
        if ((current_calltree_elem_->parent_->info.state != call_tree::node_state::calibrate) and
            (current_calltree_elem_->parent_->info.state !=
                call_tree::node_state::measure_duration))
        {
            if (!cal_->keep_calibrating())
            {
                current_calltree_elem_->set_configuration(
                    cal_->request_configuration(current_calltree_elem_->info.region_id));

                tmm_->store_configuration(current_calltree_elem_->build_callpath(),
                    current_calltree_elem_->get_configuration(),
                    current_calltree_elem_->info.duration);
                current_calltree_elem_->info.state = call_tree::node_state::known;
                logging::trace("RTS") << "EXIT Change State to : call_tree::node_state::known.";
            }
        }
    }
    else if (current_calltree_elem_->info.state == call_tree::node_state::measure_duration)
    {
        logging::trace("RTS") << "EXIT State: call_tree::node_state::measure_duration.";
        current_calltree_elem_->stop_measurment();
        if (current_calltree_elem_->callibrate_region(significant_duration))
        {
            logging::trace("RTS") << "EXIT Change State: call_tree::node_state::calibrate.";
            current_calltree_elem_->info.state = call_tree::node_state::calibrate;
        }
        else
        {
            logging::trace("RTS") << "EXIT Change State: call_tree::node_state::known.";
            current_calltree_elem_->info.state = call_tree::node_state::known;
        }
    }

    for (int i = 0; i < current_calltree_elem_->info.configs_set; i++)
    {
        /* if we have set different configs together with different additional identifiers, we need
         * to reset them all.
         * Example:
         * * region_id = 0
         * * region_id = 1
         * * add_id foo=1 --> congig(cpufreq = 2.5GHz)
         * * add_id baz=1 --> congig(cpufreq = 2.0GHz)
         * When we now call return_to_parent() we need to unset both add_id foo and add_id baz
         * But we get only on exit. So we need to collect them, and reset them at once.
         */
        pc_.unset_parameters();
    }
    current_calltree_elem_->info.configs_set = 0;
    current_calltree_elem_ = current_calltree_elem_->return_to_parent();
}

void rts_handler::create_location(SCOREP_LocationType location_type, std::uint32_t location_id)
{
    pc_.create_location(location_type, location_id);
}
void rts_handler::delete_location(SCOREP_LocationType location_type, std::uint32_t location_id)
{
    pc_.delete_location(location_type, location_id);
}

/** Handling user parameters
 *
 * @param user_parameter_name name of the user parameter
 * @param value value of the parameter
 * @param locationData can be used with scorep::location_get_id() and
 *        scorep::location_get_gloabl_id() to obtain the location of the call.
 *
 *
 */
void rts_handler::user_parameter(
    std::string user_parameter_name, uint64_t value, SCOREP_Location *locationData)
{
    if (!is_inside_root)
    {
        logging::warn("RTS") << "Will ignore the user parameter \"" << user_parameter_name
                             << "\" as it is not defined in the given root phase (OA Phase)";
        return;
    }

    logging::trace("RTS") << " Got additional user param uint \"" << user_parameter_name
                          << "(hash: " << user_parameter_hash_(user_parameter_name)
                          << "\": " << value;
    call_tree::add_id_node *tmp = dynamic_cast<call_tree::add_id_node *>(
        current_calltree_elem_->enter_node(user_parameter_name));
    current_calltree_elem_ = tmp->enter_node_uint(value);
    current_calltree_elem_->info.stacked_add_ids++;
    load_config();
}

/** Handling user parameters
 *
 * @param user_parameter_name name of the user parameter
 * @param value value of the parameter
 *
 */
void rts_handler::user_parameter(
    std::string user_parameter_name, int64_t value, SCOREP_Location *locationData)
{
    if (!is_inside_root)
    {
        logging::warn("RTS") << "Will ignore the user parameter \"" << user_parameter_name
                             << "\" as it is not defined in the given root phase (OA Phase)";
        return;
    }

    logging::trace("RTS") << " Got additional user param int \"" << user_parameter_name
                          << "(hash: " << user_parameter_hash_(user_parameter_name)
                          << "\": " << value;
    call_tree::add_id_node *tmp = dynamic_cast<call_tree::add_id_node *>(
        current_calltree_elem_->enter_node(user_parameter_name));
    current_calltree_elem_ = tmp->enter_node_int(value);
    current_calltree_elem_->info.stacked_add_ids++;
    load_config();
}

/** Handling user parameters
 *
 * @param user_parameter_name name of the user parameter
 * @param value value of the parameter
 * @param locationData can be used with scorep::location_get_id() and
 *        scorep::location_get_gloabl_id() to obtain the location of the call.
 *
 */
void rts_handler::user_parameter(
    std::string user_parameter_name, std::string value, SCOREP_Location *locationData)
{
    if (!is_inside_root)
    {
        logging::warn("RTS") << "Will ignore the user parameter \"" << user_parameter_name
                             << "\" as it is not defined in the given root phase (OA Phase)";
        return;
    }

    logging::trace("RTS") << " Got additional user param string \"" << user_parameter_name
                          << "(hash: " << user_parameter_hash_(user_parameter_name)
                          << "\": " << value;
    call_tree::add_id_node *tmp = dynamic_cast<call_tree::add_id_node *>(
        current_calltree_elem_->enter_node(user_parameter_name));
    current_calltree_elem_ = tmp->enter_node_string(value);
    current_calltree_elem_->info.stacked_add_ids++;
    load_config();
}

/** parses the input identifier .json file.
 *
 */
void rts_handler::parse_input_identifier_file(const std::string &input_id_file)
{
    std::ifstream input_id_file_(input_id_file);
    if (!input_id_file_.is_open())
    {
        if (input_id_file_.fail())
        {
            logging::error("RTS") << "Could not open Input Identifer File: " << strerror(errno);
        }
        else
        {
            logging::error("RTS") << "Could not open Input Identifer File";
        }
    }
    json input_id;
    input_id_file_ >> input_id;
    for (json::iterator it = input_id.begin(); it != input_id.end(); ++it)
    {
        if (it.key() == "collect_num_threads")
        {
            if (it.value().is_boolean() && it.value().get<bool>())
            {
                try
                {
                    auto omp_get_max_threads =
                        nitro::dl::dl(nitro::dl::self).load<int(void)>("omp_get_max_threads");
                    auto num_threads = omp_get_max_threads();
                    input_identifiers_["threads"] = std::to_string(num_threads);
                    logging::debug("RTS") << "Added Input Identifier \"threads\" with value \""
                                          << num_threads << "\"";
                }
                catch (nitro::dl::exception &e)
                {
                    logging::debug("RTS")
                        << "could not find \"omp_get_max_threads\". Reason:" << e.what();
                }
            }
            continue;
        }
        if (it.key() == "collect_num_processes")
        {
            if (it.value().is_boolean() && it.value().get<bool>())
            {
                int size = scorep::call::ipc_get_size();
                input_identifiers_["processes"] = std::to_string(size);
                logging::debug("RTS")
                    << "Added Input Identifier \"processes\" with value \"" << size << "\"";
            }
            continue;
        }
        if (it.value().is_string())
        {
            input_identifiers_[it.key()] = it.value().get<std::string>();
            logging::debug("RTS") << "Added Input Identifier \"" << it.key() << "\" with value \""
                                  << it.value().get<std::string>() << "\"";
        }
        else
        {
            logging::warn("RTS") << "Canno't parse key \"" << it.key()
                                 << "\". Value is not a string";
        }
    }
}
}

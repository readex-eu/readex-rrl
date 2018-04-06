/**@file This file holds the implementation of the rrl::pcp_handler
 *
 */

#include <rrl/pcp_handler.hpp>
#include <util/log.hpp>

#include <dlfcn.h>

namespace rrl
{

/**loads the in plugin_filename specified plugin and initialize it
 *
 * The constructor initializes the plugin specified with plugin_filename.
 * After loading rrl_tuning_plugin_<plugin_name>_get_info is called the
 * information are internaly saved.
 *
 * The function checks for empty definitions of
 * rrl_tuning_plugin_info.initialize and rrl_tuning_plugin_info.finalize.
 * Thereafter it initializes the plugin.
 *
 * This function throws an error of type pcp_error if something went wrong.
 * dlclose is called before the error is thrown.
 *
 * @param plugin_filename where the plugin is to find
 * @param plugin_name name of the plugin for error messages and
 *          rrl_tuning_plugin_<plugin_name>_get_info
 *
 */
pcp_handler::pcp_handler(std::string plugin_filename, std::string plugin_name)
    : plugin_filename_(plugin_filename), plugin_name_(plugin_name)
{
    if (plugin_filename == "")
    {
        logging::debug() << "[PCP HANDLER] empty plugin filename. Don't do anything";
        return;
    }

    this->dlfcn_handle_ = dlopen(plugin_filename.c_str(), RTLD_NOW);
    if (this->dlfcn_handle_ == NULL)
    {
        throw exception::pcp_error(std::string(dlerror()), plugin_name);
    }

    std::string get_info_name("");
    get_info_name += "rrl_tuning_plugin_";
    get_info_name += plugin_name;
    get_info_name += "_get_info";

    dlerror(); // empty dlerror

    void *tmp_get_info = dlsym(this->dlfcn_handle_, get_info_name.c_str());

    /*
     *  ok, we got a temporary reference. Now check for errors.
     */
    if (tmp_get_info == NULL)
    {
        std::string error_msg(dlerror());
        dlclose(this->dlfcn_handle_);
        this->dlfcn_handle_ = NULL;
        throw exception::pcp_error(error_msg, plugin_name);
    }

    /*
     * ok now we are sure wo do not have any errors. If there is not a
     * rrl_tuning_plugin_info returned, we can't do anything more.
     *
     * So now lets get the function cast is and call it.
     */
    pcp_info_ = reinterpret_cast<rrl_tuning_plugin_info (*)()>(tmp_get_info)();

    /*
     * now check if the interface is the right one, and check if the necessary
     * functions are present.
     */
    if (pcp_info_.plugin_version > RRL_TUNING_PLUGIN_VERSION)
    {
        dlclose(this->dlfcn_handle_);
        this->dlfcn_handle_ = NULL;
        std::string error_message("Incompatible version of parameter control"
                                  "plugin detected.");
        error_message += "Version of plugin: ";
        error_message += std::to_string(this->pcp_info_.plugin_version);
        error_message += ", supported up to version: ";
        error_message += std::to_string(RRL_TUNING_PLUGIN_VERSION);
        throw exception::pcp_error(error_message, plugin_name);
    }

    if (pcp_info_.initialize == NULL)
    {
        dlclose(this->dlfcn_handle_);
        this->dlfcn_handle_ = NULL;
        throw exception::pcp_error("initialize not implemented", plugin_name);
    }

    if (pcp_info_.finalize == NULL)
    {
        dlclose(this->dlfcn_handle_);
        this->dlfcn_handle_ = NULL;
        throw exception::pcp_error("finalize not implemented", plugin_name);
    }

    /*
     * Ok, we got everything. Plugin is successfully loaded :-)
     * now lets initialize it
     */

    int ret = pcp_info_.initialize();
    if (ret != 0)
    {
        throw exception::pcp_error(
            std::string("error in initialize: ") + std::to_string(ret), plugin_name_);
    }

    rrl_tuning_action_info *tmp_action_infos = this->pcp_info_.get_tuning_info();
    int i = 0;
    while (tmp_action_infos[i].name != nullptr)
    {
        this->pcp_action_info.push_back(tmp_action_infos[i]);
        i++;
    }

    logging::debug() << "[PCP HANDLER] [" << plugin_name_ << "] got " << pcp_action_info.size()
                     << " tuning actions";
    logging::debug() << "[PCP HANDLER] [" << plugin_name_ << "] loaded";
}

/** move constructor
 *
 *  ensures that the ownership of dlfcn_handle_ stays consistent
 */
pcp_handler::pcp_handler(pcp_handler &&other)
{
    this->dlfcn_handle_ = other.dlfcn_handle_;
    this->pcp_info_ = other.pcp_info_;
    this->plugin_name_ = other.plugin_name_;
    this->plugin_filename_ = other.plugin_filename_;
    this->pcp_action_info = std::move(other.pcp_action_info);
    other.dlfcn_handle_ = nullptr;
    other.plugin_name_ = "";
    other.plugin_filename_ = "";
}

/** Destructor. Finalize the plugin if something is loaded.
 *
 */
pcp_handler::~pcp_handler()
{
    if (this->dlfcn_handle_ != nullptr)
    {
        this->pcp_info_.finalize();

        if (dlclose(this->dlfcn_handle_) != 0)
        {
            logging::warn() << "[PCP HANDLER] [" << plugin_name_
                            << "] dl_close failed: " << dlerror();
        }
        logging::info() << "[PCP HANDLER] [" << plugin_name_ << "] closed";
    }
}

/** passed the information about new locations to the plugin.
 *
 */
void pcp_handler::create_location(RRL_LocationType location_type, uint32_t location_id)
{
    this->pcp_info_.create_location(location_type, location_id);
}

/** passed the information about deleted to the plugin
 *
 */
void pcp_handler::delete_location(RRL_LocationType location_type, uint32_t location_id)
{
    this->pcp_info_.delete_location(location_type, location_id);
}

/** move constructor
 *
 *  ensures that the ownership of dlfcn_handle_ stays consistent
 */
pcp_handler &pcp_handler::operator=(pcp_handler &&other)
{
    this->dlfcn_handle_ = other.dlfcn_handle_;
    this->pcp_info_ = other.pcp_info_;
    this->plugin_name_ = other.plugin_name_;
    this->plugin_filename_ = other.plugin_filename_;
    this->pcp_action_info = std::move(other.pcp_action_info);
    other.dlfcn_handle_ = nullptr;
    other.plugin_name_ = "";
    other.plugin_filename_ = "";
    return *this;
}
}

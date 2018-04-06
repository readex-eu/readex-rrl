#ifndef TUD_RRL_INCLUDE_RRL_PCP_HANDLER_HPP_
#define TUD_RRL_INCLUDE_RRL_PCP_HANDLER_HPP_

#include <stdexcept>
#include <string>
#include <vector>

#include <scorep/rrl_tuning_plugins.h>
#include <util/log.hpp>

namespace rrl
{

namespace exception
{
/** pcp_error is thrown if soemthing wents wrong while loading a plugin
 *
 */
class pcp_error : public std::runtime_error
{
public:
    /** pcp_error is thrown if soemthing wents wrong while loading a plugin
     *
     * @param what_arg contains a string about what heppend
     * @param pcp_name contains the name of the plugin that caused the error
     */
    pcp_error(const std::string &what_arg, const std::string &pcp_name)
        : std::runtime_error(
              std::string("[PCP HANDLER] [") + pcp_name + std::string("] ") + what_arg)
    {
    }
};
}

/** This class maintains the pcp plugins, cares about initialization and
 * finalization.
 *
 * This class provides an abstraction to the pcp plugins. It loads the plugins
 * using dlopen and provides the different parameters that can be controlled
 * through the @ref pcp_action_info variable.
 */
class pcp_handler
{
public:
    pcp_handler() = delete;

    /** copy constructor
     *
     * std::map needs a copy constructor. Therfore initalise the plugin
     * again if this is called.
     */
    pcp_handler(const pcp_handler &other)
        : pcp_handler(other.plugin_filename_, other.plugin_name_){};

    pcp_handler(pcp_handler &&);

    pcp_handler(std::string plugin_filename, std::string plugin_name);
    ~pcp_handler();

    void create_location(RRL_LocationType location_type, uint32_t location_id);
    void delete_location(RRL_LocationType location_type, uint32_t location_id);

    /** Disable copy constructor.
     *
     * Importand for ownership of dlfnc_handle
     */
    pcp_handler &operator=(const pcp_handler &) = delete;

    pcp_handler &operator=(pcp_handler &&);

    /** This Variable holds the parameter info.
     *
     * Each element of the Vector provide acces to an element of the type
     * @ref rrl_tuning_action_info
     */
    std::vector<rrl_tuning_action_info> pcp_action_info;

private:
    rrl_tuning_plugin_info pcp_info_; /**< infos from get_info */
    void *dlfcn_handle_ = NULL;       /**< Handle (should be closed when finalize) */
    std::string plugin_filename_ =
        "";                        /**< filename of the plugin (usually lib\<#plugin_name_\>.so)*/
    std::string plugin_name_ = ""; /**< name of the plugin */
};
}

#endif /* TUD_RRL_INCLUDE_RRL_PCP_HANDLER_HPP_ */

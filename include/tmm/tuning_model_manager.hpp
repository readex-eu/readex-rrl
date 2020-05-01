#ifndef INCLUDE_RRL_TUNING_MODEL_MANAGER_HPP_
#define INCLUDE_RRL_TUNING_MODEL_MANAGER_HPP_

#include <tmm/callpath.hpp>
#include <tmm/parameter_tuple.hpp>
#include <tmm/region.hpp>
#include <tmm/simple_callpath.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace rrl
{
/** This namespace holds all elements that are related to the TMM.
 *
 */
namespace tmm
{
/**
 * This Enumerator holds region status
 *
 * @brief This Enumerator holds the status of the region in the tuning model.
 * A region is either significant, insignificant or unknown
 * (when not in the tuning model)
 **/
enum region_status
{
    significant,
    insignificant,
    unknown
};

/**
 * This Enumerator holds the different kinds of calibration
 *
 * @brief This Enumerator holds the different kinds of calibration.
 * none means no calibration is done.
 * cal_all supposes to invoke \ref cal_collect_all
 * cal_fix supposes to invoke \ref cal_collect_fix
 * cal_dummy supposes to invoke \ref cal_dummy
 **/
enum calibration_type
{
    none,
    cal_all,
    cal_fix,
    cal_scaling,
    cal_scaling_ref,
    cal_neural_net,
    cal_qlearn,
    q_learning_v2,
    cal_dummy
};

using phases_in_cluster_t = std::set<unsigned int>; /**< phases_in_cluster_t Is a set containing all
                                                       the phases belonging to a
                                                       particular cluster, as determined by the
                                                       DTA*/
using phase_identifier_range_t = std::unordered_map<std::string,
    std::pair<double, double>>; /**< phase_identifier_range_t Map containing the features that were
                                   used to cluster phases during DTA and their range(min, max)*/
using phase_data_t = std::pair<phases_in_cluster_t,
    phase_identifier_range_t>; /**< The TMM returns the phase_data_t inter-phase information for
                                  runtime cluster prediction */

/**The tuning_model_manager base class manages reads and writes
 * to the tuning model.
 *
 */
class tuning_model_manager
{
public:
    /** Creates and initializes an empty tuning model manager.
     *
     **/
    inline constexpr tuning_model_manager() noexcept
    {
    }

    /** Deletes the tuning model manger.
     *
     **/
    ~tuning_model_manager(){};

    /** This function registers a region on its first occurrence.
     *
     * @brief This function registers a region on its first occurrence.
     * It gets the name of the function as well as the Score-P Region ID.
     * For further communication with the tuning model manager just the
     * Score-P region ID is used.
     *
     * @param region_name name of the region as std::string
     * @param line_number line number of the region beginning
     * qparam file_name name of the dile where the region is specified
     * @param region_id Score-P region identifier
     **/
    virtual void register_region(const std::string &region_name,
        std::uint32_t line_number,
        const std::string &file_name,
        std::uint32_t region_id) = 0;

    /**
     * This functions checks if the region is a significant one.
     *
     * @brief This function checks if a region is a significant region.
     * It gets the Score-P region ID and returns the status of the region.
     *
     * @param region_id Score-P region identifier
     * @return status of the region as rrl::tmm::region_status
     **/
    virtual region_status is_significant(std::uint32_t region_id) = 0;

    /**
     * This function returns the amount of additional identifiers to expect in a certain region.
     *
     * @brief This function returns the amount of additional identifiers that need to be collected
     * in the current region, before a call to get_current_rts_configuration can be done.
     *
     * @param region_id Score-P region identifier
     * @return amount of additional identifiers to be collected.
     */
    virtual size_t get_region_identifiers(std::uint32_t region_id) = 0;

    /**
     * This function stores the configuration for each region.
     *
     * @brief This function stores the configuration for an rts.
     *
     * For certain region there might be multible parameters, which
     * might arrive at once or by seperate calls to this function.
     * The TMM needs to accumulate these calls, and make sure that
     * there is just one configuration for each parameter retruned by
     * get_current_rts_configuration().
     *
     * @param callpath callpath consiting of callpath_element()
     * @param configuration Configuration for the region is a vector of
     *        parameter tuples @ref parameter_tuple.
     * @param exectime predetermined execution time of the rts.
     *
     **/
    virtual void store_configuration(const std::vector<simple_callpath_element> &callpath,
        const std::vector<parameter_tuple> &configuration,
        std::chrono::milliseconds exectime) = 0;

    /**
     * This function returns the configuration for the rts.
     *
     * @brief This function returns the configuration for the rts.
     * It gets the Score-P region ID, rts call path consisting of a
     * vector of Score-P region ID's and additional identifiers.
     *
     * The identifiers are groped into an @ref identifier's struct,
     * and are part of the @ref simple_callpath_element
     *
     * If there are no configuration stored for the given @ref simple_callpath_element
     * an empty vector is returned.
     *
     * @param callpath out of simple_callpath_element() that hollds region_id's
     *        and additional identifiers
     * @param inputinput_identifers input identifers, gernerated during application startup. They
     *        describe the input values of the application.
     *
     *
     * @return configuration for the rts. It is a vector of parameter tuples
     *      @ref parameter_tuple
     **/
    virtual const std::vector<parameter_tuple> get_current_rts_configuration(
        const std::vector<simple_callpath_element> &callpath,
        const std::unordered_map<std::string, std::string> &input_identifers) = 0;

    /** This function clears the tuning model and remove all content of the TMM.
     *
     * It is prefered to call this function instead of overwriting the
     * different settings  in the TMM.
     */
    virtual void clear_configurations() noexcept = 0;

    /**
     * This function returns true if the given callpath element is a root element.
     *
     * This function is introduced as a workaround for PTF and the OA interface.
     * It will return true if the current callpath element is the root element. For normal
     * programs this will be the main() function. However, this function allows other root
     * elements. Every call to the TMM is supposed to be raltiv to this root element.
     *
     * @param cpe a simple callpath element
     *
     * @return true if cpe is the root element, false otherwise.
     *
     */
    virtual bool is_root(const simple_callpath_element &cpe) const noexcept = 0;

    /** This function returns the calibration type.
     *
     * The calibration type can be encoded in an environment variable or the tuning model.
     * This depends on the implementation of the tuning model mamanger.
     *
     * @return the type of the calibration as \ref calibration_type
     *
     */
    virtual calibration_type get_calibration_type() noexcept = 0;

    /** Returns the previously determined execution time for an rts.
     *
     *  @param Callpath identifying an rts.
     *
     *  @return Rts execution time.
     */
    virtual std::chrono::milliseconds get_exectime(
        const std::vector<simple_callpath_element> &callpath) noexcept = 0;

    /** Retuns data for interphase identification.
     *
     * @return data saved in the TM for interphase dynmaism for each RTS described by
     * std::vector<callpath_element>
     */
    virtual std::unordered_map<int, phase_data_t> get_phase_data() noexcept = 0;

    /** indicates if the Tuning Model has changes since the last call to has_changed(). This will
     * change the interal state of the TM. Use set_changed() and hand the return value of
     * has_changed to reset the state to the previous one.
     */
    virtual bool has_changed() noexcept = 0;

    virtual void set_changed(bool) noexcept = 0;

    /**
     * Gets the region name for a region ID.
     *
     * @param region_id The region ID
     * @return Region Name for given ID
     */
    virtual std::string get_name_from_region_id(const std::uint32_t region_id) noexcept = 0;

    /**
     * Gets the region ID for a region name.
     * @param region_name The region name
     * @return Region ID for the given name
     */
    virtual std::uint32_t get_id_from_region_name(const std::string region_name) noexcept = 0;
};

/** This function returns an instance of one of the tuning_model_manager
 * implementations
 */
std::shared_ptr<tuning_model_manager> get_tuning_model_manager(std::string tuning_model_file_path);
} // namespace tmm
} // namespace rrl
#endif /* INCLUDE_RRL_TUNING_MODEL_MANAGER_HPP_ */

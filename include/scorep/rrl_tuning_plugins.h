/*
 * This file is part of the Score-P software (http://www.score-p.org)
 *
 * Copyright (c) 2015,
 *    Technische Universitaet Muenchen, Germany
 *
 * This software may be modified and distributed under the terms of
 * a BSD-style license. See the COPYING file in the package base
 * directory for details.
 *
 */

/**
 * @file       rrl_tuning_plugins.h
 *
 * @brief Tuning plugin definitions.
 */

#ifndef RRL_TUNING_PLUGINS_H_
#define RRL_TUNING_PLUGINS_H_

/**
 * The developer of a tuning plugin should provide a README file which
 * explains how to compile, install and use the plugin. In particular,
 * all supported tuning actions should be described in the README file.
 *
 * Each tuning plugin has to include <tt>rrl_tuning_plugin.h</tt>
 * and implement 'get_info' function. Therefore, use the
 * RRL_TUNING_PLUGIN_ENTRY macro and provide the name of the plugin
 * library as the argument. For example, the tuning plugin libexample.so
 * should use RRL_TUNING_PLUGIN_ENTRY( example ). The 'get_info'
 * function returns a rrl_tuning_plugin_info data structure which
 * contains information about the plugin and its tuning actions.
 *
 * Mandatory functions
 *
 * @ initialize
 * This function is called once per process event. It registers tuning
 * actions. If all requirements are met, data structures used by the
 * plugin can be initialized within this function.
 *
 * @ finalize
 * This function is called once per process event. It unregisters tuning
 * actions and internal infrastructure of a tuning plugin (e.g., free allocated
 * resources).
 *
 * @ plugin_version
 * Should be set to RRL_TUNING_PLUGIN_VERSION
 *
 */

#include <stdint.h>

/** Current version of Score-P tuning plugin interface */
#define RRL_TUNING_PLUGIN_VERSION 0

#ifdef __cplusplus
extern "C" {
#endif

/** Language of the functions registered.
 *
 * Determines whether the language passes parameters by-value or by-reference.
 *
 */
typedef enum SCOREP_Language { SCOREP_LANGUAGE_C, SCOREP_LANGUAGE_FORTRAN } SCOREP_Language;
typedef enum RRL_LocationType {
    RRL_LOCATION_TYPE_CPU_THREAD,
    RRL_LOCATION_TYPE_GPU
} RRL_LocationType;

/** Data about tuning parameter.
 */
typedef struct rrl_tuning_action_info
{
    char *name;                           /**< Tuning action name */
    int (*current_config)();              /**< get current configuration*/
    int (*enter_region_set_config)(int); /**< Function called at enter region event*/
    int (*exit_region_set_config)(int);  /**< Function called at exit region event*/
} rrl_tuning_action_info;

/**
 * Information describing the tuning plugin.
 *
 */
typedef struct rrl_tuning_plugin_info
{
    /*
     * For all plugins
     */

    /** Should be set to SCOREP_PLUGIN_VERSION
     *  (needed for back- and forward compatibility)
     */
    uint32_t plugin_version;

    /** This functions is called once per process to initialize
     *  tuning plugin. It should return 0 if successful.
     *
     *  @return 0 if successful.
     */
    int32_t (*initialize)(void);

    /** This functions is called once per process to finalize
     *  tuning plugin.
     */
    void (*finalize)(void);

    /** When a new location is created by scorep, this information is passed to the plugin, if it is
     * a GPU or a CPU location
     *
     * It is ensured, that only one thread at a time calles this function.
     */
    void (*create_location)(RRL_LocationType location_type, uint32_t location_id);

    /** When a location is deleted by scorep, this information is passed to the plugin, if it is
     * a GPU or a CPU location
     *
     * It is ensured, that only one thread at a time calles this function.
     */
    void (*delete_location)(RRL_LocationType location_type, uint32_t location_id);

    /** This datastructure returns data about tuning actions and is null
     * terminated.
     *
     *  @return Meta data (called properties) about this metric plugin.
     */
    rrl_tuning_action_info *(*get_tuning_info)(void);

    /** Some space for future stuff, should be zeroed */
    uint64_t reserved[100];
} rrl_tuning_plugin_info;

/** Macro used for implementation of the 'get_info' function */
#define RRL_TUNING_PLUGIN_ENTRY(_name)                                                             \
    rrl_tuning_plugin_info rrl_tuning_plugin_##_name##_get_info(void)

#ifdef __cplusplus
}
#endif

#endif /* RRL_TUNING_PLUGINS_H_ */

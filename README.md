# READEX Runtime Library

The READEX Runtiem Library is written as part of the READEX project
([www.readex.eu](http://www.readex.eu))


## Compilation and Installation

### Prerequisites

To compile this plugin, you need:

* C++14 compiler

* Score-P with `SCOREP_SUBSTRATES_PLUGIN` support

### Building and installation

1. Invoke CMake

    mkdir BUILD && cd BUILD  
    cmake ..

   The following settings are important:
   
* `SCOREP_CONFIG` path to the scorep-config tool including the file name
* `CMAKE_INSTALL_PREFIX` directory where the resulting plugin will be installed (lib/ suffix will be added)
* `MIN_LOG_LEVEL` log level of the RRL. Values are: trace, debug, warn, error, fatal
* `DISABLE_CALIBRATION` default `OFF`, disables cal, and removes dependencies to protobuf and `x86_adapt`
* `EXTERN_TENSORFLOW` Default: `OFF`, setting `ON` tries to find a local copy of the c api from tensorflow instead of downloading the needed version
* `EXTERN_PROTOBUF` Default: `OFF`, setting `ON` tries to find a local copy of google protobuf instead of downloading the needed version
  
2. Invoke make

        make

> *Note:*

> If you have `scorep-config` in your `PATH`, it should be found by CMake.

3. Invoke make install

        make install

> *Note:*

> Make sure to add the subfolder `lib` to your `LD_LIBRARY_PATH`.

### Documentation

To build the documentation please do:

    make doc
    
You'll need doxygen installed.

## Usage

To use the RRL you have to add `rrl` to the environment
variable `SCOREP_SUBSTRATE_PLUGINS`.


### Environment variables

* `SCOREP_RRL_VERBOSE` 
    Controls the output verbosity of the plugin. Possible values are:
    `VERBOSE`, `WARN` (default), `INFO`, `DEBUG`, `TRACE`
    If set to any other value, WARN is used. Case in-sensitive.

* `SCOREP_RRL_CHECK_IF_RESET`
    Sets the behaviour of the settings stack of the configuration manager:
    Possible values are:
    The default value is 'reset': Every change will be saved on the settings stack.
    'no_reset' means that only the default and current values of parameters will be saved.
    New parameter values overwrites the current values.
    
* `SCOREP_TUNING_PLUGINS`, `SCOREP_RRL_PLUGINS`
    Sets the parameter plugins to load. Please be sure the path to the libs is
    in your LD_LIBRARY_PATH.
    
* `SCOREP_METRIC_PLUGINS`
	Sets the metric plugin to load. Its value should be set to 'scorep_substrate_rrl' 
	for enabling the visualization of configuration switching in trace.
	
* `SCOREP_METRIC_SCOREP_SUBSTRATE_RRL`
	Sets the metrics to be added to the trace for visualization. Value equal to the wildcard '*' 
	makes all the loaded Hw/Sw tuning parameters available in trace.  
	Application Tuning Parameter (ATP) need to be explicitly specified.	
	To load ATPS, value should be set equal to 'ATP/<atp_name>' 
	where, atp_name is the name of the ATP. The prefix 'ATP/' is required to recognize the ATP.
	For the hardware and software tuning parameters, names of the PCPs are used.
	For example: SCOREP_METRIC_SCOREP_SUBSTRATE_RRL = 'OpenMPTP, cpu_freq'
	
* `SCOREP_RRL_SIGNIFICANT_DURATION_MS` defines the duration of a significant region in milliseconds (integer required). Default 100 ms.
    RTS which have a predefind dration below this threshold will not get a new configuration.    

* `SCOREP_RRL_COUNTER` file which holds the counters to colect for calibration

* `SCOREP_RRL_COUNTER_RESULT` filename of the resulting protobuf. Will be written in the Score-P exp. dir under "rrl/"

* `SCOREP_RRL_CAL_MODULE` calibration module to use. Currently available:
 * `collect_all` for the first training step, to build the correlation between all counters
 * `collect_fix` for the second training step, to collect the training data of the NN
 * `collect_scaling` for trainign to find optimal configuration for regions.
 * `cal_dummy` dummy calibration mechanism. Just returns 2.501 GHz core and 3 GHz uncore freq.

#### Calibration specific varibales

##### `collect_all`

##### `collect_fix`

##### `collect_scaling`

### If anything fails:

1. Check whether the plugin library can be loaded from the `LD_LIBRARY_PATH`.
2. Write a mail to the author.

## Authors

* Andreas Gocht (andreas.gocht at tu-dresden dot de)
* Umbreen Sabir Mian (umbreen.mian at tu-dresden dot de)
* Nico Reissmann (reissman at idi.ntnu.no)
* Mohammed Sourouri (mohammed.sourouri at iet.ntnu.no)


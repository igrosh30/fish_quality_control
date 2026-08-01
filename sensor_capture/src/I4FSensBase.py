import os
import time
import copy

import numpy as np

import PyZyrModbusSensors as Zsens
import PyZyrUtil as Zutil

__author__ = "Luiz Claudio Navarro"
__version__ = "1.0.0"
__date__ = "2026.07.23"
libgroup = "I4F System - Intelligent Fish Farming for Future Programs."
libname = "Sensors base functions."
libversion = "Version: " + __version__ + " Date: " + __date__ + "."
copyrightmsg = "I4F System (c) 2019-2026..."
__doc__ = libgroup + " " + libname + "\n" + libversion + \
          "\nAuthor: " + __author__ + " - " + copyrightmsg + "\n"

##############################################################################################
# License and Disclaimer: CIIMAR and FEUP and Unicamp terms
##############################################################################################

##############################################################################################
# Constants and Global variables
##############################################################################################

# sensors read parameters
C_SENS_INIT_TIMEOUT = 0.500
C_SENS_DEFAULT_N_READS = 3
C_SENS_DEFAULT_BETWEEN_READS = 0.010
C_SENS_OPER_RETRIES = 3
C_SENS_OPER_TIMEOUT = 0.100

# non (-1) number of measurements / total measurements to validate sensors data vector
C_SEQ_VALID = 0.5

default_config_dict = {
    "general": {
        "config_date": "aaaa.mm.dd",
        "tank_id": "Tank00",
        "number_of_read_cycles": 3,
        "warmup_secs": 1.0,
        "cycle_interval_secs": 20.0
    },
    "gateway": {
        "init_timeout": C_SENS_INIT_TIMEOUT,
        "reinit_on_error": False,
        "tcp-ip": {
            "host": "",
            "port": 0,
        },
        "serial": {
            "port": "",
            "baudrate": 9600,
            "bytesize": 8,
            "parity": "N",
            "stopbits": 1
        }
    },
    "sensors": {
        "name": {
            "enable": False,
            "description": "",
            "modbus_address": 0,
            "number_of_reads": C_SENS_DEFAULT_N_READS,
            "time_between_reads": C_SENS_DEFAULT_BETWEEN_READS,
            "number_of_retries": C_SENS_OPER_RETRIES,
            "command_timeout": C_SENS_OPER_TIMEOUT,
            "modbus_registers": {
                "reg_low": 0,
                "reg_high": 0,
                "reg_type": "",
                "reg_decp": 0,
                "reg_fact": 0.0,
                "reg_unit": ""
            }
        }
    }
}

# Sensors log files lay-out
hdr_sens = ["tank id", "date", "time", "time secs",
            "do_sens_do", "unit", "nonzero", "nreads", "time",
            "do_sens_sat", "unit", "nonzero", "nreads", "time",
            "do_sens_temp", "unit", "nonzero", "nreads", "time",
            "ph_sens_ph", "unit", "nonzero", "nreads", "time",
            "sal_sens_ec", "unit", "nonzero", "nreads", "time",
            "sal_sens_temp", "unit", "nonzero", "nreads", "time",
            "sal_sens_sal", "unit", "nonzero", "nreads", "time",
            "co2_sens_co2", "unit", "nonzero", "nreads", "time",
            "nh4_sens_nh4", "unit", "nonzero", "nreads", "time",
            "nh4_sens_dpnh4", "unit", "nonzero", "nreads", "time",
            "nh4_sens_temp", "unit", "nonzero", "nreads", "time",
            "nh4_sens_dptemp", "unit", "nonzero", "nreads", "time",
            "turb_sens_ntu", "unit", "nonzero", "nreads", "time",
            "temp", "do", "sat", "ph", "ec", "sal", "co2", "nh4", "turb"]

C_SENS_TANK = 0
C_SENS_DATE = C_SENS_TANK + 1
C_SENS_TIME = C_SENS_DATE + 1
C_SENS_RECORD_SECS = C_SENS_TIME + 1
C_SENS_DO_SENS_DO = C_SENS_RECORD_SECS + 1
C_SENS_UNIT_DO = C_SENS_DO_SENS_DO + 1
C_SENS_NONZERO_DO = C_SENS_UNIT_DO + 1
C_SENS_NREADS_DO = C_SENS_NONZERO_DO + 1
C_SENS_TIME_DO = C_SENS_NREADS_DO + 1
C_SENS_DO_SENS_SAT = C_SENS_TIME_DO + 1
C_SENS_UNIT_SAT = C_SENS_DO_SENS_SAT + 1
C_SENS_NONZERO_SAT = C_SENS_UNIT_SAT + 1
C_SENS_NREADS_SAT = C_SENS_NONZERO_SAT + 1
C_SENS_TIME_SAT = C_SENS_NREADS_SAT + 1
C_SENS_DO_SENS_TEMP = C_SENS_TIME_SAT + 1
C_SENS_UNIT_DOTEMP = C_SENS_DO_SENS_TEMP + 1
C_SENS_NONZERO_DOTEMP = C_SENS_UNIT_DOTEMP + 1
C_SENS_NREADS_DOTEMP = C_SENS_NONZERO_DOTEMP + 1
C_SENS_TIME_DOTEMP = C_SENS_NREADS_DOTEMP + 1
C_SENS_PH_SENS_PH = C_SENS_TIME_DOTEMP + 1
C_SENS_UNIT_PH = C_SENS_PH_SENS_PH + 1
C_SENS_NONZERO_PH = C_SENS_UNIT_PH + 1
C_SENS_NREADS_PH = C_SENS_NONZERO_PH + 1
C_SENS_TIME_PH = C_SENS_NREADS_PH + 1
C_SENS_SAL_SENS_EC = C_SENS_TIME_PH + 1
C_SENS_UNIT_EC = C_SENS_SAL_SENS_EC + 1
C_SENS_NONZERO_EC = C_SENS_UNIT_EC + 1
C_SENS_NREADS_EC = C_SENS_NONZERO_EC + 1
C_SENS_TIME_EC = C_SENS_NREADS_EC + 1
C_SENS_SAL_SENS_TEMP = C_SENS_TIME_EC + 1
C_SENS_UNIT_SALTEMP = C_SENS_SAL_SENS_TEMP + 1
C_SENS_NONZERO_SALTEMP = C_SENS_UNIT_SALTEMP + 1
C_SENS_NREADS_SALTEMP = C_SENS_NONZERO_SALTEMP + 1
C_SENS_TIME_SALTEMP = C_SENS_NREADS_SALTEMP + 1
C_SENS_SAL_SENS_SAL = C_SENS_TIME_SALTEMP + 1
C_SENS_UNIT_SAL = C_SENS_SAL_SENS_SAL + 1
C_SENS_NONZERO_SAL = C_SENS_UNIT_SAL + 1
C_SENS_NREADS_SAL = C_SENS_NONZERO_SAL + 1
C_SENS_TIME_SAL = C_SENS_NREADS_SAL + 1
C_SENS_CO2_SENS_CO2 = C_SENS_TIME_SAL + 1
C_SENS_UNIT_CO2 = C_SENS_CO2_SENS_CO2 + 1
C_SENS_NONZERO_CO2 = C_SENS_UNIT_CO2 + 1
C_SENS_NREADS_CO2 = C_SENS_NONZERO_CO2 + 1
C_SENS_TIME_CO2 = C_SENS_NREADS_CO2 + 1
C_SENS_NH4_SENS_NH4 = C_SENS_TIME_CO2 + 1
C_SENS_UNIT_NH4 = C_SENS_NH4_SENS_NH4 + 1
C_SENS_NONZERO_NH4 = C_SENS_UNIT_NH4 + 1
C_SENS_NREADS_NH4 = C_SENS_NONZERO_NH4 + 1
C_SENS_TIME_NH4 = C_SENS_NREADS_NH4 + 1
C_SENS_NH4_SENS_DPNH4 = C_SENS_TIME_NH4 + 1
C_SENS_UNIT_DPNH4 = C_SENS_NH4_SENS_DPNH4 + 1
C_SENS_NONZERO_DPNH4 = C_SENS_UNIT_DPNH4 + 1
C_SENS_NREADS_DPNH4 = C_SENS_NONZERO_DPNH4 + 1
C_SENS_TIME_DPNH4 = C_SENS_NREADS_DPNH4 + 1
C_SENS_NH4_SENS_TEMP = C_SENS_TIME_DPNH4 + 1
C_SENS_UNIT_NH4TEMP = C_SENS_NH4_SENS_TEMP + 1
C_SENS_NONZERO_NH4TEMP = C_SENS_UNIT_NH4TEMP + 1
C_SENS_NREADS_NH4TEMP = C_SENS_NONZERO_NH4TEMP + 1
C_SENS_TIME_NH4TEMP = C_SENS_NREADS_NH4TEMP + 1
C_SENS_NH4_SENS_DPTEMP = C_SENS_TIME_NH4TEMP + 1
C_SENS_UNIT_DPTEMP = C_SENS_NH4_SENS_DPTEMP + 1
C_SENS_NONZERO_DPTEMP = C_SENS_UNIT_DPTEMP + 1
C_SENS_NREADS_DPTEMP = C_SENS_NONZERO_DPTEMP + 1
C_SENS_TIME_DPTEMP = C_SENS_NREADS_DPTEMP + 1
C_SENS_TURB_SENS_NTU = C_SENS_TIME_DPTEMP + 1
C_SENS_UNIT_NTU = C_SENS_TURB_SENS_NTU + 1
C_SENS_NONZERO_NTU = C_SENS_UNIT_NTU + 1
C_SENS_NREADS_NTU = C_SENS_NONZERO_NTU + 1
C_SENS_TIME_NTU = C_SENS_NREADS_NTU + 1
C_SENS_TEMP = C_SENS_TIME_NTU + 1
C_SENS_DO = C_SENS_TEMP + 1
C_SENS_SAT = C_SENS_DO + 1
C_SENS_PH = C_SENS_SAT + 1
C_SENS_EC = C_SENS_PH + 1
C_SENS_SAL = C_SENS_EC + 1
C_SENS_CO2 = C_SENS_SAL + 1
C_SENS_NH4 = C_SENS_CO2 + 1
C_SENS_TURB = C_SENS_NH4 + 1
C_SENS_LEN = C_SENS_TURB + 1

assert len(hdr_sens) == C_SENS_LEN, "Internal error: Header and indexes mismatch!"

# ---------------------------------------------------------------------------------------------
# Saturation table
# ---------------------------------------------------------------------------------------------
do_sat_tab = np.array([[14.621, 14.12, 13.636, 13.167, 12.714, 12.277, 11.854, 11.445, 11.051],
                       [14.216, 13.733, 13.266, 12.815, 12.378, 11.956, 11.548, 11.154, 10.773],
                       [13.829, 13.364, 12.914, 12.478, 12.057, 11.65, 11.256, 10.875, 10.507],
                       [13.46, 13.011, 12.577, 12.156, 11.75, 11.356, 10.976, 10.608, 10.252],
                       [13.107, 12.674, 12.255, 11.849, 11.456, 11.076, 10.708, 10.352, 10.008],
                       [12.77, 12.352, 11.947, 11.54, 11.175, 10.807, 10.451, 10.107, 9.774],
                       [12.447, 12.043, 11.652, 11.272, 10.905, 10.55, 10.206, 9.872, 9.55],
                       [12.139, 11.748, 11.369, 11.002, 10.647, 10.303, 9.97, 9.647, 9.335],
                       [11.843, 11.465, 11.098, 10.743, 10.399, 10.066, 9.744, 9.431, 9.128],
                       [11.559, 11.194, 10.839, 10.495, 10.162, 9.839, 9.526, 9.223, 8.93],
                       [11.288, 10.933, 10.59, 10.257, 9.934, 9.621, 9.318, 9.024, 8.739],
                       [11.027, 10.684, 10.351, 10.028, 9.715, 9.412, 9.117, 8.832, 8.556],
                       [10.777, 10.444, 10.121, 9.808, 9.505, 9.21, 8.925, 8.645, 8.379],
                       [10.537, 10.214, 9.901, 9.597, 9.302, 9.017, 8.739, 8.47, 8.21],
                       [10.306, 9.993, 9.689, 9.394, 9.108, 8.83, 8.561, 8.3, 8.046],
                       [10.084, 9.78, 9.485, 9.198, 8.921, 8.651, 8.389, 8.135, 7.888],
                       [9.87, 9.575, 9.289, 9.01, 8.74, 8.478, 8.223, 7.976, 7.737],
                       [9.665, 9.378, 9.099, 8.829, 8.566, 8.311, 8, 7.823, 7.539],
                       [9.467, 9.188, 8.917, 8.654, 8.399, 8.151, 7.91, 7.676, 7.449],
                       [9.276, 9.005, 8.742, 8.486, 8.237, 7.995, 7.761, 7.533, 7.312],
                       [9.092, 8.828, 8.572, 8.323, 8.081, 7.846, 7.617, 7.395, 7.18],
                       [8.914, 8.658, 8.408, 8.166, 7.93, 7.701, 7.479, 7.262, 7.052],
                       [8.743, 8.493, 8.25, 8.014, 7.785, 7.56, 7.43, 7.13, 6.929],
                       [8.578, 8.334, 8.098, 7.867, 7.644, 7.426, 7.214, 7.009, 6.809],
                       [8.418, 8.181, 7.95, 7.725, 7.507, 7.295, 7.089, 6.888, 6.693],
                       [8.263, 8.032, 7.807, 7.588, 7.375, 7.168, 6.967, 6.771, 6.581],
                       [8.113, 7.888, 7.668, 7.455, 7.247, 7.045, 6.849, 6.658, 6.472],
                       [7.968, 7.748, 7.534, 7.326, 7.123, 6.926, 6.734, 6.548, 6.366],
                       [7.827, 7.613, 7.404, 7.201, 7.003, 6.81, 6.623, 6.441, 6.263],
                       [7.691, 7.482, 7.278, 7.079, 6.886, 6.698, 6.515, 6.337, 6.164],
                       [7.558, 7.354, 7.155, 6.961, 6.772, 6.589, 6.41, 6.236, 6.066],
                       [7.43, 7.23, 7.036, 6.846, 6.662, 6.483, 6.308, 6.137, 5.972],
                       [7.305, 7.11, 6.92, 6.735, 6.555, 6.379, 6.208, 6.042, 5.88],
                       [7.183, 6.993, 6.807, 6.626, 6.45, 6.278, 6.111, 5.948, 5.79],
                       [7.065, 6.879, 6.697, 6.52, 6.348, 6.18, 6.017, 5.857, 5.702],
                       [6.949, 6.767, 6.59, 6.417, 6.248, 6.084, 5.924, 5.768, 5.617],
                       [6.837, 6.659, 6.485, 6.316, 6.151, 5.991, 5.834, 5.681, 5.533],
                       [6.727, 6.553, 6.383, 6.218, 6.056, 5.899, 5.746, 5.597, 5.451],
                       [6.619, 6.449, 6.283, 6.121, 5.963, 5.81, 5.66, 5.513, 5.371],
                       [6.514, 6.348, 6.186, 6.027, 5.87, 5.722, 5.575, 5.432, 5.292],
                       [6.412, 6.249, 6.09, 5.935, 5.78, 5.636, 5.492, 5.352, 5.215]])
do_sat_sal = np.array([0, 5, 10, 15, 20, 25, 30, 35, 40])
do_sat_temp = np.arange(41)
assert do_sat_tab.shape == (len(do_sat_temp), len(do_sat_sal)), \
    "DO saturation table mismatch!"


##############################################################################################
# Utility and definition functions
##############################################################################################
def consist_sensors_config(config_dict):
    assert "general" in config_dict.keys() and \
           "gateway" in config_dict.keys() and \
           "sensors" in config_dict.keys(), \
        "Missing mandatory keys in configuration!"

    defdict = default_config_dict["general"]
    cfgdict = config_dict["general"]
    assert "config_date" in cfgdict.keys() and \
           "tank_id" in cfgdict.keys() and \
           "number_of_read_cycles" in cfgdict.keys() and \
           "warmup_secs" in cfgdict.keys() and \
           "cycle_interval_secs" in cfgdict.keys(), "Missing general mandatory keys!"
    Zutil.set_config_default_and_type(defdict, cfgdict)
    tankid = cfgdict["tank_id"]
    assert tankid.startswith("Tank"), "Tank ID mismatch!"

    defdict = default_config_dict["gateway"]
    cfgdict = config_dict["gateway"]
    tcpser = False
    for key in defdict.keys():
        if key in cfgdict.keys():
            if key == "tcp-ip" or key == "serial":
                assert not tcpser, "Duplicate or TCP and Serial parameters already defined!"
                tcpser = True
                defmoddict = defdict[key]
                cfgmoddict = cfgdict[key]
                if key == "tcp-ip":
                    assert "host" in cfgmoddict.keys() and \
                           "port" in cfgmoddict.keys(), \
                        "Missing mandatory TCP/IP Modbus gateway parameters!"
                elif key == "serial":
                    assert "port" in cfgmoddict.keys() and \
                           "baudrate" in cfgmoddict.keys() and \
                           "bytesize" in cfgmoddict.keys() and \
                           "parity" in cfgmoddict.keys() and \
                           "stopbits" in cfgmoddict.keys(), \
                        "Missing mandatory Serial Modbus gateway parameters!"
                else:
                    assert False, "Invalid Modbus gateway parameter!"
                Zutil.set_config_default_and_type(defmoddict, cfgmoddict)
            else:
                if type(defdict[key]) is bool:
                    cfgdict[key] = bool(cfgdict[key])
                    assert type(defdict[key]) == type(cfgdict[key]), \
                        "Configuration parameter {:s} wrong type!".format(cfgdict[key])
        else:
            if key != "tcp-ip" and key != "serial":
                cfgdict[key] = defdict[key]

    # noinspection PyTypeChecker
    defdict = dict(default_config_dict["sensors"]["name"])
    for sensor in config_dict["sensors"].keys():
        cfgdict = config_dict["sensors"][sensor]
        assert "enable" in cfgdict.keys() and \
               "modbus_address" in cfgdict.keys() and \
               "modbus_registers" in cfgdict.keys(), "Missing Modbus mandatory keys!"
        for key in defdict.keys():
            if key in cfgdict.keys():
                if key == "modbus_registers":
                    modbdef = defdict[key]
                    parmscfg = cfgdict[key]
                    for parm in parmscfg.keys():
                        modbcfg = parmscfg[parm]
                        for subkey in modbdef.keys():
                            assert subkey in modbcfg.keys(), \
                                "Missing Modbus mandatory {:s} key for parameter {:s}!".format(subkey, parm)
                            assert type(modbdef[subkey]) == type(modbcfg[subkey]), \
                                "Modbus parameter {:s} key {:s} value {:s} wrong type!". \
                                    format(parm, subkey, str(modbdef[subkey]))
                elif type(defdict[key]) is bool:
                    cfgdict[key] = bool(cfgdict[key])
                else:
                    assert type(defdict[key]) == type(cfgdict[key]), \
                        "Configuration parameter {:s} wrong type!".format(cfgdict[key])
            else:
                cfgdict[key] = defdict[key]

    return config_dict, tankid


def read_sensors_config(cfgfname):
    assert os.path.isfile(cfgfname) and cfgfname.endswith(".json"), \
        "Sensors configuration JSON file {:s} not found!".format(os.path.basename(cfgfname))
    cfgdict, _ = Zutil.json_load_unziped(cfgfname, "Load sensors configurations")
    cfg_dict, tankid = consist_sensors_config(cfgdict)
    return cfgdict, tankid


def write_sensors_raw_data(sensfname, measures, addflag=True):
    assert sensfname.endswith(".csv"), "Invalid sensors log file name!"
    assert len(measures[0]) == C_SENS_LEN, "Invalid sensors measures to log!"
    sensdir = os.path.dirname(sensfname)
    assert os.path.isdir(sensdir), "Sensors log folder not found!"
    Zutil.write_csv_file(sensfname, hdr_sens, measures, title="sensors data", addflag=addflag)
    return


def failed_resul(n_fields):
    return [-1.0] * (n_fields * Zsens.C_SENS_RESULT_LEN)


def init_record_time(tankid):
    timesec = time.time()
    dt_tm = Zutil.secs_to_string(timesec).split("-")
    measure = [tankid, int(dt_tm[0]), int(dt_tm[1]), int(timesec)]
    return measure


def null_record(tankid):
    measure = init_record_time(tankid)
    measure += [-1.0] * (C_SENS_LEN - len(measure))
    return measure


def sensors_record_from_dict(tankid, sensresul):
    measure = init_record_time(tankid)

    # get sensors' measurements from the result dictionary
    if len(sensresul) > 0:
        if "Dissolved_O2" in sensresul.keys() and \
                len(sensresul["Dissolved_O2"]) >= 3:
            dodict = sensresul["Dissolved_O2"]
            if "DO" in dodict.keys() and \
                    "saturation" in dodict.keys() and \
                    "temperature" in dodict.keys():
                measure += dodict["DO"]
                measure += dodict["saturation"]
                measure += dodict["temperature"]
            else:
                measure += failed_resul(3)
        else:
            measure += failed_resul(3)
        if "pH" in sensresul.keys() and \
                len(sensresul["pH"]) >= 1:
            phdict = sensresul["pH"]
            if "pH" in phdict.keys():
                measure += phdict["pH"]
            else:
                measure += failed_resul(1)
        else:
            measure += failed_resul(1)
        if "EC_Salinity" in sensresul.keys() and \
                len(sensresul["EC_Salinity"]) >= 5:
            saldict = sensresul["EC_Salinity"]
            if "condutivity" in saldict.keys() and \
                    "temperature" in saldict.keys() and \
                    "salinity" in saldict.keys():
                measure += saldict["condutivity"]
                measure += saldict["temperature"]
                measure += saldict["salinity"]
            else:
                measure += failed_resul(3)
        else:
            measure += failed_resul(3)
        if "Dissolved_CO2" in sensresul.keys() and \
                len(sensresul["Dissolved_CO2"]) >= 1:
            co2dict = sensresul["Dissolved_CO2"]
            if "CO2" in co2dict.keys():
                measure += co2dict["CO2"]
            else:
                measure += failed_resul(1)
        else:
            measure += failed_resul(1)
        if "Ammonia_Ion" in sensresul.keys() and \
                len(sensresul["Ammonia_Ion"]) >= 4:
            nh4dict = sensresul["Ammonia_Ion"]
            if "NH4+" in nh4dict.keys() and \
                    "decpnh4" in nh4dict.keys() and \
                    "temperature" in nh4dict.keys() and \
                    "decptemp" in nh4dict.keys():
                measure += nh4dict["NH4+"]
                measure += nh4dict["decpnh4"]
                measure += nh4dict["temperature"]
                measure += nh4dict["decptemp"]
            else:
                measure += failed_resul(4)
        else:
            measure += failed_resul(4)
        if "Turbidity" in sensresul.keys() and \
                len(sensresul["Turbidity"]) >= 1:
            tbdict = sensresul["Turbidity"]
            if "turb" in tbdict.keys():
                measure += tbdict["turb"]
            else:
                measure += failed_resul(1)
        else:
            measure += failed_resul(1)
    else:
        measure += [-1.0] * (C_SENS_LEN - 2)

    # compute summarized measures
    temp = []
    if measure[C_SENS_DO_SENS_TEMP + Zsens.C_SENS_RESULT_NONZERO] > 0:
        temp.append(float(measure[C_SENS_DO_SENS_TEMP]))
    if measure[C_SENS_SAL_SENS_TEMP + Zsens.C_SENS_RESULT_NONZERO] > 0:
        temp.append(float(measure[C_SENS_SAL_SENS_TEMP]))
    if measure[C_SENS_NH4_SENS_TEMP + Zsens.C_SENS_RESULT_NONZERO] > 0 and \
            measure[C_SENS_NH4_SENS_DPTEMP + Zsens.C_SENS_RESULT_NREADS] > 0:
        temp.append(float(measure[C_SENS_NH4_SENS_TEMP]) / (10.0 ** measure[C_SENS_NH4_SENS_DPTEMP]))
    if len(temp) > 0:
        measure.append(round(float(np.mean(np.array(temp))), 1))
    else:
        measure.append(-1.0)
    if measure[C_SENS_DO_SENS_DO] > 0.0:
        measure.append(float(measure[C_SENS_DO_SENS_DO]))
    else:
        measure.append(-1.0)
    if measure[C_SENS_DO_SENS_SAT] > 0.0:
        measure.append(float(measure[C_SENS_DO_SENS_SAT]))
    else:
        measure.append(-1.0)
    if measure[C_SENS_PH_SENS_PH] > 0.0:
        measure.append(float(measure[C_SENS_PH_SENS_PH]))
    else:
        measure.append(-1.0)
    if measure[C_SENS_SAL_SENS_EC + Zsens.C_SENS_RESULT_NREADS] > 0:
        measure.append(float(measure[C_SENS_SAL_SENS_EC]))
    else:
        measure.append(-1.0)
    if measure[C_SENS_SAL_SENS_SAL + Zsens.C_SENS_RESULT_NREADS] > 0:
        measure.append(float(measure[C_SENS_SAL_SENS_SAL] / 1000.0))
    else:
        measure.append(-1.0)
    if measure[C_SENS_CO2_SENS_CO2] > 0.0:
        measure.append(float(measure[C_SENS_CO2_SENS_CO2]))
    else:
        measure.append(-1.0)
    if measure[C_SENS_NH4_SENS_NH4] > 0.0 and \
            measure[C_SENS_NH4_SENS_DPNH4 + Zsens.C_SENS_RESULT_NREADS] > 0:
        measure.append(round(float(measure[C_SENS_NH4_SENS_NH4]) /
                             (10.0 ** measure[C_SENS_NH4_SENS_DPNH4]),
                             int(measure[C_SENS_NH4_SENS_DPNH4])))
    else:
        measure.append(-1.0)
    if measure[C_SENS_TURB_SENS_NTU] > 0.0:
        measure.append(float(measure[C_SENS_TURB_SENS_NTU]))
    else:
        measure.append(-1.0)

    if len(measure) != C_SENS_LEN:
        print("len:", C_SENS_LEN, "header:", hdr_sens)
        print("len:", len(measure), "content:", measure)
        assert False, "measures and sensors header do not match!"

    return measure


##############################################################################################
# AllSensors Class
##############################################################################################
class AllSensors:
    def __init__(self, cfgfname):

        assert os.path.isfile(cfgfname) and \
               cfgfname.endswith(".json"), \
            "Sensors configuration JSON file not found!"

        cfgdict, _ = Zutil.json_load_unziped(cfgfname, "Sensors configuration")

        assert isinstance(cfgdict, dict), "Sensors configuration object is not a dictionary!"

        print("*** Begin Initializing All Sensors ***")
        self.cfg_dict, tankid = consist_sensors_config(cfgdict)
        self.tankid = str(tankid)
        self.warmup_time = float(self.cfg_dict["general"]["warmup_secs"])
        assert 0.0 <= self.warmup_time <= 180.0, \
            "Warm up time in seconds out of range [0.0, 180.0]"
        self.nread_cycles = int(self.cfg_dict["general"]["number_of_read_cycles"])
        self.cycles_interval = float(self.cfg_dict["general"]["cycle_interval_secs"])

        cfgmodb = self.cfg_dict["gateway"]
        print("*** Initializing ModBus Gateway communication ***")
        if "tcp-ip" in cfgmodb.keys():
            parms = {"tcp-ip": cfgmodb["tcp-ip"]}
        elif "serial" in cfgmodb.keys():
            parms = {"serial": cfgmodb["serial"]}
        else:
            assert False, "Invalid Modbus gateway should be tcp-ip or serial!"
        self.modbsens = Zsens.ModbusSensor(parms,
                                           ini_tmout=cfgmodb["init_timeout"],
                                           reinit=cfgmodb["reinit_on_error"])
        if self.modbsens.is_modbus_connected():
            self.modbsens.reset_modblog()
            print("    ModBus Gateway communication initialized OK!")
        else:
            print("    ModBus Gateway communication failed!")
            self.print_modblog()
            self.modbsens = None

        # wait sensors warm_up
        if self.warmup_time > 0.0:
            print("    Waiting sensors warm-up initialization time...")
            time.sleep(self.warmup_time)

        print("*** End Initializing All Sensors ***")
        return

    def get_tankid(self):
        return self.tankid

    def get_n_read_cycles(self):
        return self.nread_cycles, self.cycles_interval

    def is_sensors_connected(self):
        return self.modbsens is not None

    def print_modblog(self):
        print("--- Modbdus Log begin ---")
        # noinspection unresolved-references
        print(self.modbsens.get_reset_modblog())
        print("--- Modbdus Log end ---")

    def read_all_sensors(self):
        if self.modbsens is None:
            return {}
        result = {}
        sensors = self.cfg_dict["sensors"]
        for sens in sensors.keys():
            if sensors[sens]["enable"]:
                print("*** Reading sensor {:s} ***".format(sens))
                sensdata = sensors[sens]
                sensresul = self.modbsens.read_registers(sens, sensdata["modbus_address"],
                                                         copy.deepcopy(sensdata["modbus_registers"]),
                                                         n_reads=sensdata["number_of_reads"],
                                                         tm_btw_rd=sensdata["time_between_reads"],
                                                         opr_tmout=sensdata["command_timeout"],
                                                         n_retries=sensdata["number_of_retries"])
                if sensresul:
                    print("    Sensor {:s} read, {:d} parameters.".format(sens, len(sensresul)))
                    result[sens] = sensresul
                    self.modbsens.reset_modblog()
                else:
                    print("    Sensor {:s} read failed!".format(sens))
                    self.print_modblog()
        return result

    def close(self):
        if self.modbsens is not None:
            self.modbsens.close_sensor()
        self.modbsens = None
        return


##############################################################################################
# main:
##############################################################################################
if __name__ == "__main__":
    print(__doc__)

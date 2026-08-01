import argparse
import os
import time

import I4FSensBase as Isens
import PyZyrUtil as Zutil

__author__ = "Luiz Claudio Navarro"
__version__ = "1.0.0"
__date__ = "2026.07.23"
libgroup = "I4F System - Intelligent Fish Farming for Future Programs."
progname = "Read Modbus sensors raw data program."
progversion = "Version: " + __version__ + " Date: " + __date__ + "."
copyrightmsg = "I4F System (c) 2019-2026..."
progfooter = progname[:-1] + " - " + progversion[:-1] + " - " + \
             __author__ + " - Powered by " + copyrightmsg
__doc__ = libgroup + "\n" + progname + "\n" + progversion + " " + copyrightmsg + "\n"


##############################################################################################
# Constants and Global variables
##############################################################################################


##############################################################################################
# Utility and definition functions
##############################################################################################


##############################################################################################
# main program
##############################################################################################
def read_sensors_raw(cfgfname, outdir):
    assert os.path.isfile(cfgfname) and \
           cfgfname.endswith(".json"), \
        "Sensors configuration JSON file not found!"

    outdir = str(os.path.abspath(outdir))
    if not os.path.isdir(outdir):
        print("Output folder does not exist.")
        os.makedirs(outdir)
        print("Then created!")

    sensobj = Isens.AllSensors(cfgfname)
    tankid = sensobj.get_tankid()
    nrcycles, cycle_time = sensobj.get_n_read_cycles()

    print("=== Starting {:d} cycles of reading sensors  ===".format(nrcycles))
    records = []
    for cycle in range(nrcycles):
        initime = time.time()
        print("--- Starting reading cycle {:d} ---".format(cycle))
        rawsens = sensobj.read_all_sensors()

        if rawsens is None or len(rawsens) <= 0:
            print("--- Reading cycle {:d} finished unsuccessfully ---".format(cycle))
            records.append(Isens.null_record(tankid))
        else:
            print("--- Successful reading cycle {:d} finished ---".format(cycle))
            records.append(Isens.sensors_record_from_dict(tankid, rawsens))

        rdtime = time.time() - initime
        print("    Reading sensors took {:.3f} secs.".format(rdtime))
        if cycle < (nrcycles - 1) and rdtime < cycle_time:
            nxtime = cycle_time - rdtime
            print("    Wait {:.3f} secs for next cycle time".format(nxtime))
            time.sleep(nxtime)

        print("")

    sensfname = str(os.path.join(outdir,
                                 "LogRawSensors_{:s}_{:s}.csv".
                                 format(tankid, time.strftime("%Y%m%d_%H%M%S"))))
    Isens.write_sensors_raw_data(sensfname, records, addflag=False)

    return


##############################################################################################
# main:
##############################################################################################
def parse_args():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("-c", "--config", required=False,
                        default="./config/sensors_config.json", help="input JSON config file")
    parser.add_argument("-o", "--outdir", required=False,
                        default="./sensors", help="output sensors folder")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    progobj = Zutil.ZyrProgHeader(__doc__)
    progobj.print_begin()

    read_sensors_raw(str(args.config), str(args.outdir))

    progobj.print_end(None, "")

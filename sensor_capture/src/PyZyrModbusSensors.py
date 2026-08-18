import copy
import ipaddress
import time
import numpy as np

from pymodbus import framer
from pymodbus.client import ModbusTcpClient, ModbusSerialClient
from pymodbus import ModbusException

__author__ = "Luiz Claudio Navarro"
__version__ = "1.0.3"
__date__ = "2026.07.23"
libgroup = "Zyryus Projects Infrastructure Libraries."
libname = "Modbus water quality sensors library"
libversion = "Version: " + __version__ + " Date: " + __date__ + "."
copyrightmsg = "Zyryus Consulting (c) 2019-2025..."
__doc__ = libgroup + " " + libname + "\n" + libversion + \
          "\nAuthor: " + __author__ + " - " + copyrightmsg + "\n"

# Sensor parameter dictionary length
C_SENS_PARM_LEN = 6

# Sensor max number of holding registers
C_SENS_MAX_INDX = 256

# Read registers controls
C_SENS_MAX_READS = 10
C_SENS_DEFAULT_N_READS = 3
C_SENS_DEFAULT_BETWEEN_READS = 0.010
C_SENS_MAX_BETWEEN_READS = 2.000

# Default number of retries on error of sensor operations
C_SENS_OPER_RETRIES = 3

# Maximum number of retries on error of sensor operations
C_SENS_MAX_RETRIES = 10

# Result parameter layout
C_SENS_RESULT_VALUE = 0
C_SENS_RESULT_UNIT = C_SENS_RESULT_VALUE + 1
C_SENS_RESULT_NONZERO = C_SENS_RESULT_UNIT + 1
C_SENS_RESULT_NREADS = C_SENS_RESULT_NONZERO + 1
C_SENS_RESULT_TIME = C_SENS_RESULT_NREADS + 1
C_SENS_RESULT_LEN = C_SENS_RESULT_TIME + 1

# Initialization default timeout and maximum number of reinitializations
C_SENS_INIT_TIMEOUT = 0.500
C_SENS_MAX_REINIT = 10

# Operation default timeout
C_SENS_OPER_TIMEOUT = 0.100


####################################################################################################
# Modbus Sensor Class
####################################################################################################
class ModbusSensor:
    def __init__(self, modparms, ini_tmout=C_SENS_INIT_TIMEOUT, reinit=False):

        self.modtype = ""
        self.host = ""
        self.tcpport = 0
        self.serport = ""
        self.baudrate = 0
        self.bytesize = 0
        self.parity = ""
        self.stopbits = 0

        if "tcp-ip" in modparms.keys():
            self.modtype = "tcp-ip"
            tcpparms = modparms[self.modtype]
            assert "host" in tcpparms.keys() and \
                   "port" in tcpparms.keys(), \
                "Missing mandatory Modbus TCP/IP gateway parameters"
            try:
                # noinspection string-conversion-without-dunder-method
                self.host = str(ipaddress.ip_address(tcpparms["host"]))
                self.tcpport = int(tcpparms["port"])
            except (ValueError,
                    ipaddress.AddressValueError,
                    ipaddress.NetmaskValueError) as err:
                assert False, "Invalid Modbus IP address. Error: {:s}!".format(str(err))
        elif "serial" in modparms.keys():
            self.modtype = "serial"
            serparms = modparms[self.modtype]
            assert "port" in serparms.keys() and \
                   "baudrate" in serparms.keys() and \
                   "bytesize" in serparms.keys() and \
                   "parity" in serparms.keys() and \
                   "stopbits" in serparms.keys(), \
                "Missing mandatory Modbus Serial gateway parameters!"
            try:
                self.serport = str(serparms["port"])
                self.baudrate = int(serparms["baudrate"])
                self.bytesize = int(serparms["bytesize"])
                if serparms["parity"] in ["N", "E", "O"]:
                    self.parity = str(serparms["parity"])
                else:
                    self.parity = int(serparms["parity"])
                self.stopbits = int(serparms["stopbits"])
            except ValueError as err:
                assert False, "Invalid Modbus Serial parameters. Error: {:s}!".format(str(err))
            assert 7 <= self.bytesize <= 8, "Invalid byte size!"
            assert 1 <= self.stopbits <= 2, "Invalid stop bits!"
        else:
            assert False, "Missing Modbus TCP/IP or Serial parameters"

        self.name = ""
        self.sensaddr = -1
        self.sensdict = {}
        self.nreads = 0
        self.tmbtwrd = -1.0
        self.initmout = float(ini_tmout)
        self.oprtmout = -1.0
        self.nretries = -1
        self.reinit = bool(reinit)
        self.cntinit = -1
        self.nparms = -1
        self.regini = -1
        self.regnrg = -1
        self.parmidx = {}
        self.modbobj = None
        self.modblog = ""
        timeout = time.time() + self.initmout
        while time.time() < timeout and \
                self.cntinit < C_SENS_MAX_REINIT:
            time.sleep(self.initmout / C_SENS_MAX_REINIT)
            self.__init_sensor()
            if self.modbobj is not None:
                self.cntinit = -1
                break
        return

    def __init_sensor(self):
        print("--- Initialize Modbus sensors ---")
        self.cntinit += 1
        self.__null_attribs()
        if self.cntinit > C_SENS_MAX_REINIT:
            return
        try:
            if self.modtype == "tcp-ip":
                # noinspection PyArgumentList
                self.modbobj = ModbusTcpClient(self.host, port=self.tcpport)
            elif self.modtype == "serial":
                # noinspection PyTypeChecker
                self.modbobj = ModbusSerialClient(self.serport,
                                                  framer=framer.FramerType.RTU,
                                                  baudrate=self.baudrate,
                                                  bytesize=self.bytesize,
                                                  parity=self.parity,
                                                  stopbits=self.stopbits)
            else:
                assert False, "Invalid Modbus gateway type!"
            self.modbobj.connect()
        except (RuntimeError, ModbusException) as err:
            self.__null_attribs()
            self.__modblogerror("Modbus initialization failed! Error: {:s}".format(str(err)))
            return
        if not self.modbobj.connected:
            self.__null_attribs()
            self.__modblogerror("Modbus not connected!")
            return
        return

    def __reinit_sensor(self):
        print("--- Re-initialize Modbus sensors ---")
        if self.reinit:
            self.close_sensor()
            timeout = time.time() + self.initmout
            while time.time() < timeout and \
                    self.cntinit < C_SENS_MAX_REINIT:
                time.sleep(self.initmout / C_SENS_MAX_REINIT)
                self.__init_sensor()
                if self.modbobj is not None:
                    self.cntinit = -1
                    break
        return

    def __null_attribs(self):
        self.modbobj = None
        return

    def is_modbus_connected(self):
        return self.modbobj is not None and self.modbobj.connected

    def set_reinit(self, reinit, ini_tmout=C_SENS_INIT_TIMEOUT):
        self.reinit = reinit
        self.cntinit = -1
        self.initmout = ini_tmout
        return

    def set_oper_timeout(self, opr_tmout=C_SENS_OPER_TIMEOUT):
        self.oprtmout = opr_tmout
        return

    def reset_modblog(self):
        self.modblog = ""
        return

    def get_modblog(self):
        return self.modblog

    def get_reset_modblog(self):
        log = copy.deepcopy(self.get_modblog())
        self.reset_modblog()
        return log

    def __modblogerror(self, errstr):
        self.modblog += ">> {:s} - Sensor{:s} - Modbuserror: {:s}\n". \
            format(time.ctime(), self.name, errstr)
        return

    def __modblogok(self, resp):
        regs = resp.registers
        nregs = len(regs)
        self.modblog += ">> {:s} - Sensor{:s} - Modbus read {:d} registers: ". \
            format(time.ctime(), self.name, nregs)
        for i in range(nregs):
            if i > 0:
                self.modblog += ","
            # noinspection string-format
            self.modblog += " 0X{:04x}".format(regs[i]).upper()
        self.modblog += "\n"
        return

    def __modbus_read_registers(self, inireg, sz):
        resp, readok, errortxt = [], False, ""
        for _ in range(self.nretries):
            if not self.is_modbus_connected():
                errortxt = "Modbus not connected!"
                self.__modblogerror(errortxt)
                resp = []
                self.__reinit_sensor()
                continue
            try:
                resp = self.modbobj.read_holding_registers(inireg, count=sz, device_id=self.sensaddr)
            except ModbusException as err:
                errortxt = str(err)
                self.__modblogerror(errortxt)
                time.sleep(self.tmbtwrd)
                resp = []
                self.__reinit_sensor()
                continue
            if resp.isError():
                errortxt = str(resp)
                self.__modblogerror(errortxt)
                time.sleep(self.tmbtwrd)
                resp = []
                self.__reinit_sensor()
                continue
            readok = True
            errortxt = ""
            self.__modblogok(resp)
            break

        if readok:
            # noinspection unresolved-references
            regread = resp.registers
        else:
            regread = []
            print("Sensor {:s} --> Modbus Error on {:d} retries: {:s}!".
                  format(self.name, self.nretries, str(errortxt)))
        return regread

    def modbus_write_register(self, wrtreg, wrtval):
        writeok, errortxt = False, ""
        for _ in range(self.nretries):
            if not self.is_modbus_connected():
                errortxt = "Modbus not connected!"
                self.__modblogerror(errortxt)
                self.__reinit_sensor()
                continue
            try:
                resp = self.modbobj.write_register(wrtreg, wrtval, device_id=self.sensaddr)
            except ModbusException as err:
                errortxt = str(err)
                self.__modblogerror(errortxt)
                time.sleep(self.tmbtwrd)
                self.__reinit_sensor()
                continue
            if resp.isError():
                errortxt = str(resp)
                self.__modblogerror(errortxt)
                time.sleep(self.tmbtwrd)
                self.__reinit_sensor()
                continue
            writeok = True
            errortxt = ""
            self.__modblogok(resp)
            break

        if not writeok:
            print("Sensor {:s} --> Modbus Error on {:d} retries: {:s}!".
                  format(self.name, self.nretries, str(errortxt)))
        return

    def __get_parm_value(self, modbregs, parm):
        if not self.is_modbus_connected():
            return -1.0
        parmdict = self.sensdict[parm]
        idxhigh = parmdict["reg_high"]
        idxlow = parmdict["reg_low"]
        regtype = parmdict["reg_type"]
        vallow = modbregs[idxlow]
        if idxhigh != idxlow:
            valhigh = modbregs[idxhigh]
        else:
            valhigh = 0
        if regtype == "f":
            regval = self.modbobj.convert_from_registers([valhigh, vallow],
                                                         self.modbobj.DATATYPE.FLOAT32)
        elif parmdict["reg_type"] == "i":
            regval = self.modbobj.convert_from_registers([valhigh, vallow],
                                                         self.modbobj.DATATYPE.INT32)
        else:
            print("Sensor {:s} --> parameter {:s} Invalid format!".
                  format(self.name, parm))
            regval = -1.0
        regval = round(regval * parmdict["reg_fact"], parmdict["reg_decp"])
        return regval

    def read_registers(self, sname, sensor_addr, sensor_dict,
                       n_reads=C_SENS_DEFAULT_N_READS,
                       tm_btw_rd=C_SENS_DEFAULT_BETWEEN_READS,
                       opr_tmout=C_SENS_OPER_TIMEOUT,
                       n_retries=C_SENS_OPER_RETRIES):
        print("--- Read registers ---")
        if not self.is_modbus_connected():
            return {}
        self.name = str(sname)
        self.sensaddr = int(sensor_addr)
        assert self.sensaddr < 256, "Invalid sensor address!"
        self.sensdict = sensor_dict
        assert isinstance(self.sensdict, dict), "Sensors parameters descriptor is not a dictionary!"
        self.nreads = int(n_reads)
        assert 0 < self.nreads <= C_SENS_MAX_READS, "Number of reads out of range!"
        self.tmbtwrd = float(tm_btw_rd)
        assert 0.0 < self.tmbtwrd <= C_SENS_MAX_BETWEEN_READS, "Invalid time between reads!"
        self.oprtmout = float(opr_tmout)
        assert self.oprtmout >= 0.001, "Operation timeout should be more than 0.001 sec!"
        self.nretries = int(n_retries)
        assert 0 <= self.nretries <= C_SENS_MAX_RETRIES, "Invalid number or operation retries!"

        self.nparms = len(self.sensdict)
        assert 1 <= self.nparms < C_SENS_MAX_INDX, "Sensor parameters dictionary invalid length!"
        self.regini = C_SENS_MAX_INDX
        self.regnrg = 0
        self.parmidx = {}
        idx = -1
        for parm in self.sensdict.keys():
            assert parm not in self.parmidx.keys(), "Duplicated sensor parameter!"
            idx += 1
            self.parmidx[parm] = idx
            parmdict = self.sensdict[parm]
            assert isinstance(parmdict, dict) and \
                   len(parmdict) == C_SENS_PARM_LEN and \
                   "reg_low" in parmdict.keys() and \
                   "reg_high" in parmdict.keys() and \
                   "reg_decp" in parmdict.keys() and \
                   "reg_fact" in parmdict.keys() and \
                   "reg_type" in parmdict.keys(), "Invalid sensor parameter!"
            try:
                parmdict["reg_low"] = int(parmdict["reg_low"])
                parmdict["reg_high"] = int(parmdict["reg_high"])
                parmdict["reg_decp"] = int(parmdict["reg_decp"])
                parmdict["reg_fact"] = float(parmdict["reg_fact"])
            except ValueError:
                assert False, "Parameter register values invalid types!"
            assert parmdict["reg_low"] < C_SENS_MAX_INDX and \
                   parmdict["reg_low"] < C_SENS_MAX_INDX, \
                "Invalid sensor parameter register indexes!"
            assert parmdict["reg_type"] == "f" or \
                   parmdict["reg_type"] == "i", (
                "Invalid parameter register type {:s}!".format(parmdict["reg_type"]))
            self.regini = min(self.regini, parmdict["reg_low"], parmdict["reg_high"])
            self.regnrg = max(self.regnrg, parmdict["reg_low"], parmdict["reg_high"])
        self.regnrg += 1

        parmreads = np.zeros((self.nparms, self.nreads))
        resul = {}
        for r in range(self.nreads):
            time.sleep(self.tmbtwrd)
            regs = self.__modbus_read_registers(self.regini, self.regnrg)
            for parm in self.sensdict.keys():
                if len(regs) > 0:
                    regval = self.__get_parm_value(regs, parm)
                    parmreads[self.parmidx[parm], r] = regval
                else:
                    parmreads[self.parmidx[parm], r] = -1.0
        parm_time = time.time()

        for parm in self.parmidx.keys():
            selrd = np.where(parmreads[self.parmidx[parm], :] > 0.0, True, False)
            parm_nreads = np.count_nonzero(parmreads[self.parmidx[parm], :] >= 0.0)
            parm_nonzero = np.count_nonzero(selrd)
            if parm_nonzero > 0:
                parm_mean = round(np.sum(parmreads[self.parmidx[parm], selrd]) / parm_nonzero,
                                  self.sensdict[parm]["reg_decp"])
                resul[parm] = [parm_mean, self.sensdict[parm]["reg_unit"],
                               parm_nonzero, parm_nreads, parm_time]
            elif parm_nreads > 0:
                parm_mean = round(np.sum(parmreads[self.parmidx[parm], selrd]) / parm_nreads,
                                  self.sensdict[parm]["reg_decp"])
                resul[parm] = [parm_mean, self.sensdict[parm]["reg_unit"],
                               parm_nonzero, parm_nreads, parm_time]
            else:
                resul[parm] = [-1.0, self.sensdict[parm]["reg_unit"], 0, 0, parm_time]

        return resul

    def recover_sensor(self, sname, sensor_dict):
        print("--- Recovering sensor {:s} ---".format(sname))
        if not self.is_modbus_connected():
            return
        try:
            self.read_registers(sname, 0, sensor_dict, n_reads=1, n_retries=1)
        except RuntimeError:
            pass
        return

    def close_sensor(self):
        print("--- Close Modbus sensors ---")
        if self.modbobj is not None:
            self.modbobj.close()
        self.__null_attribs()
        return


##############################################################################################
# main:
##############################################################################################
if __name__ == "__main__":
    print(__doc__)

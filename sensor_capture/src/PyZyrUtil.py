import csv
import gzip
import json
import os
import pickle as pkl
import re
import time
from datetime import datetime, timezone, date

__author__ = "Luiz Claudio Navarro"
__version__ = "1.0.4"
__date__ = "2025.05.09"
libgroup = "Zyryus Projects Infrastructure Libraries."
libname = "General utility functions."
libversion = "Version: " + __version__ + " Date: " + __date__ + "."
copyrightmsg = "Zyryus Consulting (c) 2019-2025."
__doc__ = libgroup + " " + libname + "\n" + libversion + \
          "\nAuthor: " + __author__ + " - " + copyrightmsg + "\n"


# ==============================================================================
# Program Begin and End Header Class
# ==============================================================================
class ZyrProgHeader:
    def __init__(self, doc):
        self.initime = datetime.now()
        self.doc = doc

    def print_begin(self):
        print("++++++++++++++++++++++++++ Begin Program ++++++++++++++++++++++++++")
        if self.doc is not None:
            print(self.doc, end="")
        print("Begin time {:s}.".format(str(self.initime)))
        print("++++++++++++++++++++++++++ Begin Program ++++++++++++++++++++++++++")
        return self.initime

    def print_end(self, n_items: None | int, itemname):
        print("+++++++++++++++++++++++++++ End Program +++++++++++++++++++++++++++")
        if self.doc is not None:
            print(self.doc, end="")
        endtime = datetime.now()
        elapsed = endtime - self.initime

        print("Begun at .......................: {:s}".format(str(self.initime)))
        print("Finished at ....................: {:s}".format(str(endtime)))
        print("Elapsed ........................: {:s}".format(str(elapsed)))
        if n_items is not None and n_items != 0 and itemname:
            print("Number of {:s}(s) processed ....: {:d}".
                  format(itemname, n_items))
            print("Time per {:s} ..................: {:7.3f} seconds".
                  format(itemname,
                         round(elapsed.total_seconds() / n_items, 3)))
        print("+++++++++++++++++++++++++++ End Program +++++++++++++++++++++++++++")
        return


# ==============================================================================
# Utility functions
# ==============================================================================
def intround(val):
    return int(round(val, 0))


# ==============================================================================
# string related functions
# ==============================================================================
def replace_nonfnamechar(strfname, repchar="_"):
    """
    Replace special characters with '_' characters which are not allowed to file name.
    @param strfname:
    @rtype str
    """
    return re.sub(r'[^a-zA-Z0-9_]', repchar, strfname)


# ==============================================================================
# path related functions
# ==============================================================================
def std_path_name(str_path):
    return str_path.replace('\\', '/')


def join_path_name(pathdir, pathbase):
    return std_path_name(os.path.join(pathdir, pathbase))


def get_parent_dir(pathdir):
    pathdir = std_path_name(pathdir)
    if pathdir[-1] == '/':
        pathdir = pathdir[:-1]
    return std_path_name(os.path.split(pathdir)[0])


# ==============================================================================
# JSON files utility functions
# ==============================================================================
def is_gzip_file(fullfname):
    """
    Return if the file is a gzip file
    :param fullfname: str
    :return: bool
    """
    with open(fullfname, 'rb') as f:
        fmagic = f.read(3)
    return fmagic == b'\x1f\x8b\x08'


def json_save_unziped(jsonfname, jsondict, msg):
    if not jsonfname.endswith('.json'):
        jsonfname += '.json'
    if msg is not None:
        print("*** Saving {:s} to JSON file {:s} ***".
              format(msg, os.path.basename(jsonfname)))
    with open(jsonfname, "w") as jsonfile:
        # noinspection PyTypeChecker
        json.dump(jsondict, jsonfile)
    return jsonfname


def json_save_gzip(jsonfname, jsondict, msg):
    if not jsonfname.endswith('.gzj'):
        jsonfname += '.gzj'
    if msg is not None:
        print("*** Saving {:s} to JSON Gziped file {:s} ***".
              format(msg, os.path.basename(jsonfname)))
    with gzip.open(jsonfname, "w") as jsonfile:
        jsonfile.write(json.dumps(jsondict).encode('utf-8'))
    return jsonfname


def json_load_unziped(jsonfname, msg):
    jsondict = {}
    if not jsonfname.endswith('.json'):
        jsonfname += '.json'
    if msg is not None:
        print("*** Loading {:s} from JSON file {:s} ***".
              format(msg, os.path.basename(jsonfname)))
    if os.path.isfile(jsonfname):
        with open(jsonfname, "r") as jsonfile:
            jsondict = json.load(jsonfile)
    return jsondict, jsonfname


def json_load_gzip(jsonfname, msg):
    jsondict = {}
    if not jsonfname.endswith('.gzj'):
        jsonfname += '.gzj'
    if msg is not None:
        print("*** Loading {:s} from JSON Gziped file {:s} ***".
              format(msg, os.path.basename(jsonfname)))
    if is_gzip_file(jsonfname):
        with gzip.open(jsonfname, "r") as jsonfile:
            jsondict = json.loads(jsonfile.read().decode('utf-8'))
    return jsondict, jsonfname


# ==============================================================================
# text files utility functions
# ==============================================================================
def read_text_file(txtfname, msg):
    print("*** Reading {:s} from text file {:s} ***".
          format(msg, os.path.basename(txtfname)))
    with open(txtfname, "r") as txtfile:
        txtstr = txtfile.readlines()
    return txtstr


def write_text_file(txtfname, textstr, msg):
    print("*** Saving {:s} to text file {:s} ***".
          format(msg, os.path.basename(txtfname)))
    with open(txtfname, "w") as txtfile:
        txtfile.write(textstr)
    return


def add_text_to_file(txtfname, textstr):
    with open(txtfname, "a") as txtfile:
        txtfile.write(textstr)
    return


def write_text_lines(txtfname, textstr, hdr=None, msg=None, mode="w", day=False):
    assert mode in ["w", "a"], "Invalid write mode {:s}".format(mode)
    if day:
        ext = "." + os.path.basename(txtfname).split(".")[-1]
        txtfname = txtfname.replace(ext, "_{:s}".format(time.strftime("%Y%m%d")) + ext)
    txtdir = os.path.dirname(txtfname)
    txtfbase = os.path.basename(txtfname)
    assert os.path.isdir(txtdir), \
        "Text file folder {:s} not found!".format(txtdir)
    if msg is not None:
        print("*** Saving {:s} into file {:s} ***".
              format(msg, os.path.basename(txtfbase)))
    txtexists = os.path.isfile(txtfname)
    with open(txtfname, mode) as txtfile:
        if hdr is not None and not txtexists:
            txtfile.writelines(hdr)
        txtfile.writelines(textstr)
    if msg is not None:
        print("    Lines saved into file: {:s}".format(txtfbase))
    return


# ==============================================================================
# Read / Write generic CSV file
# ==============================================================================
def read_csv_file(rfname, header=True, checklen=True):
    if not header:
        checklen = False
    rfbase = os.path.basename(rfname)
    print("Reading data from file {:s}".format(rfbase))
    assert os.path.isfile(rfname) and rfbase.endswith(".csv"), \
        "CSV file {:s} not found!".format(rfbase)
    ncls, nrow, valrows, hdrrow = 0, 0, [], []
    with open(rfname, 'r', newline='') as rfile:
        rrows = csv.reader(rfile, delimiter=',')
        for row in rrows:
            if header and nrow == 0:
                hdrrow = row
                ncls = len(hdrrow)
            else:
                if header and checklen:
                    assert ncls == len(row), "Invalid values row length"
                valrows.append(row)
            nrow += 1
    assert nrow > 0, "Empty file!"
    print("    {:d} lines read from file {:s}".format(nrow, rfbase))
    return hdrrow, valrows


def write_csv_file(wfname, hdrrow, valrows, title="", addflag=False, day=False, checklen=True):
    if day:
        wfname = wfname.replace(".csv", "_{:s}.csv".format(time.strftime("%Y%m%d")))
    wfbase = os.path.basename(wfname)
    assert wfbase.endswith(".csv"), \
        "File name {:s} is not .csv!".format(wfbase)
    if title:
        print("Writing {:s} into file {:s}".format(title, wfbase))
    if hdrrow is not None:
        ncls = len(hdrrow)
    else:
        checklen = False
        ncls = 0
    nrow = len(valrows)
    file_exists = os.path.isfile(wfname)
    if addflag:
        opentype = "a"
    else:
        opentype = "w"
    with open(wfname, opentype, newline='') as wfile:
        wwriter = csv.writer(wfile, delimiter=',')
        if (not file_exists or not addflag) and hdrrow is not None:
            wwriter.writerow(hdrrow)
        for i in range(nrow):
            if checklen:
                assert ncls == len(valrows[i]), "Invalid values row length"
            wwriter.writerow(valrows[i])
    print("    {:d} lines saved into {:s}.".format(nrow, wfbase))
    return


# ==============================================================================
# Read / Write Pickle file
# ==============================================================================
def save_object(objfname, saveobj):
    objdir = os.path.dirname(objfname)
    objfbase = os.path.basename(objfname)
    assert os.path.isdir(objdir) and objfbase.endswith(".pkl"), \
        "Invalid Object file name"
    with open(objfname, 'wb') as pkl_file:
        # noinspection PyTypeChecker
        pkl.dump(saveobj, pkl_file)
    return


def load_object(objfname):
    assert os.path.isfile(objfname) and objfname.endswith(".pkl"), \
        "Object .pkl file not found!"
    with open(objfname, 'rb') as pickle_file:
        loadobj = pkl.load(pickle_file)
    return loadobj


# ==============================================================================
# Time related
# ==============================================================================
def build_utc_date_time(year, month, day, hour, minute, second, utc=False):
    if utc:
        zinfo = timezone.utc
    else:
        zinfo = datetime.tzname(datetime.now())
    dt = datetime(year, month, day, hour, minute, second, tzinfo=zinfo)
    return dt


def date_time_to_secs(dt):
    return int(dt.timestamp())


def utc_date_time():
    return datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")


def format_sectime(sectime):
    return time.asctime(time.localtime(sectime))


def sectime_format_to_secs(dtstr, utc=False):
    # record datetime: 0Weekday(Www) 1Month(Mmm) 2Day (1or2d) 3HH:mm:ss 4YYYY
    month_str = ["",
                 "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
    if utc:
        zinfo = timezone.utc
    else:
        zinfo = datetime.tzname(datetime.now())
    try:
        s = dtstr.replace("  ", " ").split(" ")
        day = int(s[2])
        month = int(month_str.index(s[1]))
        year = int(s[4])
        h = s[3].split(":")
        hour = int(h[0])
        minute = int(h[1])
        second = int(h[2])
        dtsecs = int(datetime(year, month, day, hour, minute, second, tzinfo=zinfo).timestamp())
    except (ValueError, IndexError):
        assert False, "invalid datetime string {:s}".format(dtstr)
    return dtsecs


def secs_to_string(dtsecs):
    return time.strftime("%Y%m%d-%H%M%S", time.localtime(dtsecs))


def extract_date_from_secs(dtsecs):
    dt = datetime.fromtimestamp(dtsecs)
    return (int(dt.year), int(dt.month), int(dt.day),
            int(dt.hour), int(dt.minute), int(dt.second))


def secs_of_day(utc=False):
    if utc:
        zinfo = timezone.utc
    else:
        zinfo = datetime.tzname(datetime.now())
    now = datetime.now(zinfo)
    return (now.hour * 3600) + (now.minute * 60) + now.second


def date_secs(utc=False):
    if utc:
        zinfo = timezone.utc
    else:
        zinfo = datetime.tzname(datetime.now())
    now = datetime.now(zinfo)
    datestr = now.strftime("%Y/%m/%d")
    secs = (now.hour * 3600) + (now.minute * 60) + now.second
    return datestr, secs


def hhmm_to_seconds(hhmm):
    assert len(hhmm) == 5 and hhmm[2] == ":", "Invalid hh:mm string!"
    try:
        hours = (int(hhmm[0]) * 10) + int(hhmm[1])
        mins = (int(hhmm[3]) * 10) + int(hhmm[4])
    except ValueError:
        assert False, "Invalid hh mm values!"
    assert 0 <= hours <= 23 and 0 <= mins <= 59, "Invalid hh mm values!"
    secs = (hours * 3600) + (mins * 60)
    return secs


def seconds_to_hhmmss(secs):
    secsday = intround(secs) % (24 * 3600)
    hour = float(secsday) / 3600.0
    mins = int((hour - float(int(hour))) * 60.0)
    return "{:02d}:{:02d}".format(int(hour), mins)


def get_image_date_time(fname):
    fbase = os.path.basename(fname).split(".")[0]
    rematch = re.match(r"\w+_(\d{8})-(\d{6})", fbase)
    if rematch is not None:
        dt = rematch.group(1)
        tm = rematch.group(2)
        assert len(dt) == 8 and len(tm) == 6, "Invalid date and time strings"
        dttmsecs = date_time_to_secs(
            build_utc_date_time(int(dt[:4]), int(dt[4:6]), int(dt[6:]),
                                int(tm[:2]), int(tm[2:4]), int(tm[4:])))
        dt0secs = date_time_to_secs(
            build_utc_date_time(2023, 12, 31,
                                0, 0, 0))
        dtkey = dttmsecs - dt0secs
        return dt + "-" + tm, int(dt), int(tm), dtkey
    else:
        return "", 0, 0, 0


def sep_image_date_time(dttm):
    dt, tm = "", ""
    redttm = re.match(r"(\d{8})-(\d{6})", dttm)
    if redttm is not None:
        dt, tm = redttm.group(1), redttm.group(2)
    assert len(dt) == 8 and len(tm) == 6, "Invalid date/time YYYYMMDD-HHMMSS format!"
    return dt, tm


def int_date_iso_format(yyyymmdd):
    year = int(yyyymmdd // 10000)
    monthday = yyyymmdd % 10000
    month = int(monthday // 100)
    day = int(monthday % 100)
    return date(year, month, day).isoformat()


# ==============================================================================
# Configuration related functions
# ==============================================================================
def set_config_default_and_type(defdict, cfgdict):
    for key in defdict.keys():
        if key in cfgdict.keys():
            if type(defdict[key]) is bool:
                cfgdict[key] = bool(cfgdict[key])
            assert type(defdict[key]) == type(cfgdict[key]), \
                "Configuration parameter {:s} wrong type!".format(key)
        else:
            cfgdict[key] = defdict[key]
    return


##############################################################################################
# main:
##############################################################################################
if __name__ == "__main__":
    print(__doc__)

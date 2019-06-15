#!/usr/bin/env python3
"""Generate SupportedProtocols.md by scraping source code files"""
import pathlib
import argparse
import sys
import re
import time

CODE_URL = "https://github.com/markszabo/IRremoteESP8266/blob/master/src/ir_"

BRAND_MODEL = re.compile(r"Brand: *(?P<brand>.+), *Model: *(?P<model>.+)")
ENUMS = re.compile(r"enum \w+ {(.+?)};", re.DOTALL)
ENUM_ENTRY = re.compile(r"^\s+(\w+)", re.MULTILINE)
DECODED_PROTOCOLS = re.compile(r".*results->decode_type *=.*?(\w+);")
AC_FN = re.compile(r"ir_(.+).h")

ALL_FN = re.compile(r"ir_(.+).(h|cpp)")

EXCLUDED_PROTOCOLS = ["UNKNOWN", "UNUSED", "kLastDecodeType"]

MARKDOWN_HEADER = """<!--- WARNING: Do NOT edit this file directly.
      It is generated by './tools/scrape_supported_devices.py'.
      Last generated: {} --->""".format(time.asctime())


def getallprotocols(srcdir):
  """Return all protocls configured in IRremoteESP8266.h
  """
  irremote = srcdir / "IRremoteESP8266.h"
  enums = getenums(irremote)
  if not enums:
    errorexit("Error getting ENUMS from IRremoteESP8266.h")
  return enums


def getdecodedprotocols(srcdir):
  """All protocols that include decoding support"""
  ret = set()
  for path in srcdir.iterdir():
    if path.suffix != ".cpp":
      continue
    matches = DECODED_PROTOCOLS.finditer(path.open().read())
    for match in matches:
      protocol = match.group(1)
      if protocol not in EXCLUDED_PROTOCOLS:
        ret.add(protocol)
  return ret


def getallacs(srcdir):
  """All supported A/C codes"""
  ret = {}
  for path in srcdir.iterdir():
    match = AC_FN.match(path.name)
    if not match:
      continue
    acprotocol = match.group(1)
    rawmodels = getenums(path)
    models = set()
    for model in rawmodels:
      model = model.upper()
      model = model.replace("K{}".format(acprotocol.upper()), "")
      if model and model not in EXCLUDED_PROTOCOLS:
        models.add(model)
    ret[acprotocol] = models
  return ret


def getalldevices(srcdir):
  """All devices and associated branding and model information (if available)
  """
  allcodes = {}
  fnnomatch = set()
  fnmatch = set()
  for path in srcdir.iterdir():
    match = ALL_FN.match(path.name)
    if not match:
      continue
    supports = extractsupports(path)
    if supports:
      fnmatch.add(path.stem)
    else:
      fnnomatch.add(path.stem)
    protocol = match.group(1)
    for brand, model in supports:
      protocolbrand = (protocol, brand)
      allcodes[protocolbrand] = allcodes.get(protocolbrand, list()) + [model]
  nosupports = fnnomatch - fnmatch
  for fnprotocol in nosupports:
    allcodes[(fnprotocol[3:], "Unknown")] = []
  return allcodes, nosupports


def getenums(path):
  """Returns the keys for the first enum type in path
    """
  enums = ENUMS.search(path.open().read())
  ret = set()
  if not enums:
    return ret
  for enum in ENUM_ENTRY.finditer(enums.group(1)):
    enum = enum.group(1)
    if enum in EXCLUDED_PROTOCOLS:
      continue
    ret.add(enum)
  return ret


ARGS = None


def initargs():
  """Init the command line arguments"""
  global ARGS  # pylint: disable=global-statement
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "-s",
      "--stdout",
      help="output to stdout rather than SupportedProtocols.md",
      action="store_true",
  )
  parser.add_argument("-v",
                      "--verbose",
                      help="increase output verbosity",
                      action="store_true")
  parser.add_argument(
      "-a",
      "--alert",
      help="alert if a file does not have a supports section",
      action="store_true",
  )
  parser.add_argument(
      "directory",
      nargs="?",
      help="directory of the source git checkout",
      default=None,
  )
  ARGS = parser.parse_args()
  return ARGS


def errorexit(msg):
  """Print an error and exit on critical error"""
  sys.stderr.write("{}\n".format(msg))
  sys.exit(1)


def extractsupports(path):
  """Extract all of the Supports: sections and associated brands and models
  """
  supports = []
  insupports = False
  for line in path.open():
    if not line.startswith("//"):
      continue
    line = line[2:].strip()
    if line == "Supports:":
      insupports = True
      continue
    if insupports:
      match = BRAND_MODEL.match(line)
      if match:
        supports.append((match.group("brand"), match.group("model")))
      else:
        insupports = False
        continue
  return supports


def makeurl(txt, path):
  """Make a Markup URL from given filename"""
  return "[{}]({})".format(txt, CODE_URL + path)


def outputprotocols(fout, protocols):
  """For a given protocol set, sort and output the markdown"""
  protocols = list(protocols)
  protocols.sort()
  for protocol in protocols:
    fout.write("- {}\n".format(protocol))


def main():
  """Standard boiler plate"""
  initargs()
  if ARGS.directory is None:
    src = pathlib.Path("../src")
    if not src.is_dir():
      src = pathlib.Path("./src")
  else:
    src = pathlib.Path(ARGS.directory) / "src"
  if not src.is_dir():
    errorexit("Directory not valid: {}".format(str(src)))
  if ARGS.verbose:
    print("Looking for files in: {}".format(str(src.resolve())))
  if ARGS.stdout:
    fout = sys.stdout
  else:
    foutpath = src / "../SupportedProtocols.md"
    foutpath = foutpath.resolve()
    if ARGS.verbose:
      print("Output path: {}".format(str(foutpath)))
    fout = foutpath.open("w")
  allprotocols = getallprotocols(src)
  decodedprotocols = getdecodedprotocols(src)
  sendonly = allprotocols - decodedprotocols
  allacs = getallacs(src)

  allcodes, nosupports = getalldevices(src)
  allbrands = list(allcodes.keys())
  allbrands.sort()

  fout.write(MARKDOWN_HEADER)
  fout.write("\n# IR Protocols supported by this library\n\n")
  fout.write(
      "| Protocol | Brand | Model | A/C Model | Detailed A/C Support |\n")
  fout.write("| --- | --- | --- | --- | --- |\n")

  for protocolbrand in allbrands:
    protocol, brand = protocolbrand
    codes = allcodes[protocolbrand]
    codes.sort()
    if protocol in allacs:
      acmodels = list(allacs[protocol])
      acmodels.sort()
      acsupport = "Yes"
      brand = makeurl(brand, protocol + ".h")
    else:
      acmodels = []
      acsupport = "-"

    fout.write("| {} | **{}** | {} | {} | {} |\n".format(
        makeurl(protocol, protocol + ".cpp"),
        brand,
        "<BR>".join(codes),
        "<BR>".join(acmodels),
        acsupport,
    ))

  fout.write("\n\n## Send only protocols:\n\n")
  outputprotocols(fout, sendonly)

  fout.write("\n\n## Send & decodable protocols:\n\n")
  outputprotocols(fout, decodedprotocols)

  if ARGS.alert:
    nosupports = list(nosupports)
    nosupports.sort()
    print("The following files had no supports section:")
    for path in nosupports:
      print("\t{}".format(path))


if __name__ == "__main__":
  main()

#!/bin/python3

import getopt
import serial
import sys

import matplotlib.pyplot as plt

from parser import parse_seismometer_line
from sample_database import set_max_database_length, get_sample_data_array

program_name_str="Sandor Laboratories Seismometer Monitor"
version_str="0.0.1-dev"
publish_year_str="February 2023"
author_str="Edward Sandor"
website_str="https://git.sandorlaboratories.com/edward/seismometer"

title_block_str=program_name_str +"\n" \
                "Version " + version_str + "\n" \
                + author_str + ", " + publish_year_str + "\n" \
                + website_str

def main(argv) -> int:
  print(title_block_str)

  #Init working variables
  default_serial_path="/dev/ttyACM0"
  serial_path=default_serial_path
  default_serial_baud=115200
  serial_baud=default_serial_baud
  default_sample_buffer_length=10000
  sample_buffer_length=default_sample_buffer_length

  help_string=title_block_str + "\n\n" \
              "This script monitors, analyzes, and visualizes data from the seismometer data collector.\n\n" \
              "Arguments:\n" \
              "   -h             --help             Prints this Help information and exits.\n" \
              "   -b <baudrate>, --baud <baudrate>  Serial device baud.  Defaults to '" + str(default_serial_baud) + "'\n" \
              "   -s <path>,     --serial=<path>    Serial device path.  Defaults to '" + default_serial_path + "'\n" \

  #Parse command line arguments
  try:
      opts, args = getopt.getopt(argv,"hs:",["help", "serial=",])
  except getopt.GetoptError as err:
      print(err)
      print("\n"+help_string)
      sys.exit(22)

  for opt, arg in opts:
      if opt in ('-b', "--baud"): 
          serial_baud = int(baud)
      elif opt in ('-h', "--help"): 
          print(help_string)
          sys.exit()
      elif opt in ('-s', "--serial"): 
          serial_path = arg

  set_max_database_length(sample_buffer_length)

  with serial.Serial(serial_path, serial_baud, timeout=1) as ser:
    for i in range(0,10000):
      line = ser.readline().decode('utf-8').strip()
      parse_seismometer_line(line)
    
  samples = get_sample_data_array(1)
  plt.plot(samples)
  plt.show()

  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
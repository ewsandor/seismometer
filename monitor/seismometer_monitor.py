#!/bin/python3

import getopt
import matplotlib.pyplot as plt
import numpy as np
import serial
import sys
import threading
import time

from data_collector_parser import parse_seismometer_line
from sample_database import set_max_database_length, get_sample_data_array

program_name_str="Sandor Laboratories Seismometer Monitor"
version_str="0.0.1-dev"
publish_year_str="February 2023"
author_str="Edward Sandor"
website_str="https://git.sandorlaboratories.com/edward/seismometer"

default_serial_path="/dev/ttyACM0"
serial_path=default_serial_path
default_serial_baud=115200
serial_baud=default_serial_baud
default_max_database_length=500
max_database_length=default_max_database_length


title_block_str=program_name_str +"\n" \
                "Version " + version_str + "\n" \
                + author_str + ", " + publish_year_str + "\n" \
                + website_str

def serial_thread(args):
  #Open Serial Port
  with serial.Serial(serial_path, serial_baud, timeout=1) as ser:
    #Main loop
    while(True):
      line = ser.readline().decode('utf-8').strip()
      parse_seismometer_line(line)

def main(argv) -> int:
  print(title_block_str)

  #Init working variables
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

  # Set database length
  set_max_database_length(max_database_length)

  # Prepare plot
  plt.ion()
  fig = plt.figure()
  ax = fig.add_subplot(111)
  x = np.arange(max_database_length)
  line1, = ax.plot(x, [0]*max_database_length)
  ax.relim() 
  ax.autoscale_view(True,True,True)
  fig.canvas.draw()
  plt.show()
  
  serial_thread_handle=threading.Thread(target=serial_thread, args=(1,))
  serial_thread_handle.start()

  while True:
    samples = get_sample_data_array(3)
    if(samples is not None):
      if(len(samples) < len(x)):
        extension=([0]*(len(x)-len(samples)))
        y=samples+extension
      else:
        y=samples[-max_database_length:]
      line1.set_ydata(y)
    ax.relim() 
    ax.autoscale_view(False,True,True)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(.1)
    
  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
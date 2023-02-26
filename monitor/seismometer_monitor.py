#!/bin/python3
from datetime import datetime
import getopt
import matplotlib.pyplot as plt
import numpy as np
import serial
import sys
import threading
import time

from data_collector_parser import parse_seismometer_line
from sample_database import sample_database

program_name_str="Sandor Laboratories Seismometer Monitor"
version_str="0.0.1-dev"
publish_year_str="February 2023"
author_str="Edward Sandor"
website_str="https://git.sandorlaboratories.com/edward/seismometer"

default_serial_path="/dev/ttyACM0"
serial_path=default_serial_path
default_serial_baud=921600
serial_baud=default_serial_baud
default_max_database_length=500000
max_database_length=default_max_database_length
#plot_channels = [1, 2, 3, 4, 5, 6, 7, 8]
#plot_channels = [5,6,7]
#plot_channels = [11, 10, 12]
plot_channels = [12]


title_block_str=program_name_str +"\n" \
                "Version " + version_str + "\n" \
                + author_str + ", " + publish_year_str + "\n" \
                + website_str

# Set database length
database = sample_database(max_database_length)

def serial_thread(args):
  #Open Serial Port
  print("Opening serial port at '" + str(serial_path + "' with baud '" + str(serial_baud) + "'."))
  while(True):
    try:
      with serial.Serial(serial_path, serial_baud, timeout=1) as ser:
        try:
          while(ser.is_open):
            line = ser.readline().decode('utf-8').strip()
            parse_seismometer_line(database, line)
        except KeyboardInterrupt:
          sys.exit(0)
        except:
          if(ser.is_open):
            ser.close
    except:
      print(str(datetime.now()) + ": Retrying serial port at '"+serial_path+".")

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
      opts, args = getopt.getopt(argv,"b:hs:",["baud=", "help", "serial=",])
  except getopt.GetoptError as err:
      print(err)
      print("\n"+help_string)
      sys.exit(22)

  for opt, arg in opts:
      if opt in ('-b', "--baud"): 
          serial_baud = int(arg)
      elif opt in ('-h', "--help"): 
          print(help_string)
          sys.exit()
      elif opt in ('-s', "--serial"): 
          serial_path = arg

  # Prepare plot
  plt.ion()
  fig = plt.figure()
  ax = fig.add_subplot(111)
  x = np.arange(max_database_length)
  plot_line = {}
  for i in plot_channels:
    plot_line[i], = ax.plot(x, [0]*max_database_length)
  ax.relim() 
  ax.autoscale_view(True,True,True)
  fig.canvas.draw()
  plt.show()
  
  serial_thread_handle=threading.Thread(target=serial_thread, args=(1,))
  serial_thread_handle.start()

  while True:
    for i in plot_channels:
      samples = database.get_sample_data_array(i)
      if(samples is not None):
        x = np.arange(len(samples))
        plot_line[i].set_xdata(x)
        plot_line[i].set_ydata(samples)
    ax.relim() 
    ax.autoscale_view(False,True,True)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(.5)
    
  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
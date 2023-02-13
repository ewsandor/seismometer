import sample_database

def twos_complement(hexstr, bits):
    value = int(hexstr, 16)
    if value & (1 << (bits - 1)):
        value -= 1 << bits
    return value

def parse_seismometer_line(database, line):
  line_split = line.split('|')
  if((5 == len(line_split)) and ('SAMPLE' == line_split[0])):
    sample = {
      'key'      : int(line_split[1],16),
      'index'    : int(line_split[2],16),
      'timestamp': int(line_split[3],16),
      'data'     : twos_complement(line_split[4],64),
    }
    database.push_sample(sample)
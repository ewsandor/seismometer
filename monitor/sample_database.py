from collections import deque

sample_database = {}
max_database_length=1000

def set_max_database_length(new_max_database_length):
  max_database_length=new_max_database_length
  print("Allowing " + str(max_database_length) + " samples per channel")

def push_sample(sample):
  if(sample['key'] not in sample_database):
    sample_database[sample['key']] = deque(maxlen=max_database_length)
  sample_database[sample['key']].append(sample)

def get_sample_array(key):
  if(key in sample_database):
    return sample_database[key]
  else:
    print("Unrecognized key '" + str(key) +"'!")
    print("Known keys: " + str(sample_database.keys()))
    return None
from collections import deque

sample_database = {}

def push_sample(sample):
  if(sample['key'] not in sample_database):
    sample_database[sample['key']] = deque(maxlen=1000)
  sample_database[sample['key']].append(sample)

def get_sample_array(key):
  if(key in sample_database):
    return sample_database[key]
  else:
    print("Unrecognized key '" + str(key) +"'!")
    print("Known keys: " + str(sample_database.keys()))
    return None
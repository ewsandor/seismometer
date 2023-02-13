import threading
from collections import deque

class sample_database:
  sample_database_dict = {}

  def __init__(self, new_max_database_length):
    self.lock = threading.Lock()
    self.max_database_length=new_max_database_length
    print("Allowing " + str(self.max_database_length) + " samples per channel")
    self.pending_queue_lock = threading.Lock()
    self.pending_samples_queue = deque(maxlen=self.max_database_length)

  def flush_pending_samples_queue(self, force=False):
    first_attempt = True 
    flushed = False
    while((first_attempt or force) and not flushed):
      if(self.lock.acquire(blocking=False)):
        self.pending_queue_lock.acquire()
        while(len(self.pending_samples_queue) > 0):
          sample = self.pending_samples_queue.popleft()
          if(sample['key'] not in self.sample_database_dict):
            self.sample_database_dict[sample['key']] = deque(maxlen=self.max_database_length)
          self.sample_database_dict[sample['key']].append(sample)
        flushed = True
        self.pending_queue_lock.release()
        self.lock.release()
      first_attempt = False

  def push_sample(self, sample):
    self.pending_queue_lock.acquire()
    self.pending_samples_queue.append(sample)
    self.pending_queue_lock.release()
    self.flush_pending_samples_queue()

  def get_sample_data_array(self, key):
    ret_val = None
    self.lock.acquire()
    if(key in self.sample_database_dict):
      sample_data = [0] * len(self.sample_database_dict[key])
      i = 0
      for sample in self.sample_database_dict[key]:
        sample_data[i]      = sample['data']
        i = i+1
      ret_val = sample_data
    else:
      print("Unrecognized key '" + str(key) +"'!")
      print("Known keys: " + str(self.sample_database_dict.keys()))
    self.lock.release()
    return ret_val
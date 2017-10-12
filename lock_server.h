// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <thread>
#include <condition_variable>
#include <map>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

struct lock_entry {
    bool acquired = false;
    std::condition_variable cv;
};

class lock_server {

 protected:
  int nacquire;
  std::mutex mutex_;
  std::map<int, lock_entry> locks_;  
 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 








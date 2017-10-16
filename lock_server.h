// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
<<<<<<< HEAD
#include <thread>
#include <condition_variable>
#include <map>
=======
>>>>>>> cf4284c1e97487806b378590fcf15de310910818
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

<<<<<<< HEAD
struct lock_entry {
    bool acquired = false;
    std::condition_variable cv;
};

=======
>>>>>>> cf4284c1e97487806b378590fcf15de310910818
class lock_server {

 protected:
  int nacquire;
<<<<<<< HEAD
  std::mutex mutex_;
  std::map<int, lock_entry> locks_;  
=======

>>>>>>> cf4284c1e97487806b378590fcf15de310910818
 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 








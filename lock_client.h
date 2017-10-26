// lock client interface.

#ifndef lock_client_h
#define lock_client_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include <vector>

// Client interface to the lock server
class lock_client {
 protected:
  rpcc *cl;
 public:
  lock_client(std::string d);
  virtual ~lock_client() {};
  virtual lock_protocol::status acquire(lock_protocol::lockid_t);
  virtual lock_protocol::status release(lock_protocol::lockid_t);
  virtual lock_protocol::status stat(lock_protocol::lockid_t);
};

class yfs_lock {
  public:
    yfs_lock(lock_client *lc, lock_protocol::lockid_t id): id_(id), lc_(lc) {
      lc_->acquire(id_);
    }
    ~yfs_lock() {
      lc_->release(id_);
    }
  private:
    lock_protocol::lockid_t id_;
    lock_client *lc_;
};

#endif 

// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  std::unique_lock<std::mutex> lock(mutex_);

  printf("acquire %d\n", lid);

  while(locks_[lid].acquired) 
    locks_[lid].cv.wait(lock, [&](){ return locks_[lid].acquired == false; });
  locks_[lid].acquired = true;
  nacquire++;
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  printf("release %d\n", lid);
  std::unique_lock<std::mutex> lock(mutex_);
  locks_[lid].acquired = false;
  nacquire--;
  locks_[lid].cv.notify_one();

  return ret;
}

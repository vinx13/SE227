#ifndef yfs_client_h
#define yfs_client_h

#include <string>
#include <map>

#include "lock_protocol.h"
#include "lock_client.h"

//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "inode_manager.h"

#define CA_FILE "./cert/ca.pem:"
#define USERFILE	"./etc/passwd"
#define GROUPFILE	"./etc/group"


class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST,
  			NOPEM, ERRPEM, EINVA, ECTIM, ENUSE };

  typedef int status;


  struct filestat {
  	unsigned long long size;
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
  };

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
	unsigned long mode;
	unsigned short uid;
	unsigned short gid;
  };

  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

  class dir {
    std::map<std::string, inum> entries;
  public:
    dir() = default;
    dir(const std::string &buf);
    void addentry(const std::string &name, mode_t mode, inum ino);
    void removeentry(const std::string &name);
    void lookup(const std::string &name, bool &found, inum &ino) const;
    void dump(std::string &out) const;
    void getentries(std::list<dirent> &entries) const;
  };


 private:
  static std::string filename(inum);
  static inum n2i(std::string);

  int getdirnode(inum ino, dir &dir_out);
  int savedirnode(inum ino, const dir &dirnode);

 public:
  yfs_client();
  yfs_client(std::string, std::string, const char*);

  bool isfile(inum);
  bool isdir(inum);
  bool issymlink(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum, filestat, unsigned long);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);
  
  /** you may need to add symbolic link related methods here.*/
  int symlink(inum, const char *, const char *, inum &);
  int readlink(inum, std::string &);
  int verify(const char* cert_file, unsigned short*);

  void prev();
  void next();
  void commit();
};

#define MAX_NAME_LEN 255

struct dir_entry {
  uint32_t inum;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char *name;

};

#endif 

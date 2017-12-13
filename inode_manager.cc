#include <pthread.h>



#include "inode_manager.h"
#include "version.h"
#include <ctime>

// disk layer -----------------------------------------
static void shift_right(const char *block, int offset, char *dest) {
    memcpy(dest + offset, block, BLOCK_SIZE - offset);
    memcpy(dest, block + BLOCK_SIZE - offset, offset);
}

static void shift_left(const char *block, int offset, char *dest) {
    memcpy(dest, block + offset, BLOCK_SIZE - offset);
    memcpy(dest + BLOCK_SIZE - offset, block, offset);
}

disk::disk()
{
  pthread_t id;
  int ret;
  bzero(blocks, sizeof(blocks));

  ret = pthread_create(&id, NULL, test_daemon, (void*)blocks);
  if(ret != 0)
	  printf("FILE %s line %d:Create pthread error\n", __FILE__, __LINE__);
}

void
disk::read_block(blockid_t id, char *buf)
{
  /*
   *your lab1 code goes here.
   *if id is smaller than 0 or larger than DBLOCK_NUM 
   *or buf is null, just return.
   *put the content of target block into buf.
   *hint: use memcpy
  */

  if (id >= DBLOCK_NUM || !buf) return;

  char *block1 = new char[BLOCK_SIZE], *block2 = new char[BLOCK_SIZE], *block3 = new char[BLOCK_SIZE], *block4 = new char[BLOCK_SIZE];
  const char *block0 = (const char*) blocks[id];
  shift_left((char *)blocks[DBLOCK_NUM + id], 2, block1);
  shift_left((char *)blocks[DBLOCK_NUM*2 + id], 4, block2);
  shift_left((char *)blocks[DBLOCK_NUM*3 + id], 6, block3);
  shift_left((char *)blocks[DBLOCK_NUM*4 + id], 8, block4);

  for (int i = 0; i < BLOCK_SIZE; i++) {
      unsigned char a = block0[i], b = block1[i], c = block2[i], d = block3[i], e = block4[i];
  //    assert((a==b)&&(b==c)&&(c==d)&&(d==e));
      unsigned char byte = (a&b&c)|(a&b&d)|(a&b&e)|(a&c&d)|(a&c&e)|(a&d&e)|(b&c&d)|(b&c&e)|(b&d&e)|(c&d&e);
    //  assert(byte==a);
      //unsigned char byte = (block0[i] & block1[i]) | (block0[i] & block2[i]) | (block1[i] & block2[i]);
      //unsigned char byte = block0[i];
      //assert(block0[i]==block1[i]);assert(block1[i]==block2[i]);
      buf[i] = byte;
  }
}

void
disk::write_block(blockid_t id, const char *buf)
{
  /*
   *your lab1 code goes here.
   *hint: just like read_block
  */
  
  if (id >= DBLOCK_NUM || !buf) return;

  char *block0 = (char *)blocks[id], *block1 = (char *)blocks[DBLOCK_NUM + id], *block2 = (char *)blocks[DBLOCK_NUM * 2 + id], *block3 = (char *)blocks[DBLOCK_NUM * 3 + id], *block4 = (char *)blocks[DBLOCK_NUM * 4 + id];
  memcpy(block0, buf, BLOCK_SIZE);
  shift_right(block0, 2, block1);
  shift_right(block1, 2, block2);
  shift_right(block2, 2, block3);
  shift_right(block3, 2, block4);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  
  std::lock_guard<std::mutex> lock(mtx_);
  char bitmap_buf[BLOCK_SIZE];

  for (blockid_t block = IBLOCK(INODE_NUM + 1, DBLOCK_NUM), bitmap = BBLOCK(0);
       block < DBLOCK_NUM;
       ++bitmap, block += BPB) { 
    read_block(bitmap, bitmap_buf);
    
    for (uint32_t byte_index = 0; byte_index < BLOCK_SIZE; ++byte_index) {
      unsigned char byte = bitmap_buf[byte_index]; 

      for (unsigned char bit_index = 0; bit_index < 8; ++bit_index) {

        if (!(byte & (1 << bit_index))) {// found available block
          bitmap_buf[byte_index] |= 1 << bit_index;
          write_block(bitmap, bitmap_buf);
         
          blockid_t id = block + byte_index * 8 + bit_index;
          printf("\t\tbm: alloc block %d\n", id);
          return id;
        }
      }
    }
  }
  
  return DBLOCK_NUM;

}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */

  printf("\t\tbm: free_block %d\n", id);

  std::lock_guard<std::mutex> lock(mtx_);
  if (id >= DBLOCK_NUM) return;

  char bitmap_buf[BLOCK_SIZE];
  uint32_t byte_index = (id % BPB) / 8;
  unsigned char bit_index = (id % BPB) % 8;

  blockid_t bitmap = BBLOCK(id);
  read_block(bitmap, bitmap_buf);
  
  if (!(bitmap_buf[byte_index] & (1 << bit_index))) return;

  bitmap_buf[byte_index] &= ~(1 << bit_index);
  write_block(bitmap, bitmap_buf);

}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * DBLOCK_NUM;
  sb.nblocks = DBLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::attach_version_control(version_control *vc) {
  this->vc = vc;
}

void
block_manager::read_block(uint32_t id, char *buf)
{
  printf("\t\tbm: read_block %d\n", id);
  read_block_internal_(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  printf("\t\tbm: write_block %d\n", id);

  char old[DBLOCK_NUM];
  
  read_block_internal_(id, old);
  vc->add_entry(log_entry (id, old, buf));

  write_block_internal_(id, buf);
}

void
block_manager::write_block_internal_(uint32_t id, const char *buf) {
  d->write_block(id, buf);
}

void block_manager::read_block_internal_(uint32_t id, char *buf) {
  d->read_block(id, buf);
}


// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  vc = new version_control(bm);
  bm->attach_version_control(vc);

  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  vc->commit();

  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */

  std::lock_guard<std::mutex> lock(mtx_);
  
  char buf[BLOCK_SIZE];

  for (blockid_t block = IBLOCK(1, bm->sb.nblocks); block < INODE_NUM / IPB; ++block) {
  bm->read_block(block, buf);
  
    for (int inode_index = 0; inode_index < IPB; ++inode_index) {
      struct inode *ino = (struct inode*) buf + inode_index % IPB;
      if (ino->type) continue;
        
      uint32_t inum = (block - IBLOCK(1, bm->sb.nblocks)) * IPB + inode_index + 1;
      ino->type = type;
      ino->size = 0;
      ino->mtime = ino->ctime = ino->atime = std::time(0);
      
      printf("\tim: alloc_inode %d\n", inum);
      
      put_inode(inum, ino);

      return inum;
    }
  }
 
  return 0;
}


void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */

  printf("\tim: put_inode %d\n", inum);

  std::lock_guard<std::mutex> lock(mtx_);
  if (inum >= INODE_NUM) return;

  struct inode *ino = get_inode(inum);
  if (!ino->type) return;
  
  ino->type = 0;
  put_inode(inum, ino);
  free(ino);
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;

  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */
  if (inum >= INODE_NUM) return;

  struct inode *ino = get_inode(inum);
  *size = ino->size;
  
  int i;
  int nblocks = bm->get_nblocks(ino->size); 
  *buf_out = (char *)malloc(nblocks * BLOCK_SIZE);

  for (i = 0; i < MIN(NDIRECT, nblocks); i++) {
    blockid_t id = ino->blocks[i];
    bm->read_block(id, *buf_out + i * BLOCK_SIZE);
  }

  if (i < nblocks) {

    blockid_t indirect_blocks[NINDIRECT];
    bm->read_block(ino->blocks[NDIRECT], (char *)indirect_blocks);

    for (; i < nblocks; ++i) {
      blockid_t id = indirect_blocks[i - NDIRECT];
      bm->read_block(id, *buf_out + i * BLOCK_SIZE);
    }

  }
  
  ino->atime = std::time(0);
  put_inode(inum, ino);

  free(ino);
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */

  printf("\tim: write_file %d, size %d\n", inum, size);

  if (inum >= INODE_NUM) return;
  
  int nblocks = bm->get_nblocks(size);

  if (nblocks > MAXFILE) return;

  struct inode *inode = get_inode(inum);
  
  int old_nblocks = bm->get_nblocks(inode->size);

  inode->size = size;

  blockid_t indirect_blocks[NINDIRECT];
  
  if (old_nblocks > NDIRECT) {
        bm->read_block(inode->blocks[NDIRECT], (char *)indirect_blocks);
  }
  if (nblocks > old_nblocks) {
    // grow
    int i = old_nblocks;
    if (old_nblocks < NDIRECT) {
      for (; i < MIN(NDIRECT, nblocks); ++i) {
        inode->blocks[i] = bm->alloc_block();
      }
    }
    if (i < nblocks) {
      
      // need to alloc indirect blocks
      if (old_nblocks <= NDIRECT) {
        inode->blocks[NDIRECT] = bm->alloc_block();
      } else {
        i = old_nblocks; // ASSERT old_nblocks > DIRECT
      }

      for (; i < nblocks; ++i) {
        indirect_blocks[i - NDIRECT] = bm->alloc_block();
      }

      bm->write_block(inode->blocks[NDIRECT], (const char *)indirect_blocks);
    }
  } else if (nblocks < old_nblocks) {
    // shrink
    int i;

    for (i = old_nblocks - 1; i >= MAX(NDIRECT, nblocks); --i) {
      blockid_t id = indirect_blocks[i - NDIRECT];
      bm->free_block(id);
    }
    
    if (i >= nblocks) {
      if (old_nblocks > NDIRECT)  
        bm->free_block(inode->blocks[NDIRECT]);
      for (; i >= nblocks; --i) {
        blockid_t id = inode->blocks[i];
        bm->free_block(id);
      }
    }
  }

  // copy data
  int i = 0;
  char write_buf[BLOCK_SIZE];

  for (; i < MIN(nblocks, NDIRECT); ++i) {
    blockid_t id = inode->blocks[i];
    memset(write_buf, 0, BLOCK_SIZE);
    auto len = MIN(BLOCK_SIZE, size - i * BLOCK_SIZE);
    memcpy(write_buf, buf + i * BLOCK_SIZE, len);
    bm->write_block(id, write_buf);
  }

  for (; i < nblocks; ++i) {
    blockid_t id = indirect_blocks[i - NDIRECT];
    memset(write_buf, 0, BLOCK_SIZE);
    auto len = MIN(BLOCK_SIZE, size - i * BLOCK_SIZE);
    memcpy(write_buf, buf + i * BLOCK_SIZE, len);
    bm->write_block(id, write_buf);
  }  
    
  inode->ctime = inode->mtime = inode->atime = std::time(0);
  put_inode(inum, inode);

  free(inode);
  
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  
  if (inum >= INODE_NUM) return;
  
  struct inode* ino = get_inode(inum);

  if (!ino) {
    a.type = 0;
    return;
  } 

  a.type = ino->type;
  a.atime = ino->atime;
  a.mtime = ino->mtime;
  a.ctime = ino->ctime;
  a.size = ino->size;

  free(ino);
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   */

  printf("\tim: remove_file %d\n", inum);

  if (inum >= INODE_NUM) return;

  struct inode *ino = get_inode(inum);

  int nblocks = bm->get_nblocks(ino->size);

  int i;
  for (i = 0; i < MIN(nblocks, NDIRECT); ++i) {
    bm->free_block(ino->blocks[i]);
  }

  if (i < nblocks) {

    blockid_t indirect_blocks[NINDIRECT];
    bm->read_block(ino->blocks[NDIRECT], (char *)indirect_blocks);

    for (; i < nblocks; ++i) {
      blockid_t id = indirect_blocks[i - NDIRECT];
      bm->free_block(id);
    }
  }

  free_inode(inum);
}

void
inode_manager::commit() {
  vc->commit(); 
}

void
inode_manager::redo() {
  vc->redo();
}

void inode_manager::rollback() {
  vc->rollback();
}


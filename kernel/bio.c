// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache[BCACHE_PRIME];

void
binit(void)
{
//    printf("%d %d\n",1,ticks);
  struct buf *b;

  for(int i=0;i<BCACHE_PRIME;i++){

    initlock(&bcache[i].lock, "bcache");

    // Create linked list of buffers
//    bcache[i].head.prev = &bcache[i].head;
//    bcache[i].head.next = &bcache[i].head;
    for(b = bcache[i].buf; b < bcache[i].buf+NBUF; b++){
//        b->next = bcache[i].head.next;
//        b->prev = &bcache[i].head;
        b->LRU = ticks;
        b->HASH = i;
        initsleeplock(&b->lock, "buffer");
//        bcache[i].head.next->prev = b;
//        bcache[i].head.next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
//    printf("%d %d\n",1,ticks);
  struct buf *b;
  // calc hash of (dev,blockno)
  int i= blockno%BCACHE_PRIME;
  acquire(&bcache[i].lock);

  int LRU_ticks = ticks;
  struct buf* LRU_ache = 0;
  // Is the block already cached?
  for(b = bcache[i].buf; b < bcache[i].buf+NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->LRU = ticks;
      release(&bcache[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
    else if(b->refcnt == 0){
        if(b->LRU <= LRU_ticks){
            LRU_ticks = b->LRU;
            LRU_ache = b;
        }
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  if (LRU_ache != 0){
      LRU_ache->dev = dev;
      LRU_ache->blockno = blockno;
      LRU_ache->valid = 0;
      LRU_ache->refcnt = 1;
      LRU_ache->LRU = ticks;
      release(&bcache[i].lock);
      acquiresleep(&LRU_ache->lock);
      return LRU_ache;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
//    printf("%d %d\n",1,ticks);
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
//    printf("%d %d\n",1,ticks);
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
//    printf("%d %d\n",1,ticks);
  if(!holdingsleep(&b->lock))
    panic("brelse");
  int i= b->HASH;
  releasesleep(&b->lock);

  acquire(&bcache[i].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
//    b->next->prev = b->prev;
//    b->prev->next = b->next;
//    b->next = bcache.head.next;
//    b->prev = &bcache.head;
//    bcache.head.next->prev = b;
//    bcache.head.next = b;
  }
  release(&bcache[i].lock);
}

void
bpin(struct buf *b) {
//    printf("%d %d\n",1,ticks);
  int i= b->HASH;
  acquire(&bcache[i].lock);
  b->refcnt++;
  release(&bcache[i].lock);
}

void
bunpin(struct buf *b) {
//    printf("%d %d\n",1,ticks);
  int i= b->HASH;
  acquire(&bcache[i].lock);
  b->refcnt--;
  release(&bcache[i].lock);
}



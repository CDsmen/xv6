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

#define HSIZE 13

uint64 HASH(uint x,uint y){
  // return (((uint64)x) * 131 + y) % HSIZE;
  return y % HSIZE;
}

struct HB
{
  struct buf head;
  struct spinlock lock;
};

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];
  struct HB hash[HSIZE];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void change(struct buf *b, struct buf *head)
{
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = head->next;
  b->prev = head;
  head->next->prev = b;
  head->next = b;
}

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for (int i = 0; i < HSIZE;i++){
    initlock(&bcache.hash[i].lock, "bcache");
    bcache.hash[i].head.prev = &bcache.hash[i].head;
    bcache.hash[i].head.next = &bcache.hash[i].head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hash[0].head.next;
    b->prev = &bcache.hash[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hash[0].head.next->prev = b;
    bcache.hash[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);
  uint64 indx = HASH(dev, blockno);
  acquire(&bcache.hash[indx].lock);

  // Is the block already cached?
  for (b = bcache.hash[indx].head.next; b != &bcache.hash[indx].head; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      acquire(&tickslock);
      b->tick = ticks;
      release(&tickslock);
      release(&bcache.hash[indx].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // acquire(&bcache.lock);
  for (int i = indx, num = 0; num < HSIZE; num++, i++)
  {
    if (i >= HSIZE)
      i = 0;
    if(i!=indx){
      if(!holding(&bcache.hash[i].lock))
        acquire(&bcache.hash[i].lock);
      else
        continue;
    }

    struct buf *tmp;
    for (b = bcache.hash[i].head.next; b != &bcache.hash[i].head; b = b->next)
    {
      if(b->refcnt == 0 && (tmp == 0 || b->tick < tmp->tick)){
        tmp = b;
      }
    }
    if(tmp){
      b = tmp;
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      acquire(&tickslock);
      b->tick = ticks;
      release(&tickslock);
      if (i != indx)
      {
        change(b, &bcache.hash[indx].head);
        release(&bcache.hash[i].lock);
      }
      release(&bcache.hash[indx].lock);
      acquiresleep(&b->lock);
      return b;
    }else{
      if(i!=indx)
        release(&bcache.hash[i].lock);
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
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
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint64 indx = HASH(b->dev, b->blockno);
  acquire(&bcache.hash[indx].lock);
  b->refcnt--;
  acquire(&tickslock);
  b->tick = ticks;
  release(&tickslock);
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   acquire(&bcache.lock);
  //   change(b, &bcache.head);
  //   release(&bcache.lock);
  // }

  release(&bcache.hash[indx].lock);
}

void
bpin(struct buf *b) {
  uint64 indx = HASH(b->dev, b->blockno);
  acquire(&bcache.hash[indx].lock);
  b->refcnt++;
  release(&bcache.hash[indx].lock);
}

void
bunpin(struct buf *b) {
  uint64 indx = HASH(b->dev, b->blockno);
  acquire(&bcache.hash[indx].lock);
  b->refcnt--;
  // if (b->refcnt == 0)
  // {
  //   // no one is waiting for it.
  //   acquire(&bcache.lock);
  //   change(b, &bcache.head);
  //   release(&bcache.lock);
  // }
  release(&bcache.hash[indx].lock);
}



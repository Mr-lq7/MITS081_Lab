// // Buffer cache.
// //
// // The buffer cache is a linked list of buf structures holding
// // cached copies of disk block contents.  Caching disk blocks
// // in memory reduces the number of disk reads and also provides
// // a synchronization point for disk blocks used by multiple processes.
// //
// // Interface:
// // * To get a buffer for a particular disk block, call bread.
// // * After changing buffer data, call bwrite to write it to disk.
// // * When done with the buffer, call brelse.
// // * Do not use the buffer after calling brelse.
// // * Only one process at a time can use a buffer,
// //     so do not keep them longer than necessary.


// #include "types.h"
// #include "param.h"
// #include "spinlock.h"
// #include "sleeplock.h"
// #include "riscv.h"
// #include "defs.h"
// #include "fs.h"
// #include "buf.h"



// // 删除了head
// struct {
//   struct spinlock lock;
//   struct buf buf[NBUF];

//   // Linked list of all buffers, through prev/next.
//   // Sorted by how recently the buffer was used.
//   // head.next is most recent, head.prev is least.
// } bcache;


// //改进后的结构
// struct {
//   struct spinlock lock;
//   struct buf head;
// } bcaches[NBUC];

// extern uint ticks; //系统的cpu时间戳

// void
// binit(void)
// {
//   struct buf *b;
//   initlock(&bcache.lock, "bcache");

//   for(int i = 0; i < NBUC; i++)
//   {
//     initlock(&bcaches[i].lock, "bcache.bucket"); 
//     bcaches[i].head.prev = &bcaches[i].head;
//     bcaches[i].head.next = &bcaches[i].head;
//   }

//   for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
//     b->tick = -1;
//     initsleeplock(&b->lock, "buffer");
//   }

// }

// // Look through buffer cache for block on device dev.
// // If not found, allocate a buffer.
// // In either case, return locked buffer.

// //设备要用到缓存

// static struct buf*
// bget(uint dev, uint blockno)
// {
//   struct buf *b;
//   struct buf* lru; //需要被换出的buf

//   int id = blockno % NBUC;

//   acquire(&bcaches[id].lock);

//   // Is the block already cached?
//   for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->tick = ticks; //LRU算法淘汰首先先淘汰tick小的
//       b->refcnt++;
//       release(&bcaches[id].lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }

//   // Not cached.
//   // Recycle the least recently used (LRU) unused buffer.
//   //未缓存
//   //在当前链表里找到一个未使用块，缓存在这里 
  
//   // 未缓存,先在自己的桶中寻找未被缓存的buf
//   lru = 0;
//   for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
//     if(b->refcnt == 0) {
//       if (lru == 0) {
//         lru = b;
//       } else {
//         if (lru->tick > b->tick) lru = b; //lru优先替换掉小的时间戳的块
//       }
//     }
//   }
//   if (lru) {
//     lru->dev = dev;
//     lru->blockno = blockno;
//     lru->valid = 0;
//     lru->refcnt = 1;
//     lru->tick = ticks;
//     release(&bcaches[id].lock);
//     acquiresleep(&lru->lock);
//     return lru;
//   }


  
//   //去其他地方偷,先遍历整个bcache,再找其他桶
//   int oid;
//   int flag = 1;
//   while (flag) {
//     flag = 0;
//     lru = 0;
//     for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
//       if (b->refcnt == 0) {
//         flag = 1;
//         if (lru == 0) {
//           lru = b;
//         } else {
//           if (lru->tick > b->tick) lru = b;  
//         }
//       }
//     }
//     if (lru) { //还没被分配到桶中
//       if (lru->tick == -1) {
//         acquire(&bcache.lock);
//         if (lru->refcnt == 0) {
//           lru->dev = dev;
//           lru->blockno = blockno;
//           lru->valid = 0;
//           lru->refcnt = 1;
//           lru->tick = ticks;

//           lru->next = bcaches[id].head.next;
//           lru->prev = &bcaches[id].head;
//           bcaches[id].head.next->prev = lru;
//           bcaches[id].head.next = lru;

//           release(&bcache.lock);
//           release(&bcaches[id].lock);
//           acquiresleep(&lru->lock);

//         } else {
//           release(&bcache.lock);
//         }
//       } else { //已经被分配到其他桶中
//         oid = (lru->blockno) % NBUC;
//         acquire(&bcaches[oid].lock);
//         if (lru->refcnt == 0) {
//           lru->dev = dev;
//           lru->blockno = blockno;
//           lru->valid = 0;
//           lru->refcnt = 1;
//           lru->tick = ticks;

//           lru->next->prev = lru->prev;
//           lru->prev->next = lru->next;
//           lru->next = bcaches[oid].head.next;
//           lru->prev = &bcaches[oid].head;
//           bcaches[oid].head.next->prev = lru;
//           bcaches[oid].head.next = lru;

//           release(&bcaches[oid].lock);
//           release(&bcaches[oid].lock);
//           acquiresleep(&lru->lock);
//           return lru;
//         } else {
//           release(&bcaches[oid].lock);
//         }
//       }
//     }
//   }

//   panic("bget: no buffers");

// }

// // Return a locked buf with the contents of the indicated block.
// struct buf*
// bread(uint dev, uint blockno)
// {
//   struct buf *b;

//   b = bget(dev, blockno);
//   if(!b->valid) {
//     virtio_disk_rw(b, 0);
//     b->valid = 1;
//   }
//   return b;
// }

// // Write b's contents to disk.  Must be locked.
// void
// bwrite(struct buf *b)
// {
//   if(!holdingsleep(&b->lock))
//     panic("bwrite");
//   virtio_disk_rw(b, 1);
// }

// // Release a locked buffer.
// // Move to the head of the most-recently-used list.
// void //将refcnt=0的buf移动到链表头
// brelse(struct buf *b)
// {

//   if(!holdingsleep(&b->lock))
//     panic("brelse");

//   releasesleep(&b->lock);
//   int id = (b->blockno) % NBUC;

//   acquire(&bcaches[id].lock);
//   b->refcnt--;
//   release(&bcaches[id].lock);
// }

// void
// bpin(struct buf *b) {

//   int id=(b->blockno) % NBUC;
//   acquire(&bcaches[id].lock);
//   b->refcnt++;
//   release(&bcaches[id].lock);
// }

// void
// bunpin(struct buf *b) {
//   int id=(b->blockno) % NBUC;
//   acquire(&bcaches[id].lock);
//   b->refcnt--;
//   release(&bcaches[id].lock);
// }



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
  struct buf head;
} bcaches[NBUC];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

extern uint ticks;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUC; i++){
    initlock(&bcaches[i].lock, "bcache.bucket");
    bcaches[i].head.prev = &bcaches[i].head;
    bcaches[i].head.next = &bcaches[i].head;
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->tick = -1;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *lru; // Potential LRU.
  int id = blockno % NBUC;
  acquire(&bcaches[id].lock);
  
  // Is the block already cached?
  for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->tick = ticks;
      b->refcnt++;
      release(&bcaches[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Search in the bucket for LRU.
  lru = 0;
  for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
    if(b->refcnt == 0){
      if (lru == 0){
        lru = b;
      } else{
        if (lru->tick > b->tick) lru = b;
      }
    }
  }
  if (lru){
    lru->dev = dev;
    lru->blockno = blockno;
    lru->valid = 0;
    lru->refcnt = 1;
    lru->tick = ticks;
    release(&bcaches[id].lock);
    acquiresleep(&lru->lock);
    return lru;
  }

  // Recycle the least recently used (LRU) unused buffer.
  int oid;
  int flag = 1;
  while (flag){
    flag = 0;
    lru = 0;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++){
      if (b->refcnt == 0){ // With bcaches[no].lock held and the previous for loop passed, it can be said that this buffer won't be in bucket bcaches[no].
        flag = 1;
        if (lru == 0){
          lru = b;
        } else {
          if (lru->tick > b->tick) lru = b;
        }
      }
    }
    if (lru){
      if (lru->tick == -1){
        acquire(&bcache.lock);
        if (lru->refcnt == 0){ // Lucky! The buffer is still available.
          lru->dev = dev;
          lru->blockno = blockno;
          lru->valid = 0;
          lru->refcnt = 1;
          lru->tick = ticks;

          lru->next = bcaches[id].head.next;
          lru->prev = &bcaches[id].head;
          bcaches[id].head.next->prev = lru;
          bcaches[id].head.next = lru;

          release(&bcache.lock);
          release(&bcaches[id].lock);
          acquiresleep(&lru->lock);
          return lru;
        } else { // Unlucky! Go and search for another buffer.
          release(&bcache.lock);
        }
      } else {  // So this buffer is in some buffer else.
        oid = (lru->blockno) % NBUC;
        acquire(&bcaches[oid].lock);
        if (lru->refcnt == 0){ // Lucky! The buffer is still available.
          lru->dev = dev;
          lru->blockno = blockno;
          lru->valid = 0;
          lru->refcnt = 1;
          lru->tick = ticks;

          lru->next->prev = lru->prev;
          lru->prev->next = lru->next;
          lru->next = bcaches[id].head.next;
          lru->prev = &bcaches[id].head;
          bcaches[id].head.next->prev = lru;
          bcaches[id].head.next = lru;

          release(&bcaches[oid].lock);
          release(&bcaches[id].lock);
          acquiresleep(&lru->lock);
          return lru;
        } else { // Unlucky! Go and search for another buffer.
          release(&bcaches[oid].lock);
        }
      }
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

  int id = (b->blockno) % NBUC;

  acquire(&bcaches[id].lock);
  b->refcnt--;
  release(&bcaches[id].lock);
}

void
bpin(struct buf *b) { 
  int id = (b->blockno) % NBUC;

  acquire(&bcaches[id].lock);
  b->refcnt++;
  release(&bcaches[id].lock);
}

void
bunpin(struct buf *b) { 
  int id = (b->blockno)%NBUC;

  acquire(&bcaches[id].lock);
  b->refcnt--;
  release(&bcaches[id].lock);
}
#### risc-v的isa

> 需要注意的是pc寄存器保存的是**当前运行代码的地址**而不是下一条
>

#### gdb调试常用指令

> ```bash
>make CPUS=1 qemu-gdb
> 
> gdb-multiarch
> 
> 调试指定可执行文件：file 可执行文件
> 
> layout split #查看源代码和汇编代码
> ni #单步执行汇编（不进函数）
> si #单步执行汇编（进入函数）
> n #单步执行源码（不进函数）
> s #单步执行源码（进入函数）
> 
> p $a0 #打印寄存器a0的值
> i r a0 #查看寄存器a0的值
> p/x 1536 #以16进制打印1536
> x/i 0x630 #查看0x630地址处的指令
> x /x addr #查看addr地址处的值
> 
> b *addr：使断点停到address地址处
> ```
> 

### Lab:  locks

链接：[Lab: locks (mit.edu)](https://pdos.csail.mit.edu/6.S081/2021/labs/lock.html)

> 需要注意的是我们做的实验是2021的，因此要找到2021的指导书
>
> 前期准备
>
> ```bash
> git fetch
> git checkout net //在进行这句话的时候确保实验一已经commit
> make clean
> ```

多个cpu对于同一个大锁的争用将会导致很差的并行性。改善就是可以通过每个CPU一个锁来解决。

这个实验需要改善并行性。改善并行性需要更改数据结构和锁定策略来减少竞争。需要为xv6内存分配和块缓存。

#### Memory allocator(moderate)

首先我们来看下实验要求和提示信息：

![image-20221104204232137](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221104204232137.png)

![image-20221104204239009](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221104204239009.png)

> - 实验要求：实现每个cpu的空闲列表，并在CPU的空闲列表为空时进行窃取。必须为这些锁以"kmem"的名称开头命名。也就是应该为每个锁调用initlock，并传递一个以"kmem"开头的名称。然后运行kalloctest用来查看实现是否减少了锁的竞争。通过运行usertests sbrkmuch来检查它是否仍然可以分配所有内存。需要确保usertests的所有测试通过，然后make grade也需要通过
>
> - 实验提示
>
>   - 可以使用kernel/param.h中的常量NCPU
>   - 让freerange将所有空闲内存分配给运行freerange的CPU
>   - 函数cpuid返回当前CPU核的编号，但只有在中断关闭时调用它并使用它的结果才是安全的。应该使用push_off()和pop_off()来关闭和打开终端
>   - 查看kernel/sprintf.c，了解字符串格式的想法。将所有锁命名为"kmem"是可以的
>
> - 解决方案：修改kernel/kalloc.c文件
>
>   ```c
>   typedef struct { //修改
>     struct spinlock lock;
>     struct run *freelist;
>   } kmem;
>   
>   kmem kmems[NCPU]; //新加
>   
>   void
>   kinit() //改动
>   {
>     for (int i = 0; i < NCPU; i++) {
>       initlock(&kmem.lock, "kmem");
>     }
>     freerange(end, (void*)PHYSTOP);
>   }
>   
>   void
>   kfree(void *pa) //改动
>   {
>     struct run *r;
>   
>     if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
>       panic("kfree");
>   
>     // Fill with junk to catch dangling refs.
>     memset(pa, 1, PGSIZE);
>   
>     r = (struct run*)pa;
>   
>     int cpu_id;
>     push_off();
>     cpu_id = cpuid();
>   
>   
>     acquire(&kmem[cpu_id].lock);
>     r->next = kmem.freelist;
>     kmem.freelist = r;
>     release(&kmem[cpu_id].lock);
>     pop_off();
>   
>   }
>   
>   void *
>   kalloc(void) //新加
>   {
>     struct run *r;
>   
>     int cpu_id;
>     push_off();
>     cpu_id=cpuid();
>     
>     
>     acquire(&(kmems[cpu_id].lock));
>     r = kmems[cpu_id].freelist;
>     if(r)
>     {
>       //当前cpu的freelist不为空
>       kmems[cpu_id].freelist = r->next;
>     }
>     else
>     {
>       //当前cpu的freelist为空
>       //遍历其他所有cpu的freelist
>       //int steal_num=32;       
>       for(int steal_id=0;steal_id<NCPU;steal_id++)
>       {
>         if(steal_id==cpu_id)
>         {
>           continue;
>         }     
>         acquire(&(kmems[steal_id].lock));
>         r=kmems[steal_id].freelist;                 
>         if(r)
>         {
>           //找到一个其他cpu的freelist不为空，从其表头拿1个节点
>           
>           kmems[steal_id].freelist = r->next; 
>           release(&(kmems[steal_id].lock));
>           break;
>         }
>         release(&(kmems[steal_id].lock));
>       }         
>     }
>     release(&(kmems[cpu_id].lock));
>     pop_off(); 
>   
>     if(r)
>       memset((char*)r, 5, PGSIZE); // fill with junk
>     return (void*)r;
>   }
>   
>   ```
>
> - 结果
>
>   ```bash
>   make qemu
>   kalloctest
>   usertests #看到都ok即可
>   ```
>
>   ![image-20221104213822117](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221104213822117.png)

---

#### Memory allocator总结

1. kalloc()只有一个单一的空闲列表由一个锁维护。因此出现多核cpu的时候，容易队这个锁产生争夺。

![image-20221107101939392](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221107101939392.png)

2. 改进的思路如图下所示。为每个cpu维护一个空闲列表，每个列表都有自己的锁，因为每个cpu将队不同的列表进行操作，因此不同cpu的分配和释放可以并行运行。



![image-20221107102041017](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221107102041017.png)

---



#### Buffer cache(hard)

如果多个进程激烈使用文件系统，它们可能会争夺bcache.lock。该锁保护磁盘块缓存（kernel/bio.c）。

实验要求：（实现对xv6原有的buffer cache的优化，以减少多个进程访问buffer cache时锁碰撞的频率）

![image-20221105111634572](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221105111634572.png)

实验提示：

![image-20221105111710755](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221105111710755.png)

> - 实验要求：**修改块缓存（block cache）**使得当运行bcachetest程序时bcache所有锁的acquire循环迭代次数接近0。理想情况下，块缓存中所有锁的计数总和应该为0，如果总和小于500也是可以的。修改bget和brelse以便减少对bcache的不同块的并发查找和释放的锁的冲突（例如不需要都等待bcache.lock）。需要保持每个块最多缓存一个副本。
>
> - 减少块状缓存中的争用比kalloc更棘手，因为bcache缓冲区确实是在进程（也就是CPU）之间共享的。对于kalloc来说，我们可以通过给每个CPU提供自己的分配器来消除大部分的争用，但这对块状缓存来说是行不通的。我们建议你用一个哈希表来查询缓存中的区块号码，每个哈希桶有一个锁。
>
> - 实验提示：
>
>   - 查看8.1-8.3中块缓存的描述
>   - 使用固定数量的桶，不动态调整哈希表的大小是可以的。使用一个质数（例如13）的桶来减少散列冲突的可能性
>   - 在哈希表中搜索一个缓冲区，并在没有找到缓冲区时为其分配一个条目，这必须是原子性的
>   - 移除所有缓冲区的列表（bcache.head等），取而代之的是使用最后一次使用缓冲区的时间戳（kernel/trap.c的ticks）。有了这个改变brelse就不需要获取bcache锁，bget可以根据时间戳选择最近使用的最少的块。
>   - 在bget中序列化替换是可以的（即bget中选择缓冲区的部分，当在缓存中查找失误时可以重新使用）
>   - 在某些情况下，你的解决方案可能需要持有两个锁。例如在替换过程中，你可能需要持有bcache锁和每个桶的锁，确保能避免死锁
>   - 当替换一个区块时，你可能会将一个结构缓冲区从一个桶移到另一个桶，因为新区块哈希到不同的桶。你可能遇到一个棘手的情况：新的块可能哈希到与旧的块相同的桶。请确保在这种情况下避免死锁。
>   - 一些调试技巧：实现桶锁，但在bget的开始/结束时留下全局的bcache.lock acquire/release，以使代码序列化。可以用make CPUS=1 qemu来用一个核心进行测试。
>

---

首先需要了解一下xv6中原来的buffer cache的结构。主要可以通过kernel/bio.c和kernel/buf.h中查看。

根据kernel/bio.c可以看到bcache的struct的定义以及binit初始化函数，同时struct buf的定义在kernel/buf.h中查看。

通过阅读代码，我们可以大致画出原来的结构图：

![image-20221107110614199](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221107110614199.png)

bcache的结构体的buf是一个存储buffer的数组，其容量NBUF就是buffer cache预设的缓存内buffer总量。其中，**需要注意的是**head.next是最近访问过的buffer，而head.prev则是缓存中最久没访问过的buffer。bcache的核心在于**bget函数的实现**。观察bget的代码可以发现在bget函数开始时便由bcache.lock锁上，在return前才进行解锁。锁的粒度太大了，bget全程都会被锁上，这也是题目要求优化的问题。

通过观察kernel/bio.c中的代码我们也可以知道当buffer的refcnt被减少到0时，则没有任何调用该buffer的设备，它处于空闲状态。并且因为它刚被其他设备用完，因此呢，需要将该buf移到链表的最前面。因此需要对双向链表进行操作。

解决方案结构图：

![image-20221107114413211](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221107114413211.png)

下图只是展示了最清晰的一种结果，事实上相同颜色的buffer不一定处在相邻位置，但是它们彼此之间通过链表来进行相连。

解决方案：

1. NBUC是哈希桶的数量，每个哈希桶自己维护一个双向链表，正如前面所述
2. 在struct buf中引入tick是根据提示，便于利用lru策略进行替换
3. 需要重点关注bget函数。总的说来可以分为以下几步：
   - 先查看是否被cache，如果被cache将非常好办
   - 如果未被cache，则先在自己的哈希桶中进行查找refcnt==0的buf，并引入lru。先寻找自己的桶，如果这步中没找到，那么又会出现两种可能。一是buf未入桶，需要在bcache中进行查找；二是buf入了其他的桶，需要在bcaches[otherid]中查找
   - 在bcache中查找，找到就替换，将找到的块插入到bcaches[id]的头中
   - 在bcaches[otherid]中查找，找到就替换，将找到的块插入到bcaches[id]的头中
   - 以上步骤根据时间戳引入lru策略
4. 因为引入了时间戳，brelse就无需进行链表移动了
5. bpin和bunpin修改为各自哈希桶维护的双向链表即可

> - param.h
>
>   ```c
>   #define NBUC    13 //最好是质数
>   ```
>
> - kernel/buf.h
>
>   ```c
>   struct buf {
>     int valid;   // has data been read from disk?
>     int disk;    // does disk "own" buf?
>     uint dev;
>     uint blockno;
>     struct sleeplock lock;
>     uint refcnt;
>     struct buf *prev; // LRU cache list
>     struct buf *next;
>     uchar data[BSIZE];
>   
>     int tick; //时间戳
>   };
>   
>   
>   ```
>
>   
>
> - kernel/bio.c
>
>   ```c
>   struct {
>     struct spinlock lock;
>     struct buf head;
>   } bcaches[NBUC];
>   
>   struct {
>     struct spinlock lock;
>     struct buf buf[NBUF];
>   } bcache;
>   
>   
>   void
>   binit(void)
>   {
>     struct buf *b;
>   
>     initlock(&bcache.lock, "bcache");
>   
>     for (int i = 0; i < NBUC; i++){
>       initlock(&bcaches[i].lock, "bcache.bucket");
>       bcaches[i].head.prev = &bcaches[i].head;
>       bcaches[i].head.next = &bcaches[i].head;
>     }
>     
>     for(b = bcache.buf; b < bcache.buf+NBUF; b++){
>       b->tick = -1;
>       initsleeplock(&b->lock, "buffer");
>     }
>   }
>   
>   
>   
>   //重点！！！
>   // Look through buffer cache for block on device dev.
>   // If not found, allocate a buffer.
>   // In either case, return locked buffer.
>   static struct buf*
>   bget(uint dev, uint blockno)
>   {
>     struct buf *b;
>     struct buf *lru; // Potential LRU.
>     int id = blockno % NBUC;
>     acquire(&bcaches[id].lock);
>     
>     // Is the block already cached?
>     for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
>       if(b->dev == dev && b->blockno == blockno){
>         b->tick = ticks;
>         b->refcnt++;
>         release(&bcaches[id].lock);
>         acquiresleep(&b->lock);
>         return b;
>       }
>     }
>   
>     // Search in the bucket for LRU.
>     lru = 0;
>     for(b = bcaches[id].head.next; b != &bcaches[id].head; b = b->next){
>       if(b->refcnt == 0){
>         if (lru == 0){
>           lru = b;
>         } else{
>           if (lru->tick > b->tick) lru = b;
>         }
>       }
>     }
>     if (lru){
>       lru->dev = dev;
>       lru->blockno = blockno;
>       lru->valid = 0;
>       lru->refcnt = 1;
>       lru->tick = ticks;
>       release(&bcaches[id].lock);
>       acquiresleep(&lru->lock);
>       return lru;
>     }
>   
>     // Recycle the least recently used (LRU) unused buffer.
>     int oid;
>     int flag = 1;
>     while (flag){
>       flag = 0;
>       lru = 0;
>       for (b = bcache.buf; b < bcache.buf + NBUF; b++){
>         if (b->refcnt == 0){ // With bcaches[no].lock held and the previous for loop passed, it can be said that this buffer won't be in bucket bcaches[no].
>           flag = 1;
>           if (lru == 0){
>             lru = b;
>           } else {
>             if (lru->tick > b->tick) lru = b;
>           }
>         }
>       }
>       if (lru){
>         if (lru->tick == -1){
>           acquire(&bcache.lock);
>           if (lru->refcnt == 0){ // Lucky! The buffer is still available.
>             lru->dev = dev;
>             lru->blockno = blockno;
>             lru->valid = 0;
>             lru->refcnt = 1;
>             lru->tick = ticks;
>   
>             lru->next = bcaches[id].head.next;
>             lru->prev = &bcaches[id].head;
>             bcaches[id].head.next->prev = lru;
>             bcaches[id].head.next = lru;
>   
>             release(&bcache.lock);
>             release(&bcaches[id].lock);
>             acquiresleep(&lru->lock);
>             return lru;
>           } else { // Unlucky! Go and search for another buffer.
>             release(&bcache.lock);
>           }
>         } else {  // So this buffer is in some buffer else.
>           oid = (lru->blockno) % NBUC;
>           acquire(&bcaches[oid].lock);
>           if (lru->refcnt == 0){ // Lucky! The buffer is still available.
>             lru->dev = dev;
>             lru->blockno = blockno;
>             lru->valid = 0;
>             lru->refcnt = 1;
>             lru->tick = ticks;
>   
>             lru->next->prev = lru->prev;
>             lru->prev->next = lru->next;
>             lru->next = bcaches[id].head.next;
>             lru->prev = &bcaches[id].head;
>             bcaches[id].head.next->prev = lru;
>             bcaches[id].head.next = lru;
>   
>             release(&bcaches[oid].lock);
>             release(&bcaches[id].lock);
>             acquiresleep(&lru->lock);
>             return lru;
>           } else { // Unlucky! Go and search for another buffer.
>             release(&bcaches[oid].lock);
>           }
>         }
>       }
>     }
>   
>     panic("bget: no buffers");
>   }
>   
>   
>   
>   // Release a locked buffer.
>   // Move to the head of the most-recently-used list.
>   //加入时间戳后不需要移动了
>   void
>   brelse(struct buf *b)
>   {
>     if(!holdingsleep(&b->lock))
>       panic("brelse");
>   
>     releasesleep(&b->lock);
>   
>     int id = (b->blockno) % NBUC;
>   
>     acquire(&bcaches[id].lock);
>     b->refcnt--;
>     release(&bcaches[id].lock);
>   }
>   
>   void
>   bpin(struct buf *b) { 
>     int id = (b->blockno) % NBUC;
>   
>     acquire(&bcaches[id].lock);
>     b->refcnt++;
>     release(&bcaches[id].lock);
>   }
>   
>   void
>   bunpin(struct buf *b) { 
>     int id = (b->blockno)%NBUC;
>   
>     acquire(&bcaches[id].lock);
>     b->refcnt--;
>     release(&bcaches[id].lock);
>   }
>   ```
>
> - 结果
>
>   ```
>   make qemu
>   bcachetest
>   ```
>
>   ![image-20221108114238008](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221108114238008.png)

整个lab进行测试，linux终端中

> - 在xv6-labs-2021文件夹下新建time.txt，填入数字
>
> - ./grade-lab-lock
>
>   ![image-20221108114341596](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221108114341596.png)

#### 总结

> - 该实验的第一部分Memory allocator是为了减少锁的争用，因为抢夺大锁会导致时间占用，从而无法达到比较高的并发性。其主要思想是原来xv6是不同的cpu争抢一个大锁，谁先获得到大锁后其他cpu就得等待；而改进的思路是将大锁分成多个小锁，采取的是每个cpu分配一个锁，如果当前cpu的空闲列表为空（无法kalloc），那么就可以从其他cpu中偷分配一个到自己的空闲列表中。
> - 该实验的第二部分和第一部分不太一样。提示也已经给出了。减少block cache的争用比kalloc更棘手，因为bcache缓冲区确实是在进程（也就是CPU）之间共享的。对于kalloc来说，我们可以通过给每个CPU提供自己的分配器来消除大部分的竞争，**但这对block cache是行不通的**
> - 借鉴了网上很多方法，但test writebig还是通过不了。猜测是该题和后一个实验file system中Large files有一定的关系，应该是拓展支持Large files后，才能过这个样例
> - 总的来说，该实验还是非常难的，需要好好琢磨和理解

#### 番外

> - 解决了test wirtebig: panic: balloc: out of blocks的问题了！
>
>   - 解决方式：增大XV6文件系统中最大的文件块数
>
>   - kernel/param.h
>
>     ```c
>     #define FSSIZE 20000
>     ```
>
> - 结果：
>
>   ![image-20221124142528291](D:\WorkSpace\Lab\MITS081\locks.assets\image-20221124142528291.png)
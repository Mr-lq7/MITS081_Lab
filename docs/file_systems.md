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

### Lab:  file system

链接：[Lab: file system (mit.edu)](https://pdos.csail.mit.edu/6.S081/2021/labs/fs.html)

> 需要注意的是我们做的实验是2021的，因此要找到2021的指导书
>
> 前期准备
>
> ```bash
> git fetch
> git checkout fs //在进行这句话的时候确保实验一已经commit
> make clean
> ```

该实验将向XV6文件系统添加大文件和符号链接。

#### Large files（moderate）

需要增加XV6文件的最大尺寸。当前，XV6文件限制为268个块，每块是1024个字节。XV6的inode包括12个直接块号和一个单独的块号（这是指一个块，该块最多可容纳256个块号）。总计12+256=268块。

需要修改XV6文件系统代码，以支持每个inode中的双向间接块。

![image-20221108155546858](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108155546858.png)

XV6将磁盘分为以下几个部分，如上图所示。



![image-20221108155529114](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108155529114.png)

一个文件在磁盘中的表示如上图所示，所以原来的文件块是12+256=268。经过修改后的文件块有11（直接块）+256（单直接块）+256*256（双直接块）。示意图如下所示：

![image-20221108162735561](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108162735561.png)

如下图所示

首先我们来看下实验要求和提示信息：

![image-20221108160758822](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108160758822.png)

![image-20221108160808445](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108160808445.png)

XV6文件系统里定义了两个版本的inode。一个是硬盘上面存储的版本struct dinode（kernel/fs.h）；另一个是内存里存储的版本struct inode，起到缓存的作用（kernel/file.h）。nlink是inode在文件系统里面的链接数，如果nlink为0，说明inode对应得文件已经被删除；ref是inode被进程引用的次数，如果ref为0就说明没有进程使用这个inode了，这时候操作系统就不用继续在内存中缓存它了，可以将它驱逐到硬盘。操作系统可以使用iupdate更新硬盘里面的inode，用iget增加一次ref，iput减少一次ref。

内存里的inode有一个睡眠锁，读写inode的时候需要使用ilock和iunlock加解锁。使用readi和writei可以读写inode管理的磁盘块，使用namei可以根据路径搜索inode，使用nameiparent可以搜索指定文件的父目录的inode。inode相当于记录了文件的元数据信息。

> - 实验要求：修改bmap()函数，使其除了直接块和单直接块之外，还能实现双直接块。你必须只有11个直接块，而不是12个，以便为你的双重直接块腾出空间；你不允许改变一个磁盘上的inode的大小。ip->addrs[]的前11个元素应该是直接块；弟12个元素应该是单直接块（就像当前的一样）；第13个应该是新的**双间接块**。当bigfile写入65803个块，并且usertests成功运行时，就完成了这个练习。
>
> - 实验提示
>
>   - 确保理解bmap()函数。确保理解为什么增加一个双重间接块会使最大文件大小增加256*256个块。
>   - 考虑一下如何用逻辑块号来索引双重间接块，以及它所指向的间接块。
>   - 如果你改变了NDIRECT的定义，你可能必须改变file.h中struct inode中addrs[]的声明。却表inode和dinode在其addrs[]数组中有相同数量的元素
>   - 如果你改变了NDIRECT的定义，确保创建一个新的fs.img，因为mkfs用NDIRECT来构建文件系统
>   - 如果你的文件系统进入了一个糟糕的状态，也许是因为崩溃，删除fs.img（在unix中做这个操作不是XV6中）
>   - 不要忘了在每一个bread()块上都要用brelse()
>   - 你应该只在需要时分配间接块和双重间接块，就像原来的bmap()一样
>   - 确保itrunc释放一个文件的所有块，包括双间接块。
>
> - 解决方案
>
>   - 修改inode的相关定义。kernel/fs.h
>
>     ```c
>     #define NDIRECT 11 //修改
>     
>     #define NINDIRECT (BSIZE / sizeof(uint))
>     
>     #define NDBINDIRECT (BSIZE / sizeof(uint)) * (BSIZE / sizeof(uint)) //新加256*256
>     
>     #define SDIRECT_PTR 11 //新加
>     #define DDIRECT_PTR 12 //新加
>     
>     #define MAXFILE (NDIRECT + NINDIRECT + NDBINDIRECT) //修改
>     
>     // On-disk inode structure
>     struct dinode {
>       short type;           // File type
>       short major;          // Major device number (T_DEVICE only)
>       short minor;          // Minor device number (T_DEVICE only)
>       short nlink;          // Number of links to inode in file system
>       uint size;            // Size of file (bytes)
>       uint addrs[NDIRECT+2];   // Data block addresses
>     };
>     
>     ```
>
>     我们将NDIRECT减为11，但是为了保证dinode的大小不变，所以addrs数组要+2。根据提示，在kernel/file.h中的inode中的addrs数组下标也得同步变化。
>
>   - 修改inode的相关定义。kernel/file.h
>
>     ```c
>     // in-memory copy of an inode
>     struct inode {
>       uint dev;           // Device number
>       uint inum;          // Inode number
>       int ref;            // Reference count
>       struct sleeplock lock; // protects everything below here
>       int valid;          // inode has been read from disk?
>     
>       short type;         // copy of disk inode
>       short major;
>       short minor;
>       short nlink;
>       uint size;
>       uint addrs[NDIRECT+2]; //修改
>     };
>     ```
>
>   - 修改bmap函数。bmap是返回inode ip中第n个块的磁盘块地址。如果位于direct pointer之内就直接访问；如果没有这样的块，bmap会分配一个；如果超出了direct的范围就把bn减去NDIRECT的偏移量，先访问addrs[11]指向的一级指针块，使用一级指针块里面的指针访问数据块；同理可以实现二级指针块。实现的时候参考原有写法即可
>
>     ```c
>     static uint
>     bmap(struct inode *ip, uint bn)
>     {
>       uint addr, *a;
>       struct buf *bp;
>     
>       if(bn < NDIRECT){
>         if((addr = ip->addrs[bn]) == 0)
>           ip->addrs[bn] = addr = balloc(ip->dev);
>         return addr;
>       }
>       bn -= NDIRECT;
>     
>       if(bn < NINDIRECT){
>         // Load indirect block, allocating if necessary.
>         if((addr = ip->addrs[NDIRECT]) == 0)
>           ip->addrs[NDIRECT] = addr = balloc(ip->dev);
>         bp = bread(ip->dev, addr);
>         a = (uint*)bp->data;
>         if((addr = a[bn]) == 0){
>           a[bn] = addr = balloc(ip->dev);
>           log_write(bp);
>         }
>         brelse(bp);
>         return addr;
>       }
>     
>     ////////////////////////////////////////
>       //新加
>       struct buf *bp1st;
>       struct buf *bp2nd;
>       uint *a1;
>       bn -= NINDIRECT;
>       if(bn < NDBINDIRECT){
>         // Load indirect block, allocating if necessary.
>         if((addr = ip->addrs[DDIRECT_PTR]) == 0)
>           ip->addrs[DDIRECT_PTR] = addr = balloc(ip->dev);
>     
>         bp1st = bread(ip->dev, addr);
>         a = (uint*)bp1st->data;
>         uint block1st = bn / NINDIRECT; //定位到在第一级中的索引表
>         if((addr = a[block1st]) == 0){
>           a[block1st] = addr = balloc(ip->dev);
>           log_write(bp1st);
>         }
>         brelse(bp1st);
>     
>         bp2nd = bread(ip->dev, addr);//读装满指针的二阶block
>         a1 = (uint*)bp2nd->data;
>         uint block2nd = bn % NINDIRECT;//访问二阶block的哪个指针
>         if((addr = a1[block2nd]) == 0)
>         {
>           a1[block2nd] = addr = balloc(ip->dev);
>           log_write(bp2nd);
>         }
>         
>         brelse(bp2nd);
>     
>         return addr;
>       }
>     
>       panic("bmap: out of range");
>     }
>     ```
>
>     
>
>   - 修改itrunc函数，释放磁盘块。也是参考原有的写法即可
>
>     ```c
>     // Truncate inode (discard contents).
>     // Caller must hold ip->lock.
>     void
>     itrunc(struct inode *ip)
>     {
>       int i, j;
>       struct buf *bp;
>       uint *a;
>     
>       for(i = 0; i < NDIRECT; i++){
>         if(ip->addrs[i]){
>           bfree(ip->dev, ip->addrs[i]);
>           ip->addrs[i] = 0;
>         }
>       }
>     
>       if(ip->addrs[NDIRECT]){
>         bp = bread(ip->dev, ip->addrs[NDIRECT]);
>         a = (uint*)bp->data;
>         for(j = 0; j < NINDIRECT; j++){
>           if(a[j])
>             bfree(ip->dev, a[j]);
>         }
>         brelse(bp);
>         bfree(ip->dev, ip->addrs[NDIRECT]);
>         ip->addrs[NDIRECT] = 0;
>       }
>     
>       struct buf *bp1, *bp2;
>       uint *a1;
>       
>       /////////释放所有二阶指针，参照原来的写法
>       if(ip->addrs[DDIRECT_PTR])
>       {
>         bp1 = bread(ip->dev, ip->addrs[DDIRECT_PTR]); //访问一阶指针块
>         a=(uint*)bp1->data;
>         for(j=0;j<NINDIRECT;j++)
>         {
>           if(a[j])
>           {
>             //如果发现了二级指针块
>             bp2 = bread(ip->dev, a[j]); //访问它
>             a1=(uint*)bp2->data;
>             for(int k=0;k<NINDIRECT;k++)
>             {
>               if(a1[k])//如果这个位置有数据块则释放
>                 bfree(ip->dev,a1[k]);
>             }
>             brelse(bp2);
>             bfree(ip->dev,a[j]);//释放这个二级指针块
>           }
>           //释放完了二阶指针块，
>         }
>         brelse(bp1); //释放一级指针块
>         bfree(ip->dev, ip->addrs[DDIRECT_PTR]);
>         ip->addrs[DDIRECT_PTR] = 0;
>       }
>     
>     
>       ip->size = 0;
>       iupdate(ip);
>     }
>     ```
>
>     
>
> - 结果
>
>   ```bash
>   make qemu
>   bigfile
>   usertests #看到都ok即可
>   ```
>
>   ![image-20221108214349157](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108214349157.png)
>   
>   ![image-20221108214400113](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108214400113.png)

#### Symbolic links(moderate)

在XV6中添加符号链接（软链接）。它是通过路径名指向被链接的文件，当一个符号链接被打开时，内核会跟随链接到被提及的文件。通过这个实验可以了解路径名查找的工作原理。

实验要求：

![image-20221108215440277](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108215440277.png)

实验提示：

![image-20221108215447914](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221108215447914.png)



> - 实验要求：实现symlink系统调用，它在路径上创建一个新的符号链接，指向由目标命名的文件。要在Makefile中添加symlinktest并运行它。
>
> - 实验提示：
>
>   - 首先，为symlink创建一个新的系统调用号，在user/usys.pl，user/user.h中添加一个条目，并在kernel/sysfile.c中实现一个空的sys_symlink
>   - 在kernel/stat.h中增加一个新的文件类型（T_SYMLINK），用来表示符号链接
>   - 在kernel/fcntl.h中增加一个新的标志（O_NOFOLLOW），可以和open系统调用一起使用。请注意，传递给open的标志是用位操作组合的，所以你的新标志不应该与任何现有标志重叠。把它添加到Makefile中将会编译user/symlinktest.c
>   - 实现symlink(target, path)系统调用，在path创建一个指向target的新符号链接。注意系统调用成功时，目标不需要存在。只需要选择一个地方来存储符号链接的目标路径。例如在inode的数据块中，symlink应该返回一个整数，代表成功（0）或失败（-1），类似于link和unlink。
>   - 修改open系统调用，处理路径指向符号链接的情况。如果该文件不存在，打开必须失败。当一个进程在打开的标志中指定O_NOFOLLOW时，open应该打开符号链接（而不是追踪它）。
>   - 如果被链接的文件也是一个符号链接，必须递归的跟踪它指导一个非链接文件。如果链接形成一个循环需要返回错误代码。也可以通过在链接的深度达到某个阈值（10）时返回错误代码来近似处理这个问题
>   - 其他系统调用（link和unlink）不能追踪符号链接。这些系统调用对符号链接本身进行操作
>   - 不需要处理目录的符号链接
>
> - 解决方案：根据提示逐步做
>
>   - kernel/syscall.c：添加系统调用号
>
>     ```c
>     #define SYS_symlink 22
>     ```
>
>   - 在user/usys.pl添加entry
>
>     ```c
>     entry("symlink");
>     ```
>
>   - 在user/user.h中添加函数原型
>
>     ```c
>     int symlink(char *target, char *path);
>     ```
>
>   - kernel/syscall.c
>
>     ```c
>     extern uint64 sys_symlink(void);
>     
>     [SYS_symlink] sys_symlink
>     ```
>
>     
>
>   - kernel/sysfile.c（待实现）
>
>     ```c
>     uint 64 sys_symlink(void) {
>     
>     }
>     ```
>
>   - kernel/stat.h
>
>     ```c
>     #define T_SYMLINK 4
>     ```
>
>   - kernel/fcntl.h
>
>     ```c
>     #define O_NOFOLLOW 0x800
>     ```
>
>   - 添加symlinktest到Makefile去
>
>   - 实现sys_symlink。将软连接本身也视为一个文件，文件用户数据块中存放的内容是另一文件的路径名的指向。symlink(char *target, char *path)：是在path建立target的软连接
>
>     ```c
>     uint64
>     sys_symlink(void) {
>     
>       char target[MAXPATH], path[MAXPATH];
>       if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0)
>         return -1;
>     
>       begin_op();
>     
>       struct inode* ip, *dp;
>       if ((ip = namei(target)) != 0) {
>         if (ip->type == T_DIR) {
>           end_op();
>           return -1;
>         }
>       }
>       if ((dp=create(path, T_SYMLINK, 0, 0)) == 0) {
>         end_op();
>         return -1;
>       }
>     
>       int n = strlen(target);
>       // ilock(dp); //??????这个不需要加
>       if (writei(dp, 0, (uint64)target, 0, n) != n) { 
>         panic("symlink: writei");
>       }
>     
>       
>       iunlockput(dp); //用完inode记得释放
>       end_op();
>       return 0;
>     
>     }
>     ```
>
>   - 修改open系统调用
>
>     ```c
>     uint64
>     sys_open(void)
>     {
>       char path[MAXPATH];
>       int fd, omode;
>       struct file *f;
>       struct inode *ip;
>       int n;
>     
>       if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
>         return -1;
>     
>       begin_op();
>     
>     
>       if(omode & O_CREATE){
>         ip = create(path, T_FILE, 0, 0);
>         if(ip == 0){
>           end_op();
>           return -1;
>         }
>       } else {
>         if((ip = namei(path)) == 0){
>           end_op();
>           return -1;
>         }
>         ilock(ip);
>         if(ip->type == T_DIR && omode != O_RDONLY){
>           iunlockput(ip);
>           end_op();
>           return -1;
>         }
>       }
>     
>     
>     
>       //对于O_NOFOLLOW标志位没有的不用进行追踪/////////
>       if ((omode & O_NOFOLLOW) == 0) {
>         struct inode* dp;
>         char npath[MAXPATH];
>         int i;
>         for (i = 0; i < 10 && ip->type == T_SYMLINK; i++) {
>           //从inode读取数据到npath中  
>           if (readi(ip, 0, (uint64)npath, 0, MAXPATH) == 0) {
>             iunlockput(ip);
>             end_op();
>             return -1;
>           }
>     
>           if ((dp = namei(npath)) == 0) {
>             iunlockput(ip);
>             end_op();
>             return -1;
>           }
>           iunlockput(ip);
>           ip = dp;
>           ilock(ip);
>         }
>         if (i == 10) {
>           iunlockput(ip);
>           end_op();
>           return -1;
>         }
>       }
>     
>       /////////////////////
>     
>       if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
>         iunlockput(ip);
>         end_op();
>         return -1;
>       }
>     
>       if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
>         if(f)
>           fileclose(f);
>         iunlockput(ip);
>         end_op();
>         return -1;
>       }
>     
>       if(ip->type == T_DEVICE){
>         f->type = FD_DEVICE;
>         f->major = ip->major;
>       } else {
>         f->type = FD_INODE;
>         f->off = 0;
>       }
>       f->ip = ip;
>       f->readable = !(omode & O_WRONLY);
>       f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
>     
>       if((omode & O_TRUNC) && ip->type == T_FILE){
>         itrunc(ip);
>       }
>     
>       iunlock(ip);
>       end_op();
>     
>       return fd;
>     }uint64
>     sys_open(void)
>     {
>       char path[MAXPATH];
>       int fd, omode;
>       struct file *f;
>       struct inode *ip;
>       int n;
>     
>       if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
>         return -1;
>     
>       begin_op();
>     
>     
>       if(omode & O_CREATE){
>         ip = create(path, T_FILE, 0, 0);
>         if(ip == 0){
>           end_op();
>           return -1;
>         }
>       } else {
>         if((ip = namei(path)) == 0){
>           end_op();
>           return -1;
>         }
>         ilock(ip);
>         if(ip->type == T_DIR && omode != O_RDONLY){
>           iunlockput(ip);
>           end_op();
>           return -1;
>         }
>       }
>     
>     
>     
>       //对于O_NOFOLLOW标志位没有的不用进行追踪/////////
>       if ((omode & O_NOFOLLOW) == 0) {
>         struct inode* dp;
>         char npath[MAXPATH];
>         int i;
>         for (i = 0; i < 10 && ip->type == T_SYMLINK; i++) {
>           //从inode读取数据到npath中  
>           if (readi(ip, 0, (uint64)npath, 0, MAXPATH) == 0) {
>             iunlockput(ip);
>             end_op();
>             return -1;
>           }
>     
>           if ((dp = namei(npath)) == 0) {
>             iunlockput(ip);
>             end_op();
>             return -1;
>           }
>           iunlockput(ip);
>           ip = dp;
>           ilock(ip);
>         }
>         if (i == 10) {
>           iunlockput(ip);
>           end_op();
>           return -1;
>         }
>       }
>     
>       /////////////////////
>     
>       if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
>         iunlockput(ip);
>         end_op();
>         return -1;
>       }
>     
>       if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
>         if(f)
>           fileclose(f);
>         iunlockput(ip);
>         end_op();
>         return -1;
>       }
>     
>       if(ip->type == T_DEVICE){
>         f->type = FD_DEVICE;
>         f->major = ip->major;
>       } else {
>         f->type = FD_INODE;
>         f->off = 0;
>       }
>       f->ip = ip;
>       f->readable = !(omode & O_WRONLY);
>       f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
>     
>       if((omode & O_TRUNC) && ip->type == T_FILE){
>         itrunc(ip);
>       }
>     
>       iunlock(ip);
>       end_op();
>     
>       return fd;
>     }
>     ```
>
> - 结果
>
>   ```c
>   make qemu
>   symlinktest
>   usertests #看到都ok即可
>   ```
>
>   

整个lab进行测试，linux终端中

> - 在xv6-labs-2021文件夹下新建time.txt，填入数字
>
> - ./grade-lab-fs
>
>   ![image-20221124114944809](D:\WorkSpace\Lab\MITS081\file_systems.assets\image-20221124114944809.png)
>

#### 总结

> - 该实验第一部分需要了解XV6系统中文件在磁盘中的组织结构比如超级块之类的，还有dinode的结构
> - 该实验第二部分主要了解软连接。该部分可以调用其他已经实现好的系统调用，例如create等。其次，需要注意的是用完inode后需要释放锁以及减少ref数。当我们不需要访问struct inode的成员时应该调用iunlock释放掉inode的锁，需要访问的时候再获取锁；当我们不需要使用struct inode的指针时，要调用iput弃用掉这个指针。
> - 实验二的第一个关键部分在于可以将软链接视为一个文件，然后用create进行创建，用writei将链接到的路径写入到数据块中；第二个关键部分就是锁的使用和释放。
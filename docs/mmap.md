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

### Lab:  mmap

链接：[Lab: mmap (mit.edu)](https://pdos.csail.mit.edu/6.S081/2021/labs/mmap.html)

> 需要注意的是我们做的实验是2021的，因此要找到2021的指导书
>
> 前期准备
>
> ```bash
> git fetch
> git checkout fs //在进行这句话的时候确保实验一已经commit
> make clean
> ```

#### mmap（hard）

读写文件都需要经过页缓存（在内核空间中）。传统的读写文件需要将文件内容读入到内存中、修改内存中的内容、把内存的数据写入到文件中。使用mmap可以直接在用户空间中读写页缓存，可以免去将页缓存的数据复制到用户空间缓冲区的过程。

```c
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```

1. 映射的地址由内核自行决定，即addr总是为0
2. length是要映射的字节数，它可能与文件的长度不一样
3. prot参数支持PROT_READ或PROT_WRITE或PROT_WRITE|PROT_READ
4. flags参数支持MAP_SHARED或MAP_PRIVATE。MAP_SHARED意味着对映射内存的修改应该被写回文件；MAP_PRIVATE意味着它们不应该被写回。offset是要映射的文件的起始点，可以假定offset为0

mmap返回 0xffffffffffffffff表明失败

首先我们来看实验要求和提示信息：

![image-20221110163730059](D:\WorkSpace\Lab\MITS081\mmap.assets\image-20221110163730059.png)

![image-20221110163744313](D:\WorkSpace\Lab\MITS081\mmap.assets\image-20221110163744313.png)

> - 实验要求：实现足够的mmap和munmap函数让mmaptest工作。如果mmaptest不适用某个mmap功能，就不需要 实现该功能
>
> - 提示信息：
>
>   - 在UPROGS中加入_mmaptest。然后添加mmap和munmap的系统调用。现在只需要从mmap和munmap返回错误
>   - 懒惰填页表用来应对页故障。也就是说mmap不应该分配物理内存或者读取文件。相反，在usertrap中（或由usertrap调用）的页面故障处理代码中进行。懒惰分配的原因是为了确保大文件的mmap是快速的，而且比物理内存大的文件的mmap是可能的
>   - 追踪mmap为每个进程映射了什么。定义一个与第15讲中描述的VMA（虚拟内存区）相对应的结构，记录mmap创建的虚拟内存范围的地址、长度、权限、文件等。由于xv6内核中没有内存分配器，因此可以声明一个固定大小的VMA数组，并根据需要从该数组中进行分配。16的大小应该是足够的
>   - 执行mmap：在进程的地址空间中找到一个未使用的区域来映射文件，并在进程的映射区域表中添加一个VMA。VMA应该包含一个指向被映射文件的结构文件的指针；mmap应该增加该文件的引用计数，以便在文件关闭时结构不会消失（提示：见filedup）。运行mmaptest：第一个mmap应该成功，但是对mmaped内存的第一次访问将导致一个页面故障并杀死mmaptest
>   - 添加代码，在一个mmap过的区域中引起页面故障，分配一个物理内存页，将相关文件的4096字节读入该页，并将其映射到用户地址空间。用readi读取文件，它需要一个偏移量参数来读取文件（但你必须锁定/解锁传递给readi的inode）。不要忘记正确设置页面的权限。运行mmaptest；它应该进入第一个Munmap
>   - 实现munmap：找到地址范围的VMA，并取消指定的页面映射（提示：使用uvmunmap）。如果munmap删除了之前mmap的所有页面，它应该递减相应结构文件的引用计数。如果一个未映射的页被修改了，并且文件被映射为MAP_SHARED，那么就把这个页写回文件。看一下filewrite，从中得到启发
>   - 理想情况下，你的实现只写回程序实际修改的MAP_SHARED页。RISC-V PTE中的dirty bit (D)表示一个页面是否已经被写入。然而，mmaptest并不检查非脏页是否被写回；因此你可以不看D位而写回页面
>   - 修改exit以解除进程的映射区域，就像调用了munmap一样。运行mmaptest；mmap_test应该通过，但可能不会通过fork_test
>   - 修改fork以确保子代拥有与父代相同的映射区域。不要忘记为VMA的结构文件增加引用计数。在子代的页面故障处理程序中，分配一个新的物理页面而不是与父代共享一个页面是可以的。后者会更酷，但它需要更多的实现工作。运行mmaptest；它应该同时通过mmap_test和fork_test
>
> - 解决方案
>
>   - 在用户空间中定义mmap系统调用和入口。mumap类似
>
>     ```c
>     //    user/user.h
>     void* mmap(void*, int, int, int, int, int);
>     
>     //    usys.pl
>     entry("mmap");
>     
>     //    Makefile
>     $U/_mmaptest\
>         
>     //    kernel/syscall.h
>     #define SYS_mmap 22
>         
>     //    kernel/syscall.c
>     extern uint64 sys_mmap(void);
>     
>     [SYS_mmap] sys_mmap;
>     
>     //    kernel/sysfile.c实现mmap
>     
>     ```
>
>     
>
>   - 定义vm_area_struct结构，用来记录映射区域，并在进程结构体中添加该结构体。kernel/proc.h
>
>     ```c
>     #define MAXVMA 16
>     struct vma {
>       int mapped;
>       uint64 addr;
>       int len;
>       int prot;
>       int flags;
>       int offset;
>       struct file *f;
>     };
>     struct proc {
>        ...
>       struct vma vma_table[MAXVMA];// Table of mapped regions
>     }
>     ```
>     
> - 进程初始化对vm_area_struct初始化。kernel/proc.c
>   
>   ```c
>       // An empty user page table.
>       p->pagetable = proc_pagetable(p);
>       if(p->pagetable == 0){
>         freeproc(p);
>         release(&p->lock);
>         return 0;
>       }
>     
>       //新加
>       for (int i = 0; i < MAXVMA; i++) {
>         p->vma_table[i].mapped = 0;
>       }
>     ```
>   
>   
>   
> - sys_mmap。kernel/sysfile.c
>   
>   ```c
>     uint64
>     sys_mmap(void)
>     {
>       struct proc *p = myproc();
>       uint64 addr;
>       int len;
>       int prot;
>       int flags;
>       int offset;
>       struct file *f;
>       if(argaddr(0, &addr) < 0 || argfd(4, 0, &f) < 0){
>         return -1;
>       }
>       if(argint(1, &len) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(5, &offset) < 0){
>         return -1;
>       }
>       //check the mmaptest.c to solve
>       if(!f->writable && (prot & PROT_WRITE) && flags == MAP_SHARED) return -1;
>       for(int i = 0; i < MAXVMA; i++){
>         if(p->vma_table[i].mapped == 0){
>           p->vma_table[i].mapped = 1;
>           //if addr = 0, vma_table[i].addr is the top of heap (i.e. bottom of trapframe)
>           p->vma_table[i].addr = addr + p->sz;
>           p->vma_table[i].len = PGROUNDUP(len);
>           p->vma_table[i].prot = prot;
>           p->vma_table[i].flags = flags;
>           p->vma_table[i].offset = offset;
>           p->vma_table[i].f = filedup(f);
>           p->sz += PGROUNDUP(len);
>           return p->vma_table[i].addr;
>         }
>       }
>       return -1;
>     }
>     ```
>     
>   - 修改usertrap。kernel/trap.c。缺页中断处理
>   
>     ```c
>     #include "types.h"
>     #include "param.h"
>     #include "memlayout.h"
>     #include "riscv.h"
>   #include "spinlock.h"
>     #include "proc.h"
>   #include "defs.h"
>     
>     // 一定要加上
>     #include "fcntl.h"
>     #include "sleeplock.h"
>     #include "fs.h"
>     #include "file.h"
>     
>     void
>     usertrap(void)
>     {
>         ...
>       else if(r_scause() == 13 || r_scause() == 15){
>         uint64 va = r_stval();
>         struct vma *pvma;
>         int i = 0;
>         //p->sz point to the top of heap(i.e. bottom of trapframe)
>         //p->trapframe->sp point to the top of stack
>         if((va >= p->sz) || (va < PGROUNDDOWN(p->trapframe->sp))){
>           p->killed = 1;
>         } else{
>           va = PGROUNDDOWN(va);
>           for(; i < MAXVMA; i++){
>     	      pvma = &p->vma_table[i];
>             if(pvma->mapped && (va >= pvma->addr) && (va < (pvma->addr + pvma->len))){
>               char *mem;
>     	        mem = kalloc();
>     	        if(mem == 0){
>     	          p->killed = 1;
>     	          break;
>     	        }
>     	        memset(mem, 0, PGSIZE);
>               //PTE_R (1L << 1), so prot also needs to move left one bit
>               if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, (pvma->prot << 1) | PTE_U) != 0){
>                 kfree(mem);
>                 p->killed = 1;
>                 break;
>               }
>               break;
>     	      }
>           }
>         }
>     
>         if(p->killed != 1 && i <= MAXVMA){
>           ilock(pvma->f->ip);
>           readi(pvma->f->ip, 1, va, va - pvma->addr, PGSIZE);
>           iunlock(pvma->f->ip);
>         }
>       }
>     }
>     ```
>   
>   - 修改sys_munmap。kernel/sysfile.c
>   
>     ```c
>     uint64
>     sys_munmap(void)
>     {
>     
>       struct proc *p = myproc();
>       uint64 addr;
>       int len;
>       int i;
>     
>       if(argaddr(0, &addr) < 0 || argint(1, &len) < 0)
>         return -1;
>       //printf("[[[[[[[[[[[[[[[[munmap begin[[[[[[[[[[[[[[[[[[[\n");
>       //printf("va: %p\n", addr);
>       //printf("len: %d\n", len);
>       for(i = 0; i < 16; i++){
>         if(p->vma[i].valid == 1 && p->vma[i].addr <= addr && (p->vma[i].addr + p->vma[i].len) >= (addr + len)){
>     
>             //printf("i: %d\n", i);
>             //printf("vma: va: %p\n", p->vma[i].addr);
>             //printf("vma: len: %d\n", p->vma[i].len);
>             if((p->vma[i].flag & MAP_SHARED) != 0){
>                 filewrite(p->vma[i].f, addr, len);
>             }
>     
>             if(addr == p->vma[i].addr){
>                 p->vma[i].addr = addr + len;
>             }
>             p->vma[i].len -= len;
>     
>             if(p->vma[i].len == 0){
>                 //printf("qqqqqqqqqqqqqqqqqqqqqqqqqqqq\n");
>                 fileclose(p->vma[i].f);
>                 p->vma[i].valid = 0;
>             }
>             uvmunmap(p->pagetable, addr, len/PGSIZE, 0);
>             break;
>         }
>       }
>       //printf("]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\n");
>   
>       if(i==16)
>       return -1;
>       return 0;
>     }
>     ```
>   
>   - 修改uvmunmap函数和uvmcopy函数。kernel/vm.c
>   
>     ```c
>         if((*pte & PTE_V) == 0)
>           //panic("uvmcopy: page not present");
>             continue;
>     ```
>   
>   - 修改fork。kernel/proc.c
>   
>     ```c
>     ... 
>       np->cwd = idup(p->cwd);
>     
>       for(int i = 0; i < MAXVMA; i++){
>         if(p->vma_table[i].mapped){
>           memmove(&np->vma_table[i], &p->vma_table[i], sizeof(struct vma));
>           filedup(np->vma_table[i].f);
>         }
>       }
>       safestrcpy(np->name, p->name, sizeof(p->name));
>     ...
>     ```
>     
>     
>     
>   - 修改exit。kernel/proc.c
>   
>     ```c
>     ...  
>     // Close all open files.
>       for(int fd = 0; fd < NOFILE; fd++){
>         if(p->ofile[fd]){
>           struct file *f = p->ofile[fd];
>           fileclose(f);
>           p->ofile[fd] = 0;
>         }
>       }
>       // 新增代码
>       // 查看所有内存映射区域并释放
>       struct vma *pvma;
>       for(int i = 0; i < MAXVMA; i++){
>       pvma = &p->vma_table[i];
>         if(pvma->mapped){
>         //uvmunmap(p->pagetable, pvma->addr, pvma->len / PGSIZE, 0);
>           //memset(pvma, 0, sizeof(struct vma));
>         if(munmap(pvma->addr, pvma->len) < 0){
>             panic("exit munmap");
>           }
>         }
>       }
>     ...
>   ```
>     
>   
>   

问题：

尝试了许多方法后都会有

**error:** dereferencing pointer to incomplete type 'struct file'

![image-20221111193011930](D:\WorkSpace\Lab\MITS081\mmap.assets\image-20221111193011930.png)

排除了很久才发现问题，还是太菜了呜呜。

问题：头文件的锅！原来只在kernel/trap.c中加入了#include "file.h"，发现还是不行后就没有深究了。

解决：在kernel/trap.c中加入下列语句

```c
...
    
#include "fcntl.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
```





整个lab进行测试，linux终端中

> - 在xv6-labs-2021文件夹下新建time.txt，填入数字
>
> - ./grade-lab-mmap
>

![image-20221129135950235](D:\WorkSpace\Lab\MITS081\mmap.assets\image-20221129135950235.png)



#### 总结

> - 该实验还是很难的，综合考察了系统调用、缺页中断以及mmap的内容
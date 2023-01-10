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

### Lab:  Multithreading

链接：[Lab: Multithreading (mit.edu)](https://pdos.csail.mit.edu/6.S081/2021/labs/thread.html)

> 需要注意的是我们做的实验是2021的，因此要找到2021的指导书
>
> 前期准备
>
> ```bash
> git fetch
> git checkout thread //在进行这句话的时候确保实验一已经commit
> make clean
> ```

锁保障了并发性带来的问题，也限制了性能。

加锁的条件：（**竞争**）如果两个进程访问共享的数据结构，并且其中一个进程会更新共享的数据结构，那么就需要对这个共享的数据结构加锁。

实验将会让我们熟悉多线程。将在用户级中实现线程切换；并使用多线程加速程序；实现barrier

#### Uthread: switching between threads(moderate)

首先，我们来看实验要求和提示信息：

![image-20221016204950315](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221016204950315.png)

![image-20221016205010626](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221016205010626.png)

![image-20221016205027583](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221016205027583.png)

> - 实验要求：制定一个计划**创建线程**并通过**保存/恢复寄存器**在**进程之间进行切换**。
>
> - 实验提示：
>
>   - 需要在user/uthread.c对thread_create和thread_schedule函数添加代码，然后在user/uthread_switch.S中对thread_switch添加代码。
>
>   - thread_switch函数需要保存/恢复被调用者保存寄存器
>
>   - 修改struct thread结构，用来保存寄存器
>
>   - 需要在thread_schedule函数中调用thread_switch
>
>   - 需要进行正确的线程切换，从thread t切换到next_thread
>
>   - 可以看user/uthread.asm汇编代码，方便调试
>
>   - 为了测试代码可以使用gdb调试，
>
>     ```bash
>     (gdb)file user/_uthread
>     (gdb)b uthread.c 60
>     ```
>
>     具体见[gdb调试常用指令](####gdb调试常用指令) 
>
> - 解决：其实比较简单
>
>   - 编写user/uthread_switch.S（其实和switch.S中的汇编代码一样）
>
>     ```assembly
>     	.text
>     
>     	/*
>              * save the old thread's registers,
>              * restore the new thread's registers.
>              */
>     
>     	.globl thread_switch
>     thread_switch:
>     	/* YOUR CODE HERE */
>     
>     	sd ra, 0(a0)
>     	sd sp, 8(a0)
>     	sd s0, 16(a0)
>     	sd s1, 24(a0)
>     	sd s2, 32(a0)
>     	sd s3, 40(a0)
>     	sd s4, 48(a0)
>     	sd s5, 56(a0)
>     	sd s6, 64(a0)
>     	sd s7, 72(a0)
>     	sd s8, 80(a0)
>     	sd s9, 88(a0)
>     	sd s10, 96(a0)
>     	sd s11, 104(a0)
>     
>     	ld ra, 0(a1)
>     	ld sp, 8(a1)
>     	ld s0, 16(a1)
>     	ld s1, 24(a1)
>     	ld s2, 32(a1)
>     	ld s3, 40(a1)
>     	ld s4, 48(a1)
>     	ld s5, 56(a1)
>     	ld s6, 64(a1)
>     	ld s7, 72(a1)
>     	ld s8, 80(a1)
>     	ld s9, 88(a1)
>     	ld s10, 96(a1)
>     	ld s11, 104(a1)
>     
>     
>     	ret    /* return to ra */
>     
>     ```
>
>   - 修改线程结构，便于寄存器保存
>
>     ```c
>     //新加
>     struct thread_context{
>       uint64     ra;
>       uint64     sp;
>       uint64     fp; 
>       uint64     s1;
>       uint64     s2;
>       uint64     s3;
>       uint64     s4;
>       uint64     s5;
>       uint64     s6;
>       uint64     s7;
>       uint64     s8;
>       uint64     s9;
>       uint64     s10;
>       uint64     s11;
>     };
>     
>     struct thread {
>       char       stack[STACK_SIZE]; /* the thread's stack */
>       int        state;             /* FREE, RUNNING, RUNNABLE */
>     	
>       //新加
>       struct thread_context context;
>     };
>     ```
>
>   - 完善thread_create的代码
>
>     ```c
>     void 
>     thread_create(void (*func)())
>     {
>       struct thread *t;
>     
>       for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
>         if (t->state == FREE) break;
>       }
>       t->state = RUNNABLE;
>       // YOUR CODE HERE
>     
>       t->context.ra = (uint64)func;
>       t->context.sp = (uint64)t->stack + STACK_SIZE - 1; //高地址，栈底
>     }
>     ```
>
>     
>
>   - 在thread_schedule函数中调用thread_switch
>
>     ```c
>         ...
>     	/* YOUR CODE HERE
>          * Invoke thread_switch to switch from t to next_thread:
>          * thread_switch(??, ??);
>          */
>         thread_switch((uint64)&t->context, (uint64)&next_thread->context);//->优先级高于&
>     	...
>     ```
>
> 该实验中，函数声明以及我们要加的代码的位置(YOUR CODE HERE)都已经提供好了，因此只需要根据提示在对应位置加上相关逻辑的代码即可。

#### Using threads(moderate)

首先，我们来看**实验要求**和**提示信息**：

> - 实验要求：使用哈希表探索并行编程和所得使用
>
> - 提示信息
>
>   - 使用下面四个函数
>
>   ![image-20221017104955914](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221017104955914.png)
>
>   - 不要忘记调用pthread_mutex_init
>
> - 如果不加锁会造成线程不安全，当有多线程的时候，会导致丢项，然后会导致get和put不匹配，然后结果如下：（./ph 2，2是代表线程数）
>
>   ![image-20221017105127661](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221017105127661.png)

哈希表不是**线程安全**的，当多个线程同时写的时候可能会出现丢项的情况。

解决：要对puts和gets进行**加锁**

> - 解决方案
>
>   - 不同的对象被哈希到不同的桶中，每个桶之间相互独立，很明显可以想到声明的锁的数量要和桶数量相等
>
>   - 需要新加全局锁；对gets和puts函数进行加锁解锁；在main函数中对锁进行**初始化**
>
>   - ```c
>     //notxv6/ph.c
>     
>     //新加全局锁
>     pthread_mutex_t nbucketlock[NBUCKET];
>     
>     static 
>     void put(int key, int value)
>     {
>       int i = key % NBUCKET;
>     
>       // is the key already present?
>       pthread_mutex_lock(&nbucketlock[i]);//找到索引位置后加锁
>       ...
>       ...
>       pthread_mutex_unlock(&nbucketlock[i]); //返回前解锁
>     
>     }
>     
>     static struct entry*
>     get(int key)
>     {
>       int i = key % NBUCKET;
>     
>       pthread_mutex_lock(&nbucketlock[i]);//找到索引位置后加锁
>       ...
>       ...
>       pthread_mutex_unlock(&nbucketlock[i]);//返回前解锁
>       return e;
>     }
>     
>     int
>     main(int argc, char *argv[])
>     {
>       pthread_t *tha;
>       void *value;
>       double t1, t0;
>     
>       //根据提示对锁进行初始化
>       for (int i = 0; i < NBUCKET; i++) {
>         pthread_mutex_init(&nbucketlock[i], NULL);
>       }
>       ...
>           
>     ```
>
> - 测试：在自己的linux终端上（注意不是在xv6中）
>
>   ```bash
>   gcc ph.c -o ph -lpthread
>   ./ph 2
>   ```
>
>   发现对应的线程都是0 keys missing即可

#### Barrier(moderate)

首先，我们来看**实验要求**和**提示信息**：

> - 实验要求：线程同步。在所有线程均执行到特定点后，该轮才算结束，可以进行新增一轮。但是不同线程之间速度是不一样的，因此先到特定点的线程需要等待还没到特定点的线程，待他们都到达特定点后，该轮结束，计数清零，开启下一轮。
>
> - 提示：
>
>   - 使用下面两个函数进行线程等待和线程唤醒
>
>   ![image-20221017111404436](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221017111404436.png)
>
>   - 在所有线程都到达特定点后，对BSTATE的轮数进行增加
>
>   - 需要对BSTATE的nthread进行清0，在该轮结束后。因此在下一轮中需要进行复用
>
>   - 需要实现barrier.c中的barrier函数
>
>     ```c
>     // notxv6/barrier.c
>     static void 
>     barrier()
>     {
>       // YOUR CODE HERE
>       //
>       // Block until all threads have called barrier() and
>       // then increment bstate.round.
>       //
>     
>       pthread_mutex_lock(&bstate.barrier_mutex);
>       
>       ++bstate.nthread;
>     
>       if (bstate.nthread != nthread) {
>         pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
>       } else {
>         ++bstate.round;
>         bstate.nthread = 0;
>         pthread_cond_broadcast(&bstate.barrier_cond);
>       }
>       pthread_mutex_unlock(&bstate.barrier_mutex);  
>     }
>     
>     ```
>
> - 测试：在自己的linux终端上（注意不是在xv6中）
>
>   ```bash
>   gcc barrier.c -o barrier -lpthread
>   ./barrier 2
>   ```

整个lab进行测试，在linux终端中

> - 在xv6-labs-2021文件夹下新建time.txt，填入数字
>
> - ```bsh
>   make qemu 
>   uthread
>   ```
>
> - 将进入xv6中运行uthread的输出复制，然后在xv6-labs-2021文件夹下新建answers-thread.txt，将内容拷贝进去
>
> - ```bash
>   ./grade-lab-thread
>   ```

![image-20221017112137538](D:\WorkSpace\Lab\MITS081\multithreading.assets\image-20221017112137538.png)

#### 总结

> - 多线程可以提高速度
> - 出现竞争问题时，多线程会错误，因此需要加锁
> - 线程之间等待和唤醒
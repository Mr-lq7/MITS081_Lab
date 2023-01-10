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

### Lab:  networking

链接：[Lab: networking (mit.edu)](https://pdos.csail.mit.edu/6.S081/2021/labs/net.html)

> 需要注意的是我们做的实验是2021的，因此要找到2021的指导书
>
> 前期准备
>
> ```bash
> git fetch
> git checkout net //在进行这句话的时候确保实验一已经commit
> make clean
> ```

以太网地址也叫MAC地址，是给每一个制造出来的网卡分配一个唯一的ID，长度是48位。这48位地址中，前24bit表示的是制造商，后24位是由网卡制造商提供的任意唯一数字。

以太网帧：（从右到左）

| payload（负载） | T（类型） | S（源地址） | D（目的地址） |
| --------------- | --------- | ----------- | ------------- |
|                 | 16位      | 48位        | 48位          |

以太网的header包括48bit目的以太网地址，48bit源以太网地址，16bit类型

ARP：根据IP地址（32位）获取物理地址（48位）的一个TCP/IP协议。类型是0X0806。IP报：类型是0X0800

以太网header：16个字节；IPheader：20个字节

UDP包的ip协议号17

![image-20221103151340332](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103151340332.png)

网络协议栈自顶向下添加header，自底向上剥离header。

xv6依靠网络栈（network stack）实现收发数据。

发包：当网络栈需要发送一个包的时候，会先将包发送到**环形缓冲区tx_ring**，之后网卡再将包发出去。每次发送包前都要检查上一次的包是否发送完，若发送完则要释放。

收包：当网卡需要接收包的时候，网卡会直接访问内存，先将接收到的RAM的数据写入到接收**环形缓冲区rx_ring**中，接着网卡会向cpu发出一个硬件中断，当cpu接收到中断后，cpu就可以从rx_ring中读取数据包到网络栈中。

---

首先，我们来看一下实验要求和提示信息：

![image-20221103164209645](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103164209645.png)

![image-20221103164221828](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103164221828.png)

> - 实验要求：需要在kernel/e1000.c中实现e1000_transmit()和e1000_recv()。MBUF(memory buffer)：存储器缓存。用于保存在进程和网络接口间相互传递的用户数据。
>
> - 描述符指的是rx_ring和tx_ring
>
> - e1000_transmit实验提示：
>
>   - 通过读取E1000_TDT控制寄存器向E1000询问E1000的TX环形缓冲区索引
>   - 检查环形缓冲区是否溢出。如果未在上一步获得的索引中设置E1000_TXD_STAT_DD，则返回错误。E1000_TXD_STAT_DD是status的对应bit
>   - 否则，使用mbuffree()释放从该描述符传输的最后一个MBUF（如果有）
>   - 然后填写描述符。m->head指向存储器中数据包的内容，m->len是数据包长度。设置必要的CMD标志
>   - 通过对索引+1对TX_RING_SIZE取模实现环形队列的更新
>   - 成功返回0，失败返回1。
>
> - e1000_transmit解决方案：
>
>   - 因为有可能出现多线程争用资源的情况，因此需要用锁
>   - 如果tx_mbufs中当前索引能够访问，要先进行释放，在进行装入
>   - 根据提示(https://pdos.csail.mit.edu/6.S081/2021/readings/8254x_GBe_SDM.pdf) 的3.3.3.1节中我们知道CMD中有8位，但是e1000_dev.h中只提供了2个标志位，因此我们猜测只需要设置这两个
>   - 代码：
>   
>   ```c
>   int
>   e1000_transmit(struct mbuf *m)
>   {
>     //
>     // Your code here.
>     //
>     // the mbuf contains an ethernet frame; program it into
>     // the TX descriptor ring so that the e1000 sends it. Stash
>     // a pointer so that it can be freed after sending.
>     //
>     acquire(&e1000_lock);
>   
>     uint32 tx_index = regs[E1000_TDT];
>   
>     // 检查状态
>     if ((tx_ring[tx_index].status & E1000_TXD_STAT_DD) == 0) {
>       release(&e1000_lock);
>       return -1;
>     }
>   
>     // 释放
>     if (tx_mbufs[tx_index]) {
>       mbuffree(tx_mbufs[tx_index]);
>     }
>   
>     // 发送
>     tx_mbufs[tx_index] = m;
>     tx_ring[tx_index].addr = (uint64)m->head;
>     tx_ring[tx_index].length = m->len;
>     tx_ring[tx_index].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
>   
>     regs[E1000_TDT] = (tx_index + 1) % TX_RING_SIZE;
>   
>     release(&e1000_lock);
>   
>     return 0;
>   }
>   ```
>   
> - e1000_recv实验提示：
>
>   - 首先通过E1000_RDT控制寄存器并对RX_RING_SIZE取模来询问下一个等待收到的数据包（如果有）的环形队列索引
>   - 通过检查描述符状态部分中的E1000_RXD_STAT_DD位来检查新数据包。如果没有就停止。（标致出问题了）
>   - 否则将mbuf的M->len更新为描述符中报告的长度，然后使用net_rx()将MBUF交付到网络堆栈中。
>   - 然后使用mbufalloc()去分配新的mbuf用来替换刚刚给出的net_rx()。之后，将数据指针m->head赋值到描述符中，并且将描述符的状态位清除为0
>   - 最后，更新E1000_RDT寄存器更新为最后一个环描述符的索引
>
> - e1000_recv解决方案：
>
>   ```c
>   static void
>   e1000_recv(void)
>   {
>     //
>     // Your code here.
>     //
>     // Check for packets that have arrived from the e1000
>     // Create and deliver an mbuf for each packet (using net_rx()).
>     //
>   
>     while (1) {
>         // get next index
>         uint32 rx_index = (regs[E1000_RDT] + 1) % RX_RING_SIZE;
>         
>         // check status, if not set we will return 
>         if ((rx_ring[rx_index].status & E1000_RXD_STAT_DD) == 0)
>           return ;
>         
>         // deliver to network stack
>         rx_mbufs[rx_index]->len = rx_ring[rx_index].length;
>         net_rx(rx_mbufs[rx_index]);
>         
>         // alloc a new mbuf and fill a new descriptor  
>         rx_mbufs[rx_index] = mbufalloc(0);
>         if (!rx_mbufs[rx_index]) {
>           panic("e1000");
>         }
>         rx_ring[rx_index].addr = (uint64)rx_mbufs[rx_index]->head;
>         rx_ring[rx_index].status = 0;
>         
>         // as current index
>         regs[E1000_RDT] = rx_index; 
>       }
>   }
>   ```

整个lab进行测试，在linux终端中

> - 在xv6-labs-2021文件夹下新建time.txt，填入数字
>
> - 在一个终端中运行make server（guest）
>
>   ![image-20221103215915663](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103215915663.png)
>
> - 在另一个终端中运行make qemu(host)
>
> - ```bash
>   make qemu 
>   nettests
>   ```
>
>   ![image-20221103221155343](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103221155343.png)
>   
> - 在make server的终端中会输出"a message from xv6"。
>
> - ./grade-lab-net

![image-20221103221621060](D:\WorkSpace\Lab\MITS081\nic.assets\image-20221103221621060.png)

#### 总结

> - xv6依靠网络栈（network stack）实现收发数据。
>
>   发包：当网络栈需要发送一个包的时候，会先将包发送到**环形缓冲区tx_ring**，之后网卡再将包发出去。每次发送包前都要检查上一次的包是否发送完，若发送完则要释放。
>
>   收包：当网卡需要接收包的时候，网卡会直接访问内存，先将接收到的RAM的数据写入到接收**环形缓冲区rx_ring**中，接着网卡会向cpu发出一个硬件中断，当cpu接收到中断后，cpu就可以从rx_ring中读取数据包到网络栈中。
>
> - 提示基本上就是伪代码，挨个翻译为代码即可。
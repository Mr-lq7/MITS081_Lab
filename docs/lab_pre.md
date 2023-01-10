### 实验配置

> - 系统：XV6系统
> - 处理器：RISC-V
> - 需要工具
>   - qemu：是纯软件实现的虚拟化模拟器，几乎可以模拟任何硬件设备，模拟一台能够独立运行操作系统的虚拟机
>   - gdb：调试器
>   - gcc：编译器
>   - binutils：二进制工具集，包括gnu链接器，objdump等
> - 用的是ubuntu系统，ubuntu源自debian，使用相同的apt包管理系统，ubuntu共享来自debian中大量的包和库
> - 指导链接![[6.S081 / Fall 2019 (mit.edu)](https://pdos.csail.mit.edu/6.828/2019/tools.html)]
> - Installing on Debian/Ubuntu
>
> ```bash
> sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu 
> ```
>
> - testing installation
>
> ```bash
> riscv64-unknown-elf-gcc --version //如果失败,需额外执行下面这句话
> sudo apt-get install gcc-riscv64-unknown-elf -y
> qemu-system-riscv64 --version
> ```
>
> 两个版本号有输出即成功
>
> - pull xv6-riscv repo
>
> ```bash
> git@github.com:mit-pdos/xv6-riscv.git
> cd xv6-riscv/
> make qemu
> #退出qemu
> ctrl+a+x
> ```

### 实验介绍

> - 实验难度等级
>   - Easy
>   - Moderate
>   - Hard：通常不需要成百上千的代码，但是是概念复杂的，并且细节上很重要
> - 调试技巧
>   - make qemu #正常启动
>   - make qemu-gdb #以gdb模式启动
>   - print
>   - 将print的内容重定向到文件中

### 实验评估

> - 需要在user文件夹下创建文件来实现相应的功能
>
> - 编写完成后，需要在Makefile文件里加上新建的文件用于启动的时候进行编译
>
> - ctrl+a+x退出模拟器后，使用下面命令来运行测试用例
>
>   ```
>   ./grade-lab-util xxx
>   #或者
>   make GRADEFLAGS=xxx grade
>   ```
>
>   其中xxx是新建文件的文件名

> github仓库网址：git://g.csail.mit.edu/xv6-labs-2021
>
> ```
> git clone git://g.csail.mit.edu/xv6-labs-2021
> ```
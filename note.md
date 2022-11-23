# 构建流程

> *.c -> gcc -> *.o
> *.s -> as -> *.o
>
> *.o -> ld -> kernel.bin
>
> grub -> grub-rescue + kernel.bin -> my-os.iso

使用target=i686-elf作为gcc, as, ld交叉编译套件

## 跑内核

- 虚拟机(e.g., Virtual Box)
- QEMU (x86模拟器)
- Bochs (x86模拟器)

使用Bochs

## 具体操作

### 1. 使用自动环境安装脚本

sh gcc-build.sh(跑大概半个小时左右)

gcc-build.sh为下列代码

```c
#! /usr/bin/bash
sudo apt update &&\
     sudo apt install -y \
		build-essential \
		bison\
		flex\
		libgmp3-dev\
		libmpc-dev\
		libmpfr-dev\
		texinfo

BINUTIL_VERSION=2.37
BINUTIL_URL=https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.xz

GCC_VERSION=11.2.0
GCC_URL=https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz

GCC_SRC="gcc-${GCC_VERSION}"
BINUTIL_SRC="binutils-${BINUTIL_VERSION}"

# download gcc & binutil src code

export PREFIX="$HOME/cross-compiler"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p "${PREFIX}"
mkdir -p "${HOME}/toolchain/binutils-build"
mkdir -p "${HOME}/toolchain/gcc-build"

cd "${HOME}/toolchain"

if [ ! -d "${HOME}/toolchain/${GCC_SRC}" ]
then
	(wget -O "${GCC_SRC}.tar" ${GCC_URL} \
		&& tar -xf "${GCC_SRC}.tar") || exit
	rm -f "${GCC_SRC}.tar"
else
	echo "skip downloading gcc"
fi

if [ ! -d "${HOME}/toolchain/${BINUTIL_SRC}" ]
then
	(wget -O "${BINUTIL_SRC}.tar" ${BINUTIL_URL} \
		&& tar -xf "${BINUTIL_SRC}.tar") || exit
	rm -f "${BINUTIL_SRC}.tar"
else
	echo "skip downloading binutils"
fi

echo "Building binutils"

cd "${HOME}/toolchain/binutils-build"

("${HOME}/toolchain/${BINUTIL_SRC}/configure" --target=$TARGET --prefix="$PREFIX" \
	--with-sysroot --disable-nls --disable-werror) || exit

(make && make install) || exit

echo "Binutils build successfully!"

echo "Building GCC"

cd "${HOME}/toolchain/gcc-build"

which -- "$TARGET-as" || echo "$TARGET-as is not in the PATH"

("${HOME}/toolchain/${GCC_SRC}/configure" --target=$TARGET --prefix="$PREFIX" \
	--disable-nls --enable-languages=c,c++ --without-headers) || exit

(make all-gcc &&\
 make all-target-libgcc &&\
 make install-gcc &&\
 make install-target-libgcc) || exit

echo "done"
```

### 2. 添加环境变量

经测试完成上面的操作后,在root/cross-compiler中为编译完成的gcc交叉编译环境的二进制等文件
将cross-compiler整个文件夹复制到用户的home文件夹中,打开etc/profile永久添加环境变量

```c
sudo vi etc/profile
# 添加以下内容    
export PATH="/home/<你的用户名>/cross-compiler/bin:$PATH"
```

### 3. 检测环境配置是否成功

执行

```c
i686-elf-gcc -dumpmachine
```

输出结果应为

```
i686-elf
```

## 项目结构规划

> arch/ 平台/架构相关代码
>
> kernel/ 所有内核相关代码(*.c)
>
> includes/ 内核头文件(*.h)
>
> hal/ 硬件抽象层
>
> libs/ 一些常用库实现

# 使用GRUB引导

## AT&T和Intel 汇编差异

INTEL: 指令 目的数 原操作数
AT&T: 指令 原操作数 目的数

指令后缀: 

> b=8, w=16, l=32, q=64

src和dest:

> 寄存器: %eax, %ebp
> 立即数: $123, $-1, $0x23
> 内存引用: 0x007c0, (%esp)

## 声明os是multiboot兼容的

一个相当成熟的 Bootloader 规范
省去了相当的麻烦：比如加载内核，开启 A20 总线（ Tricky! ），进入保护模
式,直接进入内核开发

![image-20221117233737146](/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221117233737146.png)

## 加载后处在的状态

> 已定义(节选)
>
> %EAX = 0x2BADB002 (magic number)
> %EBX = Bootloader 信息表的物理地址
> %{C|D|E|F|G|S}S = 0x0 (段寄存器)
> A20 ：已打开 (A20总线打开)
> %CR0 = (%CR0 & 0x7FFFFFFF) | 0x1 (控制寄存器最高位清零 PG=0保护模式,最低位置1 PE=1分页开启)
> %EFLAGS = %EFLAGS & 0xFFFEFEFF (IF=VM=0 关闭所有硬件中断)

> 未定义(需要自己设置)
>
> %ESP:调用栈地址(重要)
> %GDTR, %IDTR: 描述表,中断表
> **避免对%{C|D|E|F|G|S}的操作,否则引发#GP异常**

# 简单显示

**VGA文本模式**

文本缓冲区（显存）起始点： 0xB8000 （注意，物理地址！）
缓冲区大小： 2×W×H 字节 (W= 屏幕宽度，H= 高度，以字符为单位

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221120115152299.png" alt="image-20221120115152299" style="zoom:50%;" />

# 链接器配置

将这些占位符换成具体的地址保证multiboot在最开头(至少前8KB处)
配置链接器行为

# 安装GDT

通过 段基地址 + 段内偏移，我们可以计算出段内任何一个数据的 线性地址

## 全局描述符GDT

全局描述符表是一块内存空间，最大 64KB
全局描述符的地址指针会被存放在一个特殊的寄存器里—— GDTR

## 段描述符

段描述符描述了这个段的特性
每个段描述符为 8 个字节，即两个DWORD

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221120160040793.png" alt="image-20221120160040793" style="zoom:50%;" />

## 特权级

特权级有： 0 ， 1 ， 2 ， 3 这几个等级。
• 0 级，最中心的圆，所以权限最高。
• 往外特权级依次递增，权限递减
用户程序在 3 级，我们的内核会运行在 0 级。中间的等级则留给其他驱动程序或者一些服务。

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221120160154947.png" alt="image-20221120160154947" style="zoom:50%;" />

lgdt : 加载GDT的地址到GDTR

用法: lgdt(%reg)
寄存器%reg的值是GDTR的值的地址

# 中断

标志着某些事情发生,需要cpu去救场
e.g. 致命错误,键盘/鼠标输入,u盘插入,某些系统调用

## 忽略(mask)中断

cli(clear interrupt flag)
sti(set interrupt flag)

## 中断服务过程与中断描述表

为每个中断写一个 **处理函数(中断服务过程ISR)**, 编成 **一张表中断描述表IDT**

IDT: 64bits的数组

描述符的索引代表 **中断向量号**

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221120212846499.png" alt="image-20221120212846499" style="zoom:50%;" />

## 安装IDT

指令:lidt

和lgdt一样

内联汇编注入iret

**设计模式** - 门面模式(Facade)

# 分页与内核重映射

## 联级分页

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221121145342455.png" alt="image-20221121145342455" style="zoom:50%;" />

## 内核重映射

加载流程

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221121145744604.png" alt="image-20221121145744604" style="zoom:50%;" />

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221121145931490.png" alt="image-20221121145931490" style="zoom:50%;" />

# 内存管理

## 设计目标

## **物理内存管理**

分配可用叶(用于映射)
对合适的页进行Swap操作

记录所有物理页的可用性

4GiB大小 = 2^20 个记录

**位图bitmap**
0 - 可用, 1 - 已占用
需要 128Ki的空间

## **虚拟内存管理**

管理映射 - 增删改查

- 增： VA <---> PA
- 删：删除映射（删除对应表项，释放占用的物理页，刷新 TLB ）
- 改：修改映射
- 查： VA ----> PA

**虚拟地址解决方法:递归映射**

# malloc

## 堆空间

动态地,按需创建的空间

规定，所有分配的空间大小必须为 4 的倍数（地址 4 字节对齐）

## 如何处理虚碎片

Implicit Free List

边界标签法（ Boundary Tag ）——在头尾加上标签，写上一些必要元数据

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122123109348.png" alt="image-20221122123109348" style="zoom:50%;" />

# 外中断处理

## PIC 可编程中断控制器

决定CPU接下来要处理的中断
帮助CPU缓存未处理的中断
拓展CPU的中断Pin

## APIC

为多核心处理器打造,为单个核心指定不同的中断优先级

### LVT 本地向量表

一组APIC内置寄存器,定义 **APIC本地中断**至CPU中断向量号的映射

## Local APIC

进阶版PIC,专职于服务外部中断事件
允许多个IOAPIC
单个多达24个IRQ

## 初始化APIC

1. 禁用中断(cli)
2. 禁用8259PIC
3. 硬启用LAPIC
4. 初始化LAPIC
5. 设置中断优先级
6. 初始化LVT
7. 软启用LAPIC

### 使用Local APIC

- 硬件层面启动
  特殊寄存器:IA32_APIC_BASE

- 禁用8259PIC

- 设置中断优先级
  <img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122205651423.png" alt="image-20221122205651423" style="zoom:50%;" />
- 初始化LVT
- 软启用Local APIC
- 初始化I/O APIC
- 映射IRQ  (interrupt request)
  <img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122224610267.png" alt="image-20221122224610267" style="zoom:50%;" />

# APIC计时器与RTC

向CPU发送固定间隔的,周期性的中断

- 告诉CPU什么时候该进行任务调度
- 实现更加精确地等待与延时

## APIC自带Timer

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122231513865.png" alt="image-20221122231513865" style="zoom:50%;" />

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122231710068.png" alt="image-20221122231710068" style="zoom:50%;" />

F_timer = F_CPU / k

### 手动测量CPU时钟频率

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122231911158.png" alt="image-20221122231911158" style="zoom:50%;" />

## 操作步骤

1. 安装 RTC 与 LAPIC Timer 的临时中断服务例程
2. 配置 RTC 的频率（我们使用 1024Hz ）
3. 打开中断（ sti ）
4. 写入 ICR （随便找个很大的值）
5. PIE 置位
6. 阻塞
7. 当 LAPIC Timer 触发后，计算频率并关闭 RTC
8. 清除掉临时的例程

# 键盘驱动

1. 初始化8042控制器
2. 将键盘的硬件原数据进行抽象封装
3. 将封装好的键盘事件递送到上层

# 多进程

## 进程的状态

- 运行：正在使用 CPU
- 暂停：可以随时被调度
- 阻塞：只有当特定条件满足时，才会要么加载进 CPU ，要么进入“暂停”状态
- 终止（僵尸）：已终止，但仍存留在进程表里，等待用户读取返回代码，不能被调度，并且 pid 不能被回收。
- 摧毁：进程已被释放，其 pid 可以被回收重用。

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122235658847.png" alt="image-20221122235658847" style="zoom:50%;" />

## 并发物理内存访问

内存共享和引用计数

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221122235908676.png" alt="image-20221122235908676" style="zoom:50%;" />

### 记录权限

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123000234044.png" alt="image-20221123000234044" style="zoom:50%;" />

### 共享内核运行时

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123000844284.png" alt="image-20221123000844284" style="zoom:50%;" />

## Fork

fork() 的功能就是将进程一分为二。
fork() 是仅有的能返回两次的函数

### 实现fork

把父进程的一切复制过来
注意:

- pid 等进程唯一的属性不能直接复制过来
- 任何栈空间需要进行**完整**拷贝
- 对于任何读共享的内存区域，需要同时将父进程和子进程的对应映射标记为只读，从而保证 COW 的应用

### 另一种:posix_spawn()

从零创建一个进程 :
地址空间直接派生自内核
只需分配用户栈区域和代码区域。
比 fork 要轻量许多

## 进程内的 上下文切换

切换操作模式 => 切换所使用的栈

当 ISR 的特权级 (RPL) 与当前特权级 (CPL) 不一致时……
1. CPU 切换到内核栈
2. CPU 推入先前的使用的 SS 和 ESP

### 系统调用

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123003040310.png" alt="image-20221123003040310" style="zoom:50%;" />

# 信号



# PCI

外围设备互联总线

## PCI配置空间

一组 32 位寄存器组成的区域。
大小 256 字节（ 64 个寄存器）。
在前 64 字节包含了一个头部。
包括了设备的所有信息以及 PCI 相关设置

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123011442004.png" alt="image-20221123011442004" style="zoom:50%;" />

## 总线扫描

穷举出所有可能的设备地址，进行探测。
通过检测生产商 ID 来确定设备是否存在。
0xFFFF 则为不存在。

## 初始化PCI设备

command<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123011945984.png" alt="image-20221123011945984" style="zoom:50%;" />

## 操作PCI设备

不同的设备会提供不同的寄存器， OS 通过往这些寄存器里写值，向设备发送命令。
设备相关的寄存器的基地址会在 **BAR** 中给出：<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123012028507.png" alt="image-20221123012028507" style="zoom:50%;" />

### 设置中断

一个设备需要通过中断去和 CPU 通讯。
Interrupt Line ：使用的 IRQ
可以通过 ACPI 查询到 IRQ → IOAPIC针脚的映射关系，从而通过 IOAPIC 来管理这些中断。

### MSI(信息中断)

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123012552066.png" alt="image-20221123012552066" style="zoom:50%;" />

#### 启用MSI功能

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123012630392.png" alt="image-20221123012630392" style="zoom:50%;" />

#### 让CPU识别MSI

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123012658594.png" alt="image-20221123012658594" style="zoom:50%;" />

# SATA

## 如何读写磁盘

### PIO

Programmed Input/Output

1. CPU 下达指令
2. 磁盘响应指令，**通过 CPU** ，向内存拿取或写入数据

### DMA

Direct Memory Access

1. CPU 下达指令
2. 磁盘响应指令，根据事先同 CPU 商定好的地址，**通过 DMA 控制器**，直接向内存拿取或写入数据。

## SATA协议: FIS

Frame Information Structure

SATA标准定义了如下的FIS:

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123185813992.png" alt="image-20221123185813992" style="zoom:50%;" />

### LBA与CHS

SATA 控制器将物理扇区映射到逻辑扇区
软件可通过逻辑扇区的编号访问扇区

### DMA读扇区

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123190410586.png" alt="image-20221123190410586" style="zoom:50%;" />

### DMA写扇区

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123190432054.png" alt="image-20221123190432054" style="zoom:50%;" />

### SATA对SCSI的封装: PACKET命令

SATA 协议使用 PACKET 命令，对 SCSI 命令进行封装，而后由SATA 控制器使用 SCSI 协议进行转发

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123192105565.png" alt="image-20221123192105565" style="zoom:50%;" />

## AHCI

一个单独的芯片，至少可带 32 个 SATA 控制器。
是对 SATA 协议的一个更高层抽象。
每个 SATA 控制器对应到 AHCI 的一个端口上。

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123192417728.png" alt="image-20221123192417728" style="zoom:50%;" />

### AHCI与SATA的通讯

操作系统分配内存(由命令槽构成命令队列)

### AHCI初始化工作

AHCI 的标准要求软件至少执行以下几个步骤（ 10.1.2 ），以完成 AHCI
的初始化
1. GHC 寄存器的 AE 位置位，表明我们使用的是 AHCI模式，而不是 IDE 兼容模式。
2. 读取 PI 寄存器，找出所有已开启的端口
3. 读取 CAP 寄存器的 NCS 字段，找出每个端口的命令队列可用的长度（从 0 开始算）。对于每一个端口：
  1. 进行端口重置
  2. 分配操作空间，设置 PxCLB 和 PxFB
  3. 将 PxSERR 寄存器清空
  4. 将需要使用的中断通过 PxIE 寄存器开启
4. 最后将 GHC 的 IE 位置位，从而使得 HBA 可以向CPU 发送中断

需要映射寄存器到虚拟内存

<img src="/media/routhleck/Windows-SSD/Users/Routhleck/Documents/GitHub/myOS/note.assets/image-20221123193703723.png" alt="image-20221123193703723" style="zoom:50%;" />

## SLAB分配器

slab分配器基于对象进行管理，同样类型的对象归为一类(如进程描写叙述符就是一类)，每当要申请这样一个对象。slab分配器就分配一个空暇对象出去，而当要释放时，将其又一次保存在slab分配器中，而不是直接返回给伙伴系统。

## 探测磁盘信息

使用 IDENTIFY DEVICE 或 IDENTIFY PACKET DEVICE 去探测磁盘信息
两个命令均返回一个长度为 512 字节的，结构相同的数据块儿，包含该设备的所有信息。
每个信息以字段形式存储，每个字段的长度字对齐（ 2 个字节）

- 磁盘生产商名称
- 磁盘型号
- 最大可寻址逻辑扇区数 *
- 逻辑扇区大小 *
- 每个逻辑扇区包含的物理扇区数 *
- 磁盘唯一编号 (WWN)
- (ATAPI) 设备要求的 CDB 大小
- 是否支持 48 位 LBA

## 扇区读写

### 读写ATA 磁盘的扇区

如果磁盘支持 28 位 LBA ： READ/WRITE DMA
如果磁盘支持 48 位 LBA ： READ/WRITE DMA (EXT)->可向下兼容 READ/WRITE DMA

每个命令需要三个参数：
1. 起始 LBA 地址 → LBA
2. 需要读入的逻辑块数量 → count
3. 数据的存取位置 → buffer

## 读写 ATAPI 设备的扇区

需要使用 SCSI 协议的命令，使用 PACKET 命令做封装
如果磁盘支持 28 位 LBA ： 12 字节长度的 CDB
如果磁盘支持 48 位 LBA ： 16 字节长度的 CDB

和 ATA 设备的一样，也是需要三个参数：
1. 起始 LBA
2. 操作的逻辑块数
3. 存取缓冲区位置

# 文件系统

实现虚拟文件系统

1. 构造一个虚拟文件树来表示文件层级关系
2. 实现文件的十四个操作,并提供相关的系统调用
3. 实现文件系统的挂载,以及挂载点的管理
4. 对上述的十四个操作进行抽象,封装成接口. 允许22实际文件系统进行实现
5. 实现一个文件系统管理器(工厂), 从而开始初始化实际文件系统驱动的实例

## 路径游走

### 虚拟文件树的动态生长

目录节点将会随着游走的过程动态的被创建
对应的文件节点将会随着目录节点一起被创建创建

**递归**

### 目录节点缓存方案

键值对:文件名为键,目录节点实例为值 -- 哈系表

## 挂载与文件子树

### 树的递归性质之应用


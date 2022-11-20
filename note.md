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


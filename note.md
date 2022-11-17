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
将cross-compiler整个文件夹复制到用户的home文件夹中,使用

```c
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


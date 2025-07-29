# Readme

## 1:说明

这个文件下的脚本实现对代码进行加扰的功能。主要有以下文件：

collect.sh: 收集代码中的函数名，结构体名，宏名。分别保存在 fun.txt  struct.txt   macro.txt中

replace.sh: 对代码中的函数名，结构体名，宏名进行加扰替换

fun.txt：保存函数名，使用collect.sh后生成

struct.txt：保存结构体名，使用collect.sh后生成

macro.txt：保存宏名，使用collect.sh后生成



## 2:使用方法：

### 第一步：

通过collect.sh来对一个文件或者一个文件夹下的所有文件进行收集函数名，结构体名

```shell
./collect.sh  xxx.c   #xxx.c 表示一个c文件， 也可以是一个头文件
或
./collect.sh  xxx	  #xxx 表示一个文件夹名称
```

默认不对宏进行收集。通过添加第二个参数on来开启宏的收集

```shell
./collect.sh  xxx on  #xxx 表示一个文件夹名称
```

执行完成后会在脚本所在路径下生成fun.txt  struct.txt   macro.txt



示例：

当前路径是在gb_drm_drv下

收集单个文件

```shell
./scripts/code_scrambling/collect.sh kms/device/gbdc_device.c
```

收集kms/device/下所有文件

```shell
./scripts/code_scrambling/collect.sh kms/device/
```





### 第二步：

通过replace.sh来对一个文件或者一个文件下的文件进行加扰替换

```shell
./replace.sh  xxx.c   #xxx.c 表示一个c文件， 也可以是一个头文件
或
./replace.sh  xxx	  #xxx 表示一个文件夹名称
```

默认不对宏进行加扰替换，因为驱动中定义了很多宏和内核中名字一样，替换后会出问题。如果确认自己要处理的文件中的宏都不和内核重复。通过添加第二个参数on可开启对目标文件的加扰替换

```shell
./replace.sh  xxx on  #xxx 表示一个文件夹名称
```

此过程根据生成的fun.txt  struct.txt   macro.txt来进行加扰替换。



note：如果加扰的是个别的文件或者文件夹，其加扰后的函数，可能在其他未加扰的文件中被调用。编译整体代码时有的未加扰文件会出现找不到函数定义的错误。针对这些文件，使用./replace.sh再次进行加扰即可



示例：

当前路径是在gb_drm_drv下

对单个文件进行加扰替换

```shell
./scripts/code_scrambling/replace.sh  kms/device/gbdc_device.c
```

对kms/device/ 所有文件进行加扰替换

```shell
./scripts/code_scrambling/replace.sh  kms/device/
```



## 3: 全加扰脚本

通过使用脚本overall.sh可以对驱动中所有相关的文件进行自动加扰

相关加扰的文件有audio，common，gpu，gpu_test， ip， kms， vpu,  mcu_peripherals

### 1：使用方式：

进入gb_drm_drv目录下执行下面的指令

**单线程加扰：**

逐个对每个文件夹进行加扰

```
./scripts/code_scrambling/overall.sh
```

**多线程加扰：**

启动多线程对每个文件夹进行同时加扰

```
./scripts/code_scrambling/overall.sh on
```



### 2：函数过滤

如果有些特别的拼接函数，例如，需要 GB_FEATURE，DEVICE_ATTR_RO等函数操作的。

可以在 overall.sh添加对应的函数名或者宏名进行剔除

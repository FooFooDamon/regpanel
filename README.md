# 寄存器面板 | Register Panel

## 简介 | Introduction

一款通用的寄存器信息转换工具。更详细地说，它可直观展示已收录的寄存器每一个二进制位的含义及当前值，
或者将已在图形面板调校好的寄存器状态值转换成可放入程序代码的数组项，
因此有助于加速开发人员对各种芯片寄存器的解读和配置工作。

> A generalized tool for register conversion between hexadecimal values and readable descriptions.
More specifically, it's able to visualize the meaning and current value of each bit of registers in database,
or convert the configured (via GUI panel) register status values to array items,
and hence accelerate developers' work of interpreting and configuring registers of all kinds of chips.

## 安装 | Installation

* 推荐使用`Ubuntu 22.04`（附带版本号为`5.15.3`的`Qt`库）。
    > `Ubuntu 22.04` (equipped with `Qt` library of version `5.15.3`) is recommended.

* 编译并安装 | Compile and install：
    ````
    $ cd src
    $
    $ make prepare # Only needed at the first time
    $
    $ make && sudo make install
    ````

## 用法 | Usage

`一图胜千言`。直接看下图即可：

> `A picture paints a thousand words`. See the picture below for most (if not all) details you need:

![HOW_TO_USE](HOW_TO_USE.gif)

## 后续计划 | What's Next

* 支持十进制负数的显示。
    > Support displaying negative decimal numbers.

* 支持`64`位基址。
    > Support `64-bit` address base.

* 持续补充各种芯片的配置文件，但不是改动本项目，而是另立[新项目](https://github.com/FooFooDamon/regpanel-conf)，
届时也欢迎各位热爱开源且有时间精力的人士共同参与。
    > Add more configuration files of all kinds of chips, but they're added to [another project](https://github.com/FooFooDamon/regpanel-conf)
    instead of this one. And, welcome more participants if you guys are interested and have time.

## 许可证 | License

`Apache-2.0`


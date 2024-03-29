# cmake_ocv
Android CMake和OpenCV的简单用法
#### 工程简介
&emsp;&emsp;该工程用CMake编译C++/C代码生成lib库，使用动态注册方式实现Java层调用C++/C层接口，并且调用OpenCV共享库进行简单的图片处理，实现从一张身份证图片截取出身份证号码图片的功能
#### CMake简介
CMake 是一个开源的跨平台自动化构建系统，可生成 native 编译配置文件，在 Linux/Unix 平台，生成makefile，
在Mac 平台，可以生成 xcode，在 Windows 平台，可以生成 MSVC 的工程文件
#### 使用建议
* 如果你的工程只有几个文件，直接编写 Makefile 是最好的选择；
* 如果使用的是 C/C++/Java 之外的语言，请不要使用 CMake；
* 如果你使用的语言有非常完备的构建体系，比如 java 的 ant，也不需要学习 cmake；
* 如果项目已经采用了非常完备的工程管理工具，并且不存在维护问题，没有必要迁移到CMake；  
**CMakeLists.txt 文件是 CMake 的构建定义文件。如果工程存在多个目录，需要在每个要管理的目录都添加一个
CMakeLists.txt 文件。**
#### CMake的常用指令
* cmake_minimum_required(VERSION 3.4.1)  指定CMake最低版本为3.4.1，如果没指定，执行cmake命令时可能会
出错
* include_directories  向工程添加多个特定的头文件搜索路径，路径之间用空格分割，如果路径中包含了空格，可以使
用双引号将它括起来
* message指令，用于向终端输出用户定义的信息，包含了三种类型：SEND_ERROR，产生错误，生成过程被跳过；STATUS，
输出前缀为—-的信息；FATAL_ERROR，立即终止所有 CMake 过程，例如：　
MESSAGE(STATUS "This is Source dir " ${CMAKE_SOURCE_DIR})
* add_executable指令  将一组源文件source生成一个可执行文件。source 可以是多个源文件，也可以是对应定义的变量
如：add_executable(hello main.c)
* add_library指令  语法：add_library(name [SHARED|STATIC|MODULE] [EXCLUDE_FROM_ALL] [source])
将一组源文件source编译出一个库文件，并保存为 libname.so (lib 前缀是生成文件时 CMake自动
添加上去的)。其中有三种库文件类型，不写的话，默认为 STATIC  
   * SHARED: 表示动态库，可以在(Java)代码中使用 System.loadLibrary(name) 动态调用；
   * STATIC: 表示静态库，集成到代码中会在编译时调用；
   * MODULE: 只有在使用 dyId 的系统有效，如果不支持 dyId，则被当作 SHARED 对待；
   * EXCLUDE_FROM_ALL: 表示这个库不被默认构建，除非其他组件依赖或手工构建  
* find_library指令  find_library(<VAR> name1 path1 path2 ...) VAR 变量表示找到的库全路径，包含库文
件名
   * find_library(libX  X11 /usr/lib)
   * find_library(log-lib log)  #路径为空，应该是查找系统环境变量路径  
* set_target_properties 指令  指定要导入的库文件的路径，如：
```java
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../libs
/${ANDROID_ABI}/libopencv_java4.so)
```
* target_link_libraries 指令  这个指令可以用来为target添加需要的链接的共享库，同样也可以用于为自己编写
的共享库添加共享库链接。，如：
```java
target_link_libraries(lib_opencv)
```
#### 工程配置
app目录下的build.gradle配置（省略无关配置）
```java
apply plugin: 'com.android.application'
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++11 -frtti -fexceptions"
                abiFilters 'x86', 'armeabi-v7a', 'arm64-v8a', 'x86_64'
            }
        }
    }
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
        }
    }

    sourceSets {
        main() {
            jniLibs.srcDirs = ['src/main/libs'] //生成的.so库路径
        }
    }
}
```
CMakeList.txt配置
```java
#设置编译 native library 需要最小的 cmake 版本
cmake_minimum_required(VERSION 3.4.1)

include_directories(include)

file(GLOB my_source_path ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.c)

#将指定的源文件编译为名为 libfunction_control.so的动态库
add_library(function_control SHARED ${my_source_path})

add_library(lib_opencv SHARED IMPORTED)
add_library(lib_share SHARED IMPORTED)

set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI}/libopencv_java4.so)
set_target_properties(lib_share PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI}/libc++_shared.so)

#查找本地 log 库
find_library(log-lib log)

#将预构建的库添加到自己的原生库
target_link_libraries(function_control jnigraphics ${log-lib} lib_opencv lib_share)
```
#### 接口文件定义和调用
```java
package soft.wl.function;

public class FunctionControl {
    static {
        System.loadLibrary("function_control");
    }

    public FunctionControl() {
    }

    native public Object sendCommand(int cmd , Object in, Object out);
}

//接口调用

mOut = mFunctionControl.sendCommand(1024, mBitmap, Bitmap.Config.ARGB_8888);
```
#### 身份证号码识别
* 归一化&emsp;&emsp;&emsp;&emsp;身份证图片设置为统一大小
* 灰度化&emsp;&emsp;&emsp;&emsp;RGB颜色值转化为灰度值
* 二值化&emsp;&emsp;&emsp;&emsp;图片颜色值转化为0或者250，即非黑即白
* 膨胀处理&emsp;&emsp;&emsp;将图片的黑色区域沿X方向扩大，方便查找身份证号码区域的位置和大小
* 轮廓检测&emsp;&emsp;&emsp;查找图片区域的所有轮廓
* 轮廓逻辑处理&emsp;逻辑判断，确定身份证号码所在的轮廓
* 裁剪图片&emsp;&emsp;&emsp;将身份证号码截图用于显示

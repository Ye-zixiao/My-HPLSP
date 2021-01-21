 

## 可变参函数：stdarg.h介绍

为了使用stdarg.h提供的可变参功能需要以如下的步骤进行操作：

#### 1.提供一个带省略号的函数原型

且该函数原型中必须至少有一个形参和一个省略号，形参列表中的最后一个形参（不包括省略号）起到了对省略号后可变参参数列表的定位功能。例如：

```c
int func(int val,const char*buf,...);
//其中参数buf起到了定位的功能
```

#### 2.在函数定义中创建一个va_list类型的变量

该参数会用来存储实际的可变参参数列表

#### 3.使用函数宏va_start()初始化一个可变参参数列表

函数原型：`void va_start(va_list ap, last);`

va_start()函数宏会将函数中的可变参参数列表存储到指定的va_list类型变量之中，其中第一个参数指出va_list变量，第二个参数指定函数原型中的定位形参。例如：

```c
va_list ap;
va_start(ap,parmN);
```

#### 4.使用函数宏va_arg()获取可变参数列表中指定位置的参数

函数原型：`type va_arg(va_list ap, type)`

va_arg()函数宏接受一个va_list类型变量和一个类型，然后每调用一次返回下一个未取的可变参数列表中的参数，注意这个给出的类型必须是与实际可变参数列表中的实参类型相同（期间是不会发生自动类型转换的！所以若不一致就会发生错误）。例如：

```c
int ival=va_arg(ap,int);//取出可变参数列表中的第一个参数，并将其解释为int
double dval=va_arg(ap,double);//取出可变参数列表中的第二个参数，并将其解释为double
```

#### 5.使用函数宏va_end()将为处理可变参数生成的动态内存进行销毁

函数原型：`void va_end(va_list ap)`

va_end()函数宏接受一个va_list类型的变量，然后将为该可变参数列表处理过程创建的动态内存进行销毁，并将其归还给系统。

#### 6.函数实例

```c
int func(const char*str,...){
    va_list ap;
    int ret;
    
    va_start(ap,str);
    ret=va_arg(ap,int);
    va_end(ap);
    return ret;
}
```


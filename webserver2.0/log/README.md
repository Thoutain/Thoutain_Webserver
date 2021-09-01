# 同步/异步日志系统

日志系统分为两部分，其一是单例模式与阻塞队列的定义，其二是日志类的定义与使用

同步/异步日志系统一共实现了两个模块，日志模块和阻塞队列模块，其中阻塞队列模块主要是为解决异步写入日志做准备。
* 自定义阻塞队列
* 单例模式创建日志
* 同步日志
* 异步日志
* 实现按天、超行分类

同步：
* 判断是否分文件
* 直接把内容处理好并写入日志

异步：
* 判断是否分文件
* 格式化日志内容，写入阻塞队列里面，然后创建一个写线程来从阻塞队列中去除内容并写入日志

```C++
class single{
private:
    //私有静态指针变量指向唯一实例
    static single *p;

    //静态锁，是由于静态函数只能访问静态成员
    static pthread_mutex_t lock;

    single(){
        pthread_mutex_init(&lock, NULL);
    }
    ~single(){}
public:
    //公有静态方法获取实例
    static single* getinstance();

};

pthread_mutex_t single::lock;

single* single::p = NULL;
single* single::getinstance(){
    if (NULL == p){
        pthread_mutex_lock(&lock);
        if (NULL == p){
            p = new single;
        }
        pthread_mutex_unlock(&lock);
    }
    return p;
}

-----------
// C++11
class single{
private:
    single(){}
    ~single(){}

public:
    static single* getinstance();

};

single* single::getinstance(){
    static single obj;
    return &obj;
}
```


#include <libubox/uloop.h>
#include <stdio.h>
struct uloop_timeout timeout;	//创建uloop_timeout全局变量

int frequency = 5; //每隔5秒超时一次

static void timeout_cb(struct uloop_timeout *t)
{
    //do your work
    puts("What is going on?");
    uloop_timeout_set(t, frequency * 1000);//设置下次的超时时间
}

int main()
{
    uloop_init();			//使用库初始化

    timeout.cb = timeout_cb;

    uloop_timeout_set(&timeout, frequency * 1000);//设置下次的超时时间

    uloop_run();
}
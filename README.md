学习linux管道的工程
pipe创建两条管道，
fork创建一个子进程，在子进程中启动/bin/bash，实现输入输出的重定向，通过管道在父子进程中传输数据。
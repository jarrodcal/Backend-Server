A Simple Echo Server

A Simple Echo Server但不仅仅是个Echo Server，更像个网络框架，包含线程设计，缓冲区，连接处理，日志，xxxxxx

采用Master+Workers+辅助线程的架构，Master线程接收客户端连接，Workers处理和客户端的实际交互，辅助线程包含写日志，查看系统状态。

主线程和工作线程都采用异步非阻塞的模式，EPOLL的边缘触发模式。

TODO：连接池，工作线程创建新的连接
##Hashtable设计思想：
* 首先创建一个初始大小Bucket，只分配指针大小不分配实际空间。当有数据Insert时候将entry(需要Malloc)插入到Bucket的index， 当冲突达到一定rate时，需要*2扩容，同时将已有的hash重新规划。
*示意图：

Bucket
----
----    entry entry
----    entry 
----    entry entry entry entry
----
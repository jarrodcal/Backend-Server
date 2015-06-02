##Hashtable设计思想：
* 首先创建一个初始大小Bucket，只分配指针大小不分配实际空间。当有数据Insert时候将entry(需要Malloc)插入到Bucket的index， 当冲突达到一定rate时，需要*2扩容，同时将已有的Rehash。
* 当数据量少时，hashtable中的key并不多，可以很好的起到快速定位，但当key较多时，会使得在数据存在rehash的过程.

###示意图：

Bucket
----
----    entry entry
----    entry 
----    entry entry entry entry

##hashtable的另外设计思想
M*N为最大的数据量，要使得N稍小。
M个bucket，数据hash到指定的bucket, 每个bucket的数据直接用链表串联，链表数目为N

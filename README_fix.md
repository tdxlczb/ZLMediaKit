# 为了方便查看自己的改动，每次同步源zlm代码时，使用rebase，而不是merge，同时记录源zlm的commit
commit: 8c9439571032a0db7d678a0104443125dc0d6916
commit: 37ea51a2ec35ca57e4f3f18c972f2b01dfb577aa

# 如果使用zlm的动态库，main函数结束时，进程无法退出

临时做法：
找到EventPoller的析构函数，EventPoller::~EventPoller() 
将 close_event(_event_fd) 替换为 close((int)_event_fd) ，
有一定的风险，这种做法在析构EventPoller时，没有释放epoll


# 调用RtspPlayer::teardown低概率死锁
Socket::onRead函数的_on_multi_read回调中不能同步调用Socket::send，否则可能会出现死锁；
Socket::onRead函数执行_on_multi_read回调时，会锁住_mtx_event；
如果内部调用了Socket::send，当执行到Socket::flushAll中的LOCK_GUARD(_mtx_sock_fd)，等待解锁时，如果刚好另一个线程调用RtspPlayer::teardown执行到Socket::flushData中的LOCK_GUARD(_mtx_event)时，_mtx_event和_mtx_sock_fd互相死锁。

同样的当执行到Socket::send_l中的LOCK_GUARD(_mtx_send_buf_waiting)，等待解锁时，如果刚好另一个线程调用RtspPlayer::teardown执行到Socket::flushData中的LOCK_GUARD(_mtx_event)时，_mtx_event和_mtx_send_buf_waiting互相死锁。

因此要注意，Socket::onRead函数中不能同步调用Socket::send，建议外面异步调用Socket::send
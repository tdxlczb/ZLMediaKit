如果使用zlm的动态库，main函数结束时，进程无法退出

临时做法：
找到EventPoller的析构函数，EventPoller::~EventPoller() 
将 close_event(_event_fd) 替换为 close((int)_event_fd) ，
有一定的风险，这种做法在析构EventPoller时，没有释放epoll
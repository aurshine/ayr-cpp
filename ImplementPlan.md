# 实现计划

- [ ] set函数，接收一个迭代器范围或可迭代对象，返回Set
- [ ] Sequence实现统一的删除函数，可传入`std::function<bool(const T&, int)>`判断删除
- [ ] Sequence实现统一修改函数，可传入`std::function<void(T&)>`传入修改逻辑
- [ ] Encoding实现int转CodePoint和CodePoint转int函数
- [ ] IOCP实现的windows平台的UltraTcpServer
- [ ] 模仿python asyncio实现协程库coro
- [ ] 文件系统库fs
- [ ] 并发算法库async
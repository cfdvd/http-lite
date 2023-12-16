轻量级 HTTP Server, 使用 C++ (11 ~ 20) 标准库以及 linux socket API 实现. 

* http 头部, 以及 http 参数采用正则表达式解析
* 使用包装的 std::unique_ptr 作为文件指针的替代
* 采用多态的方式实现特殊 http packet (code: 404, 501...) 的发送
* 对原始 socket API 进行了简单的包装

由于本人是网络编程初学者, 因此目前仅实现了 GET 功能, 后续功能会在之后的学习中补充

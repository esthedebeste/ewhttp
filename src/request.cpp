#include <ewhttp/server.h>
#include <iostream>

asio::awaitable<void> ewhttp::Request::reply(std::string body) {
	asio::streambuf b;
	std::ostream os(&b);
	os << "HTTP/1.1 200 OK\r\n"
	   << "Content-Type: text/html; charset=UTF-8\r\n"
	   << "Content-Length: " << body.length() + 2 << "\r\n\r\n"
	   << body << "\r\n";
	co_await asio::async_write(context->socket, b, asio::use_awaitable);
}
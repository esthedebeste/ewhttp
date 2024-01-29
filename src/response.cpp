#include "ewhttp/response.h"
#include "ewhttp/request.h"
#include "ewhttp/server.h"

namespace ewhttp {
	async Response::send_headers() {
		assert(!headers_sent);
		asio::streambuf b;
		std::ostream os(&b);
		os << "HTTP/1.1 " << status.code << ' ' << status.name() << "\r\n";
		for (const auto &[key, value] : headers) {
			os << key << ": " << value << "\r\n";
		}
		os << "\r\n";
		co_await asio::async_write(context.socket, b, asio::use_awaitable);
		headers_sent = true;
	}

	async Response::send_body(const std::span<const char> &body) {
		assert(!body_sent);
		if (!headers_sent) {
			headers.emplace_back("Content-Length", std::to_string(body.size_bytes()));
			co_await send_headers();
		}
		co_await asio::async_write(context.socket, asio::buffer(body), asio::use_awaitable);
		body_sent = true;
	}
} // namespace ewhttp
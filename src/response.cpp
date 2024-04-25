#include "ewhttp/response.h"
#include "ewhttp/request.h"
#include "ewhttp/server.h"


#include <iostream>
namespace ewhttp {
	async Response::send_headers() {
		assert(!headers_sent);
		asio::streambuf b;
		std::ostream os(&b);
		os << "HTTP/1.1 " << status.code << ' ' << status.name() << "\r\n";
		for (const auto &[key, values] : headers)
			for (const auto &value : values)
				os << key << ": " << value << "\r\n";
		os << "\r\n";
		co_await asio::async_write(context.socket, b, asio::use_awaitable);
		headers_sent = true;
	}

	async Response::send_body(const std::span<const unsigned char> &body) {
		assert(!body_sent);
		if (!headers_sent) {
			set_header("Content-Length", std::to_string(body.size_bytes()));
			co_await send_headers();
		}
		co_await asio::async_write(context.socket, asio::buffer(body), asio::use_awaitable);
		body_sent = true;
	}
	async Response::send_body(const std::span<const char> &body) {
		assert(!body_sent);
		if (!headers_sent) {
			set_header("Content-Length", std::to_string(body.size_bytes()));
			co_await send_headers();
		}
		co_await asio::async_write(context.socket, asio::buffer(body), asio::use_awaitable);
		body_sent = true;
	}
	async Response::send_body(std::istream &body, size_t size) {
		assert(!body_sent);
		if (!headers_sent) {
			set_header("Content-Length", std::to_string(size));
			co_await send_headers();
		}
		asio::streambuf b;
		std::ostream os(&b);
		os << body.rdbuf();
		co_await asio::async_write(context.socket, b, asio::use_awaitable);
		body_sent = true;
	}

	async Response::send_body(std::istream &body) {
		assert(!body_sent);
		if (!headers_sent) {
			set_header("Transfer-Encoding", "chunked");
			co_await send_headers();
		}
		while (body.good()) {
			std::array<char, 1024 * 8> buffer;
			body.read(buffer.data(), buffer.size());
			auto amount = body.gcount();
			asio::streambuf b;
			std::ostream os(&b);
			os << std::hex << amount << "\r\n";
			co_await asio::async_write(context.socket, b, asio::use_awaitable);
			co_await asio::async_write(context.socket, asio::buffer(buffer, amount), asio::use_awaitable);
			co_await asio::async_write(context.socket, asio::buffer("\r\n", 2), asio::use_awaitable);
		}
		co_await asio::async_write(context.socket, asio::buffer("0\r\n\r\n", 5), asio::use_awaitable);
		if (!body.eof()) // not good, no eof
			throw std::runtime_error("Error reading from stream");
		body_sent = true;
	}

	void Response::add_header(std::string_view key, std::string_view value) {
		auto found = headers.find(key);
		if (found == headers.end())
			headers.emplace(key, std::vector<std::string>{std::string{value}});
		else
			found->second.emplace_back(value);
	}
	bool Response::has_header(std::string_view key) const {
		return headers.find(key) != headers.end();
	}
	std::vector<std::string> &Response::get_header(std::string_view key) {
		auto found = headers.find(key);
		if (found == headers.end())
			return headers.emplace(key, std::vector<std::string>{}).first->second;
		else
			return found->second;
	}
	bool Response::remove_header(std::string_view key) {
		auto found = headers.find(key);
		if (found == headers.end())
			return false;
		headers.erase(found);
		return true;
	}
	bool Response::set_header(std::string_view key, std::string_view value) {
		bool overwritten = remove_header(key);
		add_header(key, value);
		return overwritten;
	}
} // namespace ewhttp
#pragma once
#include "./method.h"

#include <asio.hpp>
#include <string_view>
#include <vector>

namespace ewhttp {
	template<class T = void>
	using awaitable = asio::awaitable<T>;
	using async = asio::awaitable<void>;

	class Server;
	namespace detail {
		struct RequestContext; // server.h
	}
	struct Request {
		MethodT method;
		std::vector<std::pair<std::string, std::string>> headers{};
		std::string path{};

		asio::awaitable<void> reply(std::string body);

	private:
		detail::RequestContext *context;

		Request(MethodT method, detail::RequestContext *context) : method{method}, context{context} {}
		Request &operator=(const Request &) = default;
		friend class Server;
	};

	using Req = Request &;
} // namespace ewhttp
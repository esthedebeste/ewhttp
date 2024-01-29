#pragma once
#include "./method.h"
#include "./request.h"
#include "./status.h"
#include <span>
#include <string_view>
#include <vector>

namespace ewhttp {
	struct Response {
		StatusT status{200};
		std::vector<std::pair<std::string, std::string>> headers{};
		bool headers_sent{}, body_sent{};

		async send_headers();
		async send_body(const std::span<const char> &body);

	private:
		detail::RequestContext &context;

		explicit Response(Request &request) : context{*request.context} {}
		explicit Response(detail::RequestContext &context) : context{context} {}
		friend struct Request;
		friend class Server;
	};

	using Res = Response &;
} // namespace ewhttp
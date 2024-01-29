#include <ewhttp/request.h>
#include <ewhttp/server.h>
#include <iostream>
#include <llhttp.h>

asio::awaitable<void> ewhttp::Server::respond(asio::ip::tcp::socket socket_param) {
	llhttp_t parser;
	llhttp_settings_t settings;
	llhttp_settings_init(&settings);
	using detail::RequestContext;

	settings.on_reset = +[](llhttp_t *parser) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		locals.request = Request{{255}, &locals};
		return 0;
	};

	settings.on_method = +[](llhttp_t *parser, const char *data, const size_t amount) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		locals.method += std::string_view{data, amount};
		return 0;
	};

	settings.on_url = +[](llhttp_t *parser, const char *data, const size_t amount) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		locals.request.path += std::string_view{data, amount};
		return 0;
	};

	settings.on_header_field = +[](llhttp_t *parser, const char *data, const size_t amount) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		auto &request = locals.request;
		if (request.headers.empty() || !request.headers.back().second.empty())
			request.headers.emplace_back();
		request.headers.back().first += std::string_view{data, amount};
		return 0;
	};

	settings.on_header_value = +[](llhttp_t *parser, const char *data, const size_t amount) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		locals.request.headers.back().second += std::string_view{data, amount};
		return 0;
	};

	settings.on_method_complete = +[](llhttp_t *parser) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		if (const auto method = Method::from_string(locals.method)) {
			locals.request.method = *method;
			locals.method.clear();
		} else {
			locals.method.clear();
			return 1;
		}
		return 0;
	};

	settings.on_headers_complete = +[](llhttp_t *parser) {
		auto &locals = *static_cast<RequestContext *>(parser->data);
		asio::co_spawn(
				locals.executor, [&]() -> async {
					Request request = std::move(locals.request);
					locals.request = Request{{255}, &locals};
					Response response{locals};
					co_await locals.callback(request, response);
				},
				asio::detached);
		return 0;
	};

	llhttp_init(&parser, HTTP_REQUEST, &settings);
	RequestContext locals{
			Request{{255}, &locals},
			callback,
			"",
			std::move(socket_param),
			io_executor};
	parser.data = &locals;
	auto &socket = locals.socket;

	for (;;) {
		char data_buf[1024];
		std::size_t n = co_await socket.async_read_some(asio::buffer(data_buf), asio::use_awaitable);
		std::string_view data{data_buf, n};
		if (auto result = llhttp_execute(&parser, data.data(), data.length()); result == HPE_OK) {
		} else if (result == HPE_PAUSED_UPGRADE) {
			llhttp_resume_after_upgrade(&parser); // ignore upgrade
		} else if (result != HPE_PAUSED) {
			break;
		}
	}

	//std::string response = "yo!";

	//asio::streambuf b;
	//std::ostream os(&b);
	//os << "HTTP/1.1 200 OK\r\n"
	//   << "Content-Type: text/html; charset=UTF-8\r\n"
	//   << "Content-Length: " << response.length() + 2 << "\r\n\r\n"
	//   << response << "\r\n";
	//co_await asio::async_write(socket, b, asio::use_awaitable);
}

namespace ewhttp {
	void Server::run(const asio::ip::address &host, const uint16_t port) {
		auto executor = io_executor = asio::require(io_context.get_executor(), asio::execution::outstanding_work_t::tracked);
		co_spawn(
				io_context,
				[host, port, executor, this]() -> asio::awaitable<void> {
					asio::ip::tcp::acceptor acceptor(executor, {host, port});
					for (;;) {
						asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
						asio::co_spawn(executor, respond(std::move(socket)), asio::detached);
					}
				}(),
				asio::detached);

		io_context.run();
	}
	void Server::run(const std::string_view host, const uint16_t port) {
		run(asio::ip::make_address(host), port);
	}

	void Server::force_stop() {
		io_context.stop();
	}

	void Server::stop() {
		io_executor = asio::any_io_executor{};
	}
} // namespace ewhttp
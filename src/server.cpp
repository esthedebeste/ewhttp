#include <ewhttp/request.h>
#include <ewhttp/server.h>
#include <iostream>
#include <llhttp.h>

namespace {
	using ewhttp::detail::RequestContext;
	// llhttp callback wrappers
	template<int (*Callback)(RequestContext &)>
	int cb(llhttp_t *parser) {
		return Callback(*static_cast<RequestContext *>(parser->data));
	}
	template<int (*Callback)(RequestContext &, std::string_view)>
	int data_cb(llhttp_t *parser, const char *data, const size_t amount) {
		return Callback(*static_cast<RequestContext *>(parser->data),
						std::string_view{data, amount});
	}
} // namespace

asio::awaitable<void>
ewhttp::Server::respond(asio::ip::tcp::socket socket_param) {
	llhttp_t parser;
	llhttp_settings_t settings;
	llhttp_settings_init(&settings);
	using detail::RequestContext;

	settings.on_reset = cb<[](RequestContext &locals) {
		locals.request = Request{{255}, &locals};
		return 0;
	}>;

	settings.on_method =
			data_cb<[](RequestContext &locals, std::string_view data) {
				locals.method += data;
				return 0;
			}>;

	settings.on_url = data_cb<[](RequestContext &locals, std::string_view data) {
		locals.request.path += data;
		return 0;
	}>;

	settings.on_header_field =
			data_cb<[](RequestContext &locals, std::string_view data) {
				auto &request = locals.request;
				if (request.headers.empty() || !request.headers.back().second.empty())
					request.headers.emplace_back(data, "");
				else
					request.headers.back().first += data;
				return 0;
			}>;

	settings.on_header_value =
			data_cb<[](RequestContext &locals, std::string_view data) {
				locals.request.headers.back().second += data;
				return 0;
			}>;

	settings.on_method_complete = cb<[](RequestContext &locals) {
		if (const auto method = Method::from_string(locals.method)) {
			locals.request.method = *method;
			locals.method.clear();
		} else {
			locals.method.clear();
			return 1;
		}
		return 0;
	}>;

	settings.on_headers_complete = cb<[](RequestContext &locals) {
		asio::co_spawn(
				locals.executor,
				[&]() -> async {
					Request request = std::move(locals.request);
					locals.request = Request{{255}, &locals};
					Response response{locals};
					co_await locals.callback(request, response);
					if (!response.headers_sent) {
						std::cerr << "[EWHTTP]: Nothing Sent?\n";
					}
					co_return;
				},
				asio::detached);
		return 0;
	}>;

	llhttp_init(&parser, HTTP_REQUEST, &settings);
	RequestContext locals{Request{{255}, &locals}, callback, "",
						  std::move(socket_param), io_executor};
	parser.data = &locals;
	auto &socket = locals.socket;

	for (;;) {
		char data_buf[1024];
		std::size_t n = co_await socket.async_read_some(asio::buffer(data_buf),
														asio::use_awaitable);
		std::string_view data{data_buf, n};
		if (auto result = llhttp_execute(&parser, data.data(), data.length());
			result == HPE_OK) {
		} else if (result == HPE_PAUSED_UPGRADE) {
			llhttp_resume_after_upgrade(&parser); // ignore upgrade
		} else if (result != HPE_PAUSED) {
			break;
		}
	}
}

namespace ewhttp {
	void Server::run(const asio::ip::address host, const uint16_t port) {
		auto &executor = io_executor = asio::require(
				io_context.get_executor(), asio::execution::outstanding_work_t::tracked);
		co_spawn(
				io_context,
				[this](asio::any_io_executor &executor, const asio::ip::address host, const uint16_t port) -> asio::awaitable<void> {
					asio::ip::tcp::acceptor acceptor(executor, {host, port});
					for (;;) {
						asio::ip::tcp::socket socket =
								co_await acceptor.async_accept(asio::use_awaitable);
						asio::co_spawn(executor, respond(std::move(socket)), asio::detached);
					}
				}(executor, host, port),
				asio::detached);

		io_context.run();
	}
	void Server::run(const std::string_view host, const uint16_t port) {
		run(asio::ip::make_address(host), port);
	}

	void Server::force_stop() { io_context.stop(); }

	void Server::stop() { io_executor = asio::any_io_executor{}; }
} // namespace ewhttp
#include <ewhttp/ewhttp.h>
#include <iostream>

int main(int argc, char **argv) {
	std::vector<std::string_view> args(argv, argv + argc);
	std::string_view host = "0.0.0.0";
	int port = 80;
	for (auto it = args.begin(); it != args.end(); ++it) {
		if (*it == "-p" || *it == "--port") {
			if (++it == args.end()) {
				std::cerr << "No port specified after " << args.back() << std::endl;
				return 1;
			}
			const auto result =
					std::from_chars(it->data(), it->data() + it->size(), port);
			if (result.ec != std::errc{}) {
				std::cerr << "Invalid number '" << *it
						  << "': " << std::make_error_code(result.ec).message()
						  << std::endl;
				return 1;
			}
		}
		if (*it == "-h" || *it == "--host") {
			if (++it == args.end()) {
				std::cerr << "No host specified after " << args.back() << std::endl;
				return 1;
			}
			host = *it;
		}
		if (*it == "--help") {
			std::cout << "Usage: " << args[0] << " [-p|--port PORT] [-h|--host HOST]"
					  << std::endl;
			return 0;
		}
	}

	using namespace std::literals;
	using namespace ewhttp::build::underscore;
	using ewhttp::async;
	using ewhttp::awaitable;
	using ewhttp::awaitopt;
	using ewhttp::Req;
	using ewhttp::Res;

#define PREFIX "<h1><a href=\"https://github.com/esthedebeste/ewhttp/\">ewhttp</a> test server</h1>"
#define POSTFIX "<br><br><br>Links: <ul>"                                         \
				"<li><a href=\"/\">/</a>"                                         \
				"<li><a href=\"/other/nested\">/other/nested</a>"                 \
				"<li><a href=\"/number/12345\">/number/[number]</a>"              \
				"<li><a href=\"/ewhttp/name\">/[name]/name</a>"                   \
				"<li><a href=\"/files/hai.txt\">/files/hai.txt</a>"               \
				"<li><a href=\"/files/stream/hai.txt\">/files/stream/hai.txt</a>" \
				"</ul>"
	const auto router = ewhttp::create_router(
			_([](Req request, Res response) {
				response.add_header("Content-Type", "text/html");
			}),
			_.fallback([](Req request, Res response) -> async {
				response.status = 404;
				co_await response.send_body(
						std::format(PREFIX "404 Not Found '{}' :P" POSTFIX, request.path));
			}),
			GET([](Req request, Res response) -> async {
				co_await response.send_body(
						std::format(PREFIX "Welcome to the test server for <a "
										   "href=\"https://github.com/esthedebeste/ewhttp/"
										   "\">ewhttp</a>! You visited {}" POSTFIX,
									request.path));
			}),
			_("other", _("nested", GET([](Req request, Res response) -> async {
							 co_await response.send_body(
									 std::format(PREFIX "You got a little deeper! You visited {}" POSTFIX,
												 request.path));
						 }))),
			_("number",
			  _(
					  [](const std::string_view str, Req request,
						 Res response) -> awaitopt<int> {
						  int n;
						  const auto result =
								  std::from_chars(str.data(), str.data() + str.size(), n);
						  if (result.ec == std::errc{})
							  co_return std::make_optional(n);
						  co_await response.send_body(
								  std::format(PREFIX "Invalid number '{}': {}" POSTFIX, str,
											  std::make_error_code(result.ec).message()));
						  co_return std::nullopt;
					  },
					  GET([](Req request, Res response, const int number) -> async {
						  co_await response.send_body(
								  std::format(PREFIX "Test: your number is {}" POSTFIX, number));
					  }))),
			_([](const std::string_view str) { return str; },
			  _("name", GET([](Req request, Res response,
							   const std::string_view &part) -> async {
					co_await response.send_body(
							std::format(PREFIX "Test: your name is {}" POSTFIX, part));
				}))),
			_("files",
			  _.files("./test/files", {}),
			  _("stream", _.files("./test/files", {0}))));
	ewhttp::Server server(router);
	std::cout << "Listening on " << host << ":" << port << std::endl;
	server.run(host, port);
	return 0;
}
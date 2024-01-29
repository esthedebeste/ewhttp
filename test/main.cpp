#include <ewhttp/ewhttp.h>
#include <iostream>

int main() {
	using namespace std::literals;
	using namespace ewhttp::build::underscore;
	using ewhttp::async;
	using ewhttp::awaitable;
	using ewhttp::Req;
	using ewhttp::Res;

	std::vector<uint8_t> file;


	const auto router = ewhttp::create_router(
			_([](Req request, Res response) {
				response.headers.push_back({"Content-Type", "text/html"});
			}),
			_.fallback([](Req request, Res response) -> async {
				response.status = 404;
				co_await response.send_body(std::format("404 Not Found '{}' :P", request.path));
			}),
			GET([](Req request, Res response) -> async {
				co_await response.send_body(std::format("Welcome to the test! You visited {}", request.path));
			}),
			_("other",
			  _("nested", GET([](Req request, Res response) -> async {
					co_await response.send_body(std::format("You got a little deeper! You visited {}", request.path));
				}))),
			_("number",
			  _([](const std::string_view str, Req request, Res response) -> awaitable<std::optional<int>> {
				  int n;
				  if (const auto result = std::from_chars(str.data(), str.data() + str.size(), n); result.ec == std::errc{})
					  co_return std::make_optional(n);
				  co_await response.send_body("Invalid number");
				  co_return std::nullopt;
			  },
				GET([](Req request, Res response, const int number) -> async {
					co_await response.send_body(std::format("Test: your number is {}", number));
				}))),
			_([](const std::string_view str) {
				return str;
			},
			  _("name", GET([](Req request, Res response, const std::string_view &part) -> async {
					co_await response.send_body(std::format("Test: your name is {}", part));
				}))));
	ewhttp::Server server(router);
	server.accept("127.0.0.1", 8080);
	return 0;
}
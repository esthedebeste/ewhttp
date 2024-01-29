#include <ewhttp/ewhttp.h>

int main() {
	using namespace std::literals;
	using namespace ewhttp::build::underscore;
	using ewhttp::async;
	using ewhttp::awaitable;
	using ewhttp::awaitopt;
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
				co_await response.send_body(std::format("Welcome to the test server for <a href=\"https://github.com/esthedebeste/ewhttp/\">ewhttp</a>! You visited {}", request.path));
			}),
			_("other",
			  _("nested", GET([](Req request, Res response) -> async {
					co_await response.send_body(std::format("You got a little deeper! You visited {}", request.path));
				}))),
			_("number",
			  _([](const std::string_view str, Req request, Res response) -> awaitopt<int> {
				  int n;
				  const auto result = std::from_chars(str.data(), str.data() + str.size(), n);
				  if (result.ec == std::errc{})
					  co_return std::make_optional(n);
				  co_await response.send_body(std::format("Invalid number '{}': {}", str, std::make_error_code(result.ec).message()));
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
	server.run("0.0.0.0", 80);
	return 0;
}
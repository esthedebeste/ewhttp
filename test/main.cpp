#include <ewhttp/ewhttp.h>
#include <iostream>

int main() {
	using namespace std::literals;
	using namespace ewhttp::build::underscore;
	using ewhttp::async;
	using ewhttp::Req;

	const auto router = ewhttp::create_router(
			_("path",
			  GET([](Req request) -> async {
				  co_await request.reply(std::format("Welcome to the test! You visited {}", request.path));
			  })),
			_("other",
			  _("nested", GET([](Req request) -> async {
					co_await request.reply(std::format("You got a little deeper! You visited {}", request.path));
				}))),
			_("number",
			  _([](const std::string_view str, Req request) -> std::optional<int> {
				  int n;
				  if (const auto result = std::from_chars(str.data(), str.data() + str.size(), n); result.ec == std::errc{})
					  return n;
				  else {
					  // TODO PARSERS MOETEN OOK ASYNC ZIJN WAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHH
					  // request.reply("Invalid number");
					  return 0;
				  }
			  },
				GET([](Req request, const int number) -> async {
					co_await request.reply(std::format("Test: your number is {}", number));
				}))),
			_([](const std::string_view str) {
				return str;
			},
			  _("name", GET([](Req request, const std::string_view &part) -> async {
					co_await request.reply(std::format("Test: your name is {}", part));
				}))));
	ewhttp::Server server(router);
	server.accept("127.0.0.1", 8080);
	return 0;
}
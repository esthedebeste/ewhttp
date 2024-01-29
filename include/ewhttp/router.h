#pragma once
#include "./request.h"

#include <asio/awaitable.hpp>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace ewhttp {
	namespace detail {
		template<class T>
		concept std_optional = requires(T &t) {
			static_cast<bool>(t.has_value());
			static_cast<typename T::value_type>(t.value());
		};

		template<class Handler, class PartsTuple>
		struct HandlerVerifier;
		template<class Handler, class... Parts>
		struct HandlerVerifier<Handler, std::tuple<Parts...>> {
			static constexpr bool value = requires(Handler h, Request &request, Parts... parts) {
				h(request, parts...);
			};
		};

		template<class Tuple>
		concept tuple_like = requires(Tuple t) {
			static_cast<std::size_t>(std::tuple_size_v<Tuple>);
		} && (std::tuple_size_v<Tuple> == 0 || requires(Tuple t) {
								 static_cast<typename std::tuple_element_t<0, Tuple>>(std::get<0>(t));
							 });
#define EWHTTP_CONCEPT_LAMBDA(name) []<class T>() consteval { return name<T>; }
		template<auto C, class Tuple>
		struct Count;
		template<auto C>
		struct Count<C, std::tuple<>> {
			static constexpr std::size_t value = 0;
		};
		template<auto C, class E, class... Types>
		struct Count<C, std::tuple<E, Types...>> {
			static constexpr std::size_t value = C.template operator()<E>() + Count<C, std::tuple<Types...>>::value;
		};
		template<class E, auto C>
		concept every = Count<C, E>::value == std::tuple_size_v<E>;

		template<auto C, class Tuple>
		struct IndexOf;
		template<auto C>
		struct IndexOf<C, std::tuple<>> {
			static constexpr std::size_t value = 0;
		};
		template<auto C, class E, class... Types>
		struct IndexOf<C, std::tuple<E, Types...>> {
			static constexpr std::size_t value = C.template operator()<E>() ? 0 : 1 + IndexOf<C, std::tuple<Types...>>::value;
		};
	} // namespace detail

	template<class H>
	concept path_parser_with_early = requires(H h, std::string_view path, Request &request) {
		{ h(path, request) } -> detail::std_optional;
	};
	template<class H>
	concept path_parser = path_parser_with_early<H> || requires(H h, std::string_view path) {
		h(path);
	};
	template<class H>
	concept optional_path_parser = path_parser<H> || std::is_same_v<H, std::monostate>;

	namespace build {
		template<class P>
		concept parser_c = requires(P p) {
			static_cast<typename P::parser_type>(p.parser);
			static_cast<typename P::routes_type>(p.routes);
		};
		template<class N>
		concept name_c = requires(N n) {
			static_cast<std::string_view>(n.name);
			static_cast<typename N::routes_type>(n.routes);
		};
		template<class H>
		concept handler_c = requires(H h) {
			static_cast<MethodT>(h.method);
			static_cast<typename H::handler_type>(h.handler);
		};
		template<class R>
		concept route_c = parser_c<R> || name_c<R> || handler_c<R>;
		template<class R>
		concept router_c = detail::tuple_like<typename R::routes_type> && detail::every<typename R::routes_type, EWHTTP_CONCEPT_LAMBDA(route_c)>;

		template<route_c... Routes>
		struct Router {
			using routes_type = std::tuple<Routes...>;
			std::tuple<Routes...> routes;
			explicit Router(Routes... routes) : routes(std::make_tuple(routes...)) {}
		};
		template<path_parser P, route_c... R>
		struct Parser {
			using parser_type = P;
			using routes_type = Router<R...>;
			P parser;
			Router<R...> routes;
			explicit Parser(P parser, R... routes) : parser(parser), routes(Router{routes...}) {}
		};
		template<path_parser P, route_c... Rs>
		Parser(P, Rs...) -> Parser<P, Rs...>;
		template<route_c... R>
		struct Name {
			using routes_type = Router<R...>;
			std::string_view name;
			Router<R...> routes;
			explicit Name(const std::string_view name, R... routes) : name(name), routes(Router{routes...}) {}
		};
		template<route_c... Rs>
		Name(const std::string_view, Rs...) -> Name<Rs...>;

		template<class H>
		struct Handler {
			using handler_type = H;
			MethodT method;
			H handler;
			Handler(const MethodT &method, H handler) : method(method), handler(handler) {}
		};

		namespace handlers_only {
			template<class H>
			Handler<H> GET(H handler) { return Handler{Method::GET, handler}; }
			template<class H>
			Handler<H> HEAD(H handler) { return Handler{Method::HEAD, handler}; }
			template<class H>
			Handler<H> POST(H handler) { return Handler{Method::POST, handler}; }
			template<class H>
			Handler<H> PUT(H handler) { return Handler{Method::PUT, handler}; }
#pragma push_macro("DELETE")
#undef DELETE
			template<class H>
			Handler<H> DELETE(H handler) { return Handler{Method::DELETE, handler}; }
#pragma pop_macro("DELETE")
			template<class H>
			Handler<H> CONNECT(H handler) { return Handler{Method::CONNECT, handler}; }
			template<class H>
			Handler<H> OPTIONS(H handler) { return Handler{Method::OPTIONS, handler}; }
			template<class H>
			Handler<H> TRACE(H handler) { return Handler{Method::TRACE, handler}; }
			template<class H>
			Handler<H> PATCH(H handler) { return Handler{Method::PATCH, handler}; }
		} // namespace handlers_only
		namespace handlers {
			using namespace handlers_only;
			using namespace build;
		} // namespace handlers

		struct BuildShortener {
			template<route_c... R>
			Router<R...> operator()(R... routes) const {
				return Router<R...>{routes...};
			}
			template<path_parser P, route_c... R>
			Parser<P, R...> operator()(P parser, R... routes) const {
				return Parser<P, R...>{parser, routes...};
			}
			template<route_c... R>
			Name<R...> operator()(const std::string_view name, R... routes) const {
				return Name<R...>{name, routes...};
			}
			template<class H>
			Handler<H> operator()(const MethodT &method, H handler) const {
				return Handler<H>{method, handler};
			}
		};
		namespace underscore {
			constexpr BuildShortener _{};
			using namespace handlers;
		} // namespace underscore
	}	  // namespace build

	template<class H, class PartsTuple>
	concept handler = detail::HandlerVerifier<H, PartsTuple>::value;

	namespace detail {
		template<class T, bool Value>
		std::enable_if_t<Value, T> static_assert_type_print() {
			return std::declval<T>();
		}
		template<class H>
		struct PathParserReturn;
		template<class H>
			requires path_parser_with_early<H>
		struct PathParserReturn<H> {
			using type = typename std::invoke_result_t<H, std::string_view, Request &>::value_type;
		};
		template<class H>
			requires path_parser<H>
		struct PathParserReturn<H> {
			using type = std::invoke_result_t<H, std::string_view>;
		};

		//{
		//	using type = path_parser_with_early<H> ? typename std::invoke_result_t<H, std::string_view, Request &>::value_type : std::invoke_result_t<H, std::string_view>;
		//	//decltype([]() consteval {
		//	//	if constexpr (path_parser_with_early<H>) {
		//	//		static_assert_type_print<H>();
		//	//		return std::declval<H>()(std::declval<std::string_view>(), std::declval<Request>()).value();
		//	//		//return std::declval<typename std::invoke_result_t<H, std::string_view, Request &>::value_type>();
		//	//	} else if constexpr (path_parser<H>) {
		//	//		return std::declval<std::invoke_result_t<H, std::string_view>>();
		//	//	} else {
		//	//		static_assert_type_print<H, false>();
		//	//	}
		//	//}());
		//};

		template<class H, class... Parts>
		concept router = requires(const H t, Request &request, size_t path_progress, Parts... parts) {
			t(request, path_progress, parts...);
		};
		template<class R, class P, class... Parts>
		concept optional_parser_router = std::is_same_v<R, std::monostate> || std::is_same_v<P, std::monostate> || router<R, Parts..., typename PathParserReturn<P>::type>;

		// std::pair<std::string_view, router>
		template<class N, class... Parts>
		concept named_router = requires(N n) {
			static_cast<typename N::first_type>(n.first);
			static_cast<typename N::second_type>(n.second);
			static_cast<std::string_view>(n.first);
		} && router<typename N::second_type, Parts...>;
		// std::tuple<named_router...>
		template<class Tuple, class... Parts>
		concept named_router_tuple = every<Tuple, []<class T>() consteval { return named_router<T, Parts...>; }>;

		// std::pair<MethodT, method_handler>
		template<class N, class Parts>
		concept method_handler = requires(N n) {
			static_cast<typename N::first_type>(n.first);
			static_cast<typename N::second_type>(n.second);
			static_cast<MethodT>(n.first);
		} && handler<typename N::second_type, Parts>;
		// std::tuple<method_handler...>
		template<class Tuple, class Parts>
		concept handler_tuple = every<Tuple, []<class T>() consteval { return method_handler<T, Parts>; }>;

		/**
		 * \brief Iterate over a tuple.
		 * \param tuple Tuple of elements to iterate over
		 * \param callback Returns `true` to break, `false` to continue
		 * \return `true` if callback broke, `false` if continued past the end
		 */
		template<class... Elements>
		asio::awaitable<bool> for_each_awaitable(const std::tuple<Elements...> tuple, auto callback) {
			co_return co_await std::apply([callback]<class... T>(T &&...element) -> asio::awaitable<bool> {
				bool done = false;
				((done || ((done = co_await callback(element)))), ...);

				// optimizer should convert a series of `if(!done)`s to early returns instead of constantly reevaluating the same condition. also these lambdas should all be inlined later.
				co_return done;
			},
										  tuple);
		}

		template<typename T>
		concept first_path_parser = requires(T t) {
			{ t.first } -> path_parser;
		};


		template<typename T>
		concept first_string_view = requires(T t) {
			static_cast<std::string_view>(t.first);
		};

		template<typename T>
		concept first_method = requires(T t) {
			static_cast<MethodT>(t.first);
		};

		template<typename T>
		concept route_key = requires(T t) {
			static_cast<std::string_view>(t);
		} || requires(T t) {
			{ t } -> path_parser;
		};
		template<auto Filter>
		auto filter(auto tup) {
			if constexpr (std::tuple_size_v<decltype(tup)> == 0) {
				return std::tuple{};
			}
			return std::apply([&]<typename F>(F first, auto... rest) {
				auto filtered_rest = [&] {
					if constexpr (sizeof...(rest)) {
						return filter<Filter>(std::make_tuple(rest...));
					} else {
						return std::tuple{};
					}
				}();

				if constexpr (Filter.template operator()<F>()) {
					return std::tuple_cat(std::make_tuple(first), filtered_rest);
				} else {
					return filtered_rest;
				}
			},
							  tup);
		}

		template<auto Filter>
		auto filter_map(auto tup, auto map) {
			if constexpr (std::tuple_size_v<decltype(tup)> == 0) {
				return std::make_tuple();
			}
			return std::apply([&]<class F>(F first, auto... rest) {
				auto filtered_rest = [&] {
					if constexpr (sizeof...(rest)) {
						return filter_map<Filter>(std::make_tuple(rest...), map);
					} else {
						return std::tuple{};
					}
				}();

				if constexpr (Filter.template operator()<F>()) {
					return std::tuple_cat(std::make_tuple(map(first)), filtered_rest);
				} else {
					return filtered_rest;
				}
			},
							  tup);
		}

		template<class R, class Parts>
		struct RoutesVerifier;
		template<class Parts, class... R>
		struct RoutesVerifier<build::Router<R...>, Parts> {
			static constexpr bool value = every<std::tuple<R...>, []<class T>() consteval {
											  // return if it's a good route type
											  if constexpr (build::parser_c<T>)
												  if constexpr (RoutesVerifier<typename T::routes_type, decltype(std::tuple_cat(std::declval<Parts>(), std::make_tuple(std::declval<typename PathParserReturn<typename T::parser_type>::type>())))>::value)
													  return 1;
											  if constexpr (build::name_c<T>)
												  if constexpr (RoutesVerifier<typename T::routes_type, Parts>::value)
													  return 1;
											  if constexpr (build::handler_c<T>)
												  if constexpr (handler<typename T::handler_type, Parts>)
													  return 1;
											  return 0;
										  }> &&
										  detail::Count<EWHTTP_CONCEPT_LAMBDA(build::parser_c), std::tuple<R...>>::value <= 1; // must have none or one path parser
		};
	}; // namespace detail

	template<class Rs, class Parts>
	concept routes = detail::RoutesVerifier<Rs, Parts>::value;

	template<optional_path_parser PathParser, class PathParserNext, class Named, class Method, class... PreviouslyParsedParts>
		requires detail::optional_parser_router<PathParserNext, PathParser, PreviouslyParsedParts...> && detail::named_router_tuple<Named, PreviouslyParsedParts...> && detail::handler_tuple<Method, std::tuple<PreviouslyParsedParts...>>
	class Router {
		PathParser parser;
		PathParserNext parser_next;
		Named named;
		Method method;
		Router(PathParser parser, PathParserNext parser_next, Named named, Method method) : parser{parser}, parser_next{parser_next}, named{named}, method{method} {}
		template<class...>
		friend auto create_router(build::router_c auto router);

	public:
		// valid server callback
		asio::awaitable<bool> operator()(Request &request, const size_t path_progress = 1, PreviouslyParsedParts... parts) const {
			// reached the end of the url?
			if (request.path.size() <= path_progress || (path_progress == request.path.size() - 1 && request.path.back() == '/')) {
				// method handlers
				co_return co_await detail::for_each_awaitable(method, [&](auto &method_handler) -> asio::awaitable<bool> {
					if (method_handler.first == request.method) {
						if constexpr (std::is_void_v<std::invoke_result_t<decltype(method_handler.second), decltype(request), decltype(parts)...>>) {
							method_handler.second(request, parts...);
						} else {
							co_await method_handler.second(request, parts...);
						}
						co_return true; // found the correct method handler, stop iterating and don't continue
					}
					co_return false; // continue over method handlers
				});
			}
			size_t slash = request.path.find('/', path_progress);
			if (slash == std::string_view::npos) slash = request.path.size();
			std::string_view path_part = std::string_view{request.path}.substr(path_progress, slash - path_progress);
			if (co_await detail::for_each_awaitable(named, [&](auto &named_handler) -> asio::awaitable<bool> {
					if (named_handler.first == path_part) {
						co_await named_handler.second(request, slash + 1, parts...);
						co_return true; // found it, stop iterating and don't continue to the any-parser
					}
					co_return false; // continue iterating
				})) co_return true;	 // if we found a name handler, we're done
			if constexpr (std::is_same_v<PathParser, std::monostate>) {
				co_return false;
			} else if constexpr (path_parser_with_early<PathParser>) {
				if (auto result = parser(path_part, request); result.has_value())
					co_await parser_next(request, slash + 1, parts..., result.value());
				co_return true;
			} else {
				co_await parser_next(request, slash + 1, parts..., parser(path_part));
				co_return true;
			}
		}
	};

	template<class... PrevParsedParts>
	auto create_router(build::router_c auto router) {
		using router_t = decltype(router);
		static_assert(routes<router_t, std::tuple<PrevParsedParts...>>);
		using routes_type = typename router_t::routes_type;
		//auto numbers = detail::filter_map<EWHTTP_CONCEPT_LAMBDA(detail::first_string_view)>(std::tuple{1, 2, std::pair{"hi", "heyyy"}, 3, 4}, [](std::pair<std::string_view, std::string_view> pair) {
		//	return std::make_pair(pair.first, pair.second.substr(0, 4));
		//});

		auto [parser, parsed_router] = [&] {
			if constexpr (detail::Count<EWHTTP_CONCEPT_LAMBDA(build::parser_c), routes_type>::value == 1) {
				// has a path parser alternative
				auto parser_router = std::get<detail::IndexOf<EWHTTP_CONCEPT_LAMBDA(build::parser_c), routes_type>::value>(router.routes);
				using pr_t = decltype(parser_router);
				return std::make_pair(parser_router.parser, create_router<PrevParsedParts..., typename detail::PathParserReturn<typename pr_t::parser_type>::type>(parser_router.routes));
			} else {
				// no path parser alternative
				return std::make_pair(std::monostate{}, std::monostate{});
			}
		}();
		using parser_t = decltype(parser);
		using parsed_router_t = decltype(parsed_router);
		static_assert(optional_path_parser<parser_t>);
		static_assert(detail::optional_parser_router<decltype(parsed_router), decltype(parser), PrevParsedParts...>);
		auto named = detail::filter_map<EWHTTP_CONCEPT_LAMBDA(build::name_c)>(router.routes, []<class... Routes>(build::Name<Routes...> pair) {
			return std::make_pair(pair.name, create_router<PrevParsedParts...>(pair.routes));
		});
		using named_t = decltype(named);
		//static_assert(detail::Count<EWHTTP_CONCEPT_LAMBDA(build::name_c), decltype(named)>::value == std::tuple_size_v<decltype(named)>);
		static_assert(detail::named_router_tuple<named_t, PrevParsedParts...>);
		//return std::tuple_cat(std::conditional_t<(std::is_same_v<std::tuple_element_t<0, T>, std::string_view>),
		//										 std::tuple<std::pair<std::tuple_element_t<0, T>, std::tuple_element_t<0, T>>>,
		//										 std::tuple<>>{}...);;
		auto method = detail::filter_map<EWHTTP_CONCEPT_LAMBDA(build::handler_c)>(router.routes, []<class H>(build::Handler<H> handler) {
			return std::make_pair(handler.method, handler.handler);
		});
		using method_t = decltype(method);
		static_assert(detail::handler_tuple<method_t, std::tuple<PrevParsedParts...>>);
		//return std::tuple_cat(std::conditional_t<(std::is_same_v<std::tuple_element_t<0, T>, Method>),
		//										 std::tuple<T>,
		//										 std::tuple<>>{}...);

		return Router<parser_t, parsed_router_t, named_t, method_t, PrevParsedParts...>(parser, parsed_router, named, method);
	}
	template<class... PrevParsedParts>
	auto create_router(build::route_c auto... routes) {
		return create_router<PrevParsedParts...>(build::Router{routes...});
	}
} // namespace ewhttp
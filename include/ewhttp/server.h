#pragma once
#include "./request.h"
#include <asio.hpp>
#include <functional>

namespace ewhttp {

	using ServerCallback = std::function<asio::awaitable<bool>(Request &)>;

	template<class T>
	concept server_callback = requires(T t) {
		ServerCallback{t};
	};

	class Server {
		ServerCallback callback;
		asio::io_context io_context{1};
		asio::any_io_executor io_executor{};

	public:
		template<server_callback Callback>
		explicit Server(const Callback &callback) : callback{callback} {}
		Server(const Server &) = delete;

		~Server() = default;
		/**
		 * \brief Start the server on the given host and port.
		 * \param host The host to listen on.
		 * \param port The port number to listen on.
		 */
		void accept(const asio::ip::address &host, uint16_t port);
		/**
		 * \brief Start the server on the given host and port.
		 * \param host Must be an IP string.
		 * \param port The port number to listen on.
		 */
		void accept(std::string_view host, uint16_t port);
		/**
		 * \brief Rudely kill the server, not allowing it to finish operations.
		 */
		void force_stop();

		/**
		 * \brief Politely stop the server, allowing it to finish operations.
		 */
		void stop();

		/**
		 * \brief Stop the server on any of the given signals.
		 * \param force If true, stop the server immediately, not allowing it to finish operations. If false, allow it to finish operations. (force_stop / stop)
		 * \param stop_signals Often `SIGINT, SIGTERM`. 
		 */
		template<std::same_as<int>... S>
			requires(sizeof...(S) > 0)
		void stop_on(const bool force, S... stop_signals) {
			asio::signal_set signals(io_context, stop_signals...);
			signals.async_wait([&](auto...) {
				if (force)
					force_stop();
				else
					stop();
			});
		}

		/**
		 * \brief Gracefully stop the server on any of the given signals.
		 * \param stop_signals Often `SIGINT, SIGTERM`. 
		 */
		template<std::same_as<int>... S>
			requires(sizeof...(S) > 0)
		void stop_on(S... stop_signals) {
			stop_on(false, stop_signals...);
		}

	private:
		asio::awaitable<void> respond(asio::ip::tcp::socket);
	};

	namespace detail {
		struct RequestContext {
			Request request;
			ServerCallback &callback;
			std::string method{};
			asio::ip::tcp::socket socket;
			asio::any_io_executor &executor;

			RequestContext(const Request &request, ServerCallback &callback, std::string method, asio::ip::tcp::socket socket, asio::any_io_executor &executor) : request{request}, callback{callback}, method{std::move(method)}, socket{std::move(socket)}, executor{executor} {}
		};
	} // namespace detail
} // namespace ewhttp
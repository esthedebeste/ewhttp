#pragma once
#include "./request.h"
#include "./response.h"
#include <asio.hpp>
#include <functional>

namespace ewhttp {
	using server_callback = std::function<async(Request &, Response &)>;

	template<class T>
	concept server_callback_c = requires(T t) {
		server_callback{t};
	};

	class Server {
		server_callback callback;
		asio::io_context io_context{1};
		asio::any_io_executor io_executor{};

	public:
		template<server_callback_c Callback>
		explicit Server(const Callback &callback) : callback{callback} {}
		~Server() = default;

		/**
		 * \brief Run the server on the given host and port.
		 * \param host The host to listen on.
		 * \param port The port number to listen on.
		 */
		void run(const asio::ip::address &host, uint16_t port);
		/**
		 * \brief Run the server on the given host and port.
		 * \param host Must be an IP string.
		 * \param port The port number to listen on.
		 */
		void run(std::string_view host, uint16_t port);
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
		async respond(asio::ip::tcp::socket);
	};

	namespace detail {
		struct RequestContext {
			Request request;
			server_callback &callback;
			std::string method{};
			asio::ip::tcp::socket socket;
			asio::any_io_executor &executor;

			RequestContext(const Request &request, server_callback &callback, std::string method, asio::ip::tcp::socket socket, asio::any_io_executor &executor) : request{request}, callback{callback}, method{std::move(method)}, socket{std::move(socket)}, executor{executor} {}
		};
	} // namespace detail
} // namespace ewhttp

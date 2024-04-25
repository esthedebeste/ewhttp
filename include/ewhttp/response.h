#pragma once
#include "./detail/string_map.h"
#include "./method.h"
#include "./request.h"
#include "./status.h"
#include <span>
#include <string_view>
#include <vector>

namespace ewhttp {
	struct Response {
		StatusT status{200};
		bool headers_sent{}, body_sent{};

		/**
		 * @brief Adds a header to the response. Does not clear existing headers or overwrite existing headers.
		 * @param key Header key
		 * @param value Header value
		 */
		void add_header(std::string_view key, std::string_view value);
		/**
		 * @brief Checks if a header exists.
		 * @param key Header key
		 */
		bool has_header(std::string_view key) const;
		/**
		 * @brief Get the header value(s) for a given key.
		 * @param key Header key
		 */
		std::vector<std::string> &get_header(std::string_view key);
		/**
		 * @brief Removes all instances of a certain header from the response.
		 * @param key Header key
		 * @return true if a header was removed, false if there was no header to remove
		 */
		bool remove_header(std::string_view key);
		/**
		 * @brief Sets a header in the response. (equivalent to remove_header(key); add_header(key, value);)
		 * @param key Header key
		 * @param value Header value
		 * @return true if a previously existing header was removed, false otherwise
		 */
		bool set_header(std::string_view key, std::string_view value);


		/**
		 * @brief Sends the headers to the client.
		 */
		async send_headers();
		/**
		 * @brief Sends the body to the client in one go.
		 * @param body The body to send
		 */
		async send_body(const std::span<const unsigned char> &body);
		/**
		 * @brief Sends the body to the client in one go.
		 * @param body The body to send
		 */
		async send_body(const std::span<const char> &body);
		/**
		 * @brief Consumes the entire body and sends it to the client in one go.
		 * @param body The stream to read from
		 * @param size The size of the body
		 */
		async send_body(std::istream &body, size_t size);
		/**
		 * @brief Consumes the entire body and sends it to the client in a streaming fashion.
		 * @param body The stream to read from
		 */
		async send_body(std::istream &body);

	private:
		detail::RequestContext &context;
		detail::string_map<std::vector<std::string>> headers{};

		explicit Response(Request &request) : context{*request.context} {}
		explicit Response(detail::RequestContext &context) : context{context} {}
		friend struct Request;
		friend class Server;
	};

	using Res = Response &;
} // namespace ewhttp
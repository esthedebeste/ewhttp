#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ewhttp {
	struct StatusT {
		std::uint_fast16_t code;
		// allow implicit conversion from std::uint_fast16_t
		constexpr StatusT(const std::uint_fast16_t code) : code{code} {}

		auto operator<=>(const StatusT &other) const {
			return code <=> other.code;
		}

		constexpr std::string_view name() const {
#define EWHTTP_MAP_CODE(code, name) \
	case code:                      \
		return name;

			switch (code) {
				EWHTTP_MAP_CODE(100, "Continue")
				EWHTTP_MAP_CODE(101, "Switching Protocols")
				EWHTTP_MAP_CODE(102, "Processing")
				EWHTTP_MAP_CODE(103, "Early Hints")
				EWHTTP_MAP_CODE(200, "OK")
				EWHTTP_MAP_CODE(201, "Created")
				EWHTTP_MAP_CODE(202, "Accepted")
				EWHTTP_MAP_CODE(203, "Non-Authoritative Information")
				EWHTTP_MAP_CODE(204, "No Content")
				EWHTTP_MAP_CODE(205, "Reset Content")
				EWHTTP_MAP_CODE(206, "Partial Content")
				EWHTTP_MAP_CODE(207, "Multi-Status")
				EWHTTP_MAP_CODE(208, "Already Reported")
				EWHTTP_MAP_CODE(226, "IM Used")
				EWHTTP_MAP_CODE(300, "Multiple Choices")
				EWHTTP_MAP_CODE(301, "Moved Permanently")
				EWHTTP_MAP_CODE(302, "Found")
				EWHTTP_MAP_CODE(303, "See Other")
				EWHTTP_MAP_CODE(304, "Not Modified")
				EWHTTP_MAP_CODE(307, "Temporary Redirect")
				EWHTTP_MAP_CODE(308, "Permanent Redirect")
				EWHTTP_MAP_CODE(400, "Bad Request")
				EWHTTP_MAP_CODE(401, "Unauthorized")
				EWHTTP_MAP_CODE(402, "Payment Required")
				EWHTTP_MAP_CODE(403, "Forbidden")
				EWHTTP_MAP_CODE(404, "Not Found")
				EWHTTP_MAP_CODE(405, "Method Not Allowed")
				EWHTTP_MAP_CODE(406, "Not Acceptable")
				EWHTTP_MAP_CODE(407, "Proxy Authentication Required")
				EWHTTP_MAP_CODE(408, "Request Timeout")
				EWHTTP_MAP_CODE(409, "Conflict")
				EWHTTP_MAP_CODE(410, "Gone")
				EWHTTP_MAP_CODE(411, "Length Required")
				EWHTTP_MAP_CODE(412, "Precondition Failed")
				EWHTTP_MAP_CODE(413, "Content Too Large")
				EWHTTP_MAP_CODE(414, "URI Too Long")
				EWHTTP_MAP_CODE(415, "Unsupported Media Type")
				EWHTTP_MAP_CODE(416, "Range Not Satisfiable")
				EWHTTP_MAP_CODE(417, "Expectation Failed")
				EWHTTP_MAP_CODE(418, "I'm a teapot")
				EWHTTP_MAP_CODE(421, "Misdirected Request")
				EWHTTP_MAP_CODE(422, "Unprocessable Content")
				EWHTTP_MAP_CODE(423, "Locked")
				EWHTTP_MAP_CODE(424, "Failed Dependency")
				EWHTTP_MAP_CODE(425, "Too Early")
				EWHTTP_MAP_CODE(426, "Upgrade Required")
				EWHTTP_MAP_CODE(428, "Precondition Required")
				EWHTTP_MAP_CODE(429, "Too Many Requests")
				EWHTTP_MAP_CODE(431, "Request Header Fields Too Large")
				EWHTTP_MAP_CODE(451, "Unavailable For Legal Reasons")
				EWHTTP_MAP_CODE(500, "Internal Server Error")
				EWHTTP_MAP_CODE(501, "Not Implemented")
				EWHTTP_MAP_CODE(502, "Bad Gateway")
				EWHTTP_MAP_CODE(503, "Service Unavailable")
				EWHTTP_MAP_CODE(504, "Gateway Timeout")
				EWHTTP_MAP_CODE(505, "HTTP Version Not Supported")
				EWHTTP_MAP_CODE(506, "Variant Also Negotiates")
				EWHTTP_MAP_CODE(507, "Insufficient Storage")
				EWHTTP_MAP_CODE(508, "Loop Detected")
				EWHTTP_MAP_CODE(510, "Not Extended")
				EWHTTP_MAP_CODE(511, "Network Authentication Required")
#undef EWHTTP_MAP_CODE
				default:
					return {};
			}
		}
	};
	namespace Status {
		constexpr StatusT
				Continue{100},
				SwitchingProtocols{101},
				Processing{102},
				EarlyHints{103},
				OK{200},
				Created{201},
				Accepted{202},
				NonAuthoritativeInformation{203},
				NoContent{204},
				ResetContent{205},
				PartialContent{206},
				MultiStatus{207},
				AlreadyReported{208},
				IMUsed{226},
				MultipleChoices{300},
				MovedPermanently{301},
				Found{302},
				SeeOther{303},
				NotModified{304},
				TemporaryRedirect{307},
				PermanentRedirect{308},
				BadRequest{400},
				Unauthorized{401},
				PaymentRequired{402},
				Forbidden{403},
				NotFound{404},
				MethodNotAllowed{405},
				NotAcceptable{406},
				ProxyAuthenticationRequired{407},
				RequestTimeout{408},
				Conflict{409},
				Gone{410},
				LengthRequired{411},
				PreconditionFailed{412},
				ContentTooLarge{413},
				URITooLong{414},
				UnsupportedMediaType{415},
				RangeNotSatisfiable{416},
				ExpectationFailed{417},
				ImATeapot{418},
				MisdirectedRequest{421},
				UnprocessableContent{422},
				Locked{423},
				FailedDependency{424},
				TooEarly{425},
				UpgradeRequired{426},
				PreconditionRequired{428},
				TooManyRequests{429},
				RequestHeaderFieldsTooLarge{431},
				UnavailableForLegalReasons{451},
				InternalServerError{500},
				NotImplemented{501},
				BadGateway{502},
				ServiceUnavailable{503},
				GatewayTimeout{504},
				HTTPVersionNotSupported{505},
				VariantAlsoNegotiates{506},
				InsufficientStorage{507},
				LoopDetected{508},
				NotExtended{510},
				NetworkAuthenticationRequired{511};
	} // namespace Status
} // namespace ewhttp
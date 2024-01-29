#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ewhttp {
	namespace detail {
		constexpr std::array<std::string_view, 9> method_names{
				"GET",
				"HEAD",
				"POST",
				"PUT",
				"DELETE",
				"CONNECT",
				"OPTIONS",
				"TRACE",
				"PATCH",
		};
	}
	struct MethodT {
		std::uint_fast8_t id;
		constexpr MethodT(const std::uint_fast8_t id) : id{id} {}

		auto operator<=>(const MethodT &other) const {
			return id <=> other.id;
		}
		auto operator==(const MethodT &other) const {
			return id == other.id;
		}

		constexpr std::string_view name() {
			if (id > detail::method_names.size())
				return {};
			return detail::method_names[id];
		}
	};
	namespace Method {
		constexpr MethodT
				GET{0},
				HEAD{1},
				POST{2},
				PUT{3},
				DELETE{4},
				CONNECT{5},
				OPTIONS{6},
				TRACE{7},
				PATCH{8};


		constexpr std::optional<MethodT> from_string(const std::string_view &str) {
			for (uint8_t id = 0; id < detail::method_names.size(); id++)
				if (str == detail::method_names[id])
					return MethodT{id};
			return std::nullopt;
		}
	} // namespace Method
} // namespace ewhttp
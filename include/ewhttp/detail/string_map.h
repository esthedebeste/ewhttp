#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace ewhttp::detail {
	struct string_hash {
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;

		size_t operator()(const char *str) const { return hash_type{}(str); }
		size_t operator()(std::string_view str) const { return hash_type{}(str); }
		size_t operator()(std::string const &str) const { return hash_type{}(str); }
	};
	// indexable with const char *, std::string_view, and std::string
	template<typename T>
	using string_map = std::unordered_map<std::string, T, detail::string_hash, std::equal_to<>>;
} // namespace ewhttp::detail
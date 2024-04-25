#pragma once
#include "./detail/string_map.h"
#include "./request.h"
#include "./response.h"
#include <algorithm>
#include <filesystem>
#include <variant>
#include <vector>


namespace ewhttp {
	namespace detail {
		struct MemoryFile {
			std::vector<uint8_t> data;
		};
		struct StreamingFile {
			std::filesystem::path path;
			uintmax_t size;
		};
		using File = std::variant<MemoryFile, StreamingFile>;
	} // namespace detail
	namespace build {
		struct FilesOptions {
			uintmax_t max_memory_cached = 8'388'608; // 8mb
		};

		struct Files {
			detail::string_map<detail::File> files;
			Files(std::string_view path_to_root, const FilesOptions &options = {});
			async operator()(Req request, Res response, const size_t path_progress);
		};
	} // namespace build
} // namespace ewhttp
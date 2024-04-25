#include <ewhttp/files.h>

#include <fstream>

namespace ewhttp::build {
	Files::Files(std::string_view path_to_root, const FilesOptions &options) {
		for (const auto &member : std::filesystem::recursive_directory_iterator{path_to_root}) {
			if (member.is_directory())
				continue;
			auto size = member.file_size();
			const auto &path = member.path();
			std::string path_string = path.generic_string();
			std::string url_path = path_string.substr(path_to_root.size() + 1);
			if (size < options.max_memory_cached) {
				detail::MemoryFile file{};
				file.data.reserve(size);
				std::ifstream stream(path, std::ios::binary);
				stream.unsetf(std::ios::skipws);
				file.data.insert(file.data.begin(),
								 std::istream_iterator<uint8_t>(stream),
								 std::istream_iterator<uint8_t>());
				files.emplace(url_path, file);
			} else {
				detail::StreamingFile file{path, size};
				files.emplace(url_path, file);
			}
		}
	}

	async Files::operator()(Req request, Res response, const size_t path_progress) {
		std::string_view remaining = std::string_view{request.path}.substr(path_progress);
		if (auto file_pair = files.find(remaining); file_pair != files.end()) {
			auto &[_, file] = *file_pair;
			if (auto memory = std::get_if<detail::MemoryFile>(&file)) {
				co_await response.send_body(memory->data);
			} else {
				auto &streaming = std::get<detail::StreamingFile>(file);
				std::ifstream stream(streaming.path, std::ios::binary);
				stream.unsetf(std::ios::skipws);
				co_await response.send_body(stream);
			}
		}
	}
} // namespace ewhttp::build
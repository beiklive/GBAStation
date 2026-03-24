#include "Utils/ConfigManager.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

#include <borealis/core/logger.hpp>

namespace beiklive {

// -------------------- ConfigValue --------------------

ConfigValue::ConfigValue(int value) : data_(value) {}
ConfigValue::ConfigValue(float value) : data_(value) {}
ConfigValue::ConfigValue(std::string value) : data_(std::move(value)) {}
ConfigValue::ConfigValue(const char* value) : data_(std::string(value)) {}
ConfigValue::ConfigValue(IntArray value) : data_(std::move(value)) {}
ConfigValue::ConfigValue(FloatArray value) : data_(std::move(value)) {}
ConfigValue::ConfigValue(StringArray value) : data_(std::move(value)) {}

ConfigValue::Type ConfigValue::GetType() const {
	switch (data_.index()) {
	case 1: return Type::Int;
	case 2: return Type::Float;
	case 3: return Type::String;
	case 4: return Type::IntArray;
	case 5: return Type::FloatArray;
	case 6: return Type::StringArray;
	default: return Type::None;
	}
}

std::optional<int> ConfigValue::AsInt() const {
	if (const auto* p = std::get_if<int>(&data_)) return *p;
	return std::nullopt;
}

std::optional<float> ConfigValue::AsFloat() const {
	if (const auto* p = std::get_if<float>(&data_)) return *p;
	return std::nullopt;
}

std::optional<std::string> ConfigValue::AsString() const {
	if (const auto* p = std::get_if<std::string>(&data_)) return *p;
	return std::nullopt;
}

std::optional<ConfigValue::IntArray> ConfigValue::AsIntArray() const {
	if (const auto* p = std::get_if<IntArray>(&data_)) return *p;
	return std::nullopt;
}

std::optional<ConfigValue::FloatArray> ConfigValue::AsFloatArray() const {
	if (const auto* p = std::get_if<FloatArray>(&data_)) return *p;
	return std::nullopt;
}

std::optional<ConfigValue::StringArray> ConfigValue::AsStringArray() const {
	if (const auto* p = std::get_if<StringArray>(&data_)) return *p;
	return std::nullopt;
}

const ConfigValue::Variant& ConfigValue::Raw() const {
	return data_;
}

// -------------------- ConfigManager --------------------

ConfigManager::ConfigManager(std::string filePath) : filePath_(std::move(filePath)) {
	if (std::filesystem::exists(filePath_)) {
		Load();
	}else{
		// 文件不存在，创建一个空的配置文件
		std::ofstream out(filePath_);
		if (out.is_open()) {
			out.close();
		}
	}
}

bool ConfigManager::Load() {
	std::ifstream in(filePath_);
	if (!in.is_open()) {
		brls::Logger::warning("ConfigManager::Load: cannot open file '{}'", filePath_);
		return false;
	}

	std::unordered_map<std::string, Entry> loaded;
	std::string line;
	int count = 0;
	while (std::getline(in, line)) {
		const std::string trimmed = Trim(line);
		if (trimmed.empty() || trimmed[0] == '#') continue;

		const size_t eq = trimmed.find('=');
		if (eq == std::string::npos) continue;

		const std::string key = Trim(std::string_view(trimmed).substr(0, eq));
		const std::string rawValue = Trim(std::string_view(trimmed).substr(eq + 1));
		if (key.empty() || rawValue.empty()) continue;

		auto value = DeserializeValue(rawValue);
		if (!value.has_value()) continue;

		loaded[key] = Entry{ *value, true };
		++count;
	}

	for (const auto& [k, v] : entries_) {
		if (!v.persist) loaded[k] = v;
	}
	entries_ = std::move(loaded);
	brls::Logger::debug("ConfigManager::Load: loaded {} keys from '{}'", count, filePath_);
	return true;
}

bool ConfigManager::Save() const {
	const std::filesystem::path p(filePath_);
	if (p.has_parent_path()) {
		std::error_code ec;
		std::filesystem::create_directories(p.parent_path(), ec);
	}

	std::ofstream out(filePath_, std::ios::trunc);
	if (!out.is_open()) {
		brls::Logger::error("ConfigManager::Save: cannot open file '{}' for writing", filePath_);
		return false;
	}

	std::vector<std::string> keys;
	keys.reserve(entries_.size());
	for (const auto& [key, entry] : entries_) {
		if (entry.persist) keys.push_back(key);
	}
	std::sort(keys.begin(), keys.end());
	for (const auto& key : keys) {
		out << key << "=" << SerializeValue(entries_.at(key).value) << "\n";
	}
	brls::Logger::debug("ConfigManager::Save: saved {} keys to '{}'", static_cast<int>(keys.size()), filePath_);
	return true;
}

void ConfigManager::SetDefault(const std::string& key, const ConfigValue& value) {
	if (!Contains(key)) {
		// brls::Logger::debug("ConfigManager::SetDefault [{}]: setting default", key);
		Set(key, value, true);
	}
}

void ConfigManager::Set(const std::string& key, const ConfigValue& value, bool persist) {
	if (key.empty()) return;
	// brls::Logger::debug("ConfigManager::Set [{}] persist={}", key, persist);
	entries_[key] = Entry{ value, persist };
}

std::optional<ConfigValue> ConfigManager::Get(const std::string& key) const {
	auto it = entries_.find(key);
	if (it == entries_.end()) {
		brls::Logger::debug("ConfigManager::Get [{}]: not found", key);
		return std::nullopt;
	}
	// brls::Logger::debug("ConfigManager::Get [{}]: found", key);
	return it->second.value;
}

bool ConfigManager::Contains(const std::string& key) const {
	return entries_.find(key) != entries_.end();
}

bool ConfigManager::Remove(const std::string& key) {
	return entries_.erase(key) > 0;
}

void ConfigManager::Clear() {
	entries_.clear();
}

std::vector<std::string> ConfigManager::GetAllKeys() const {
	std::vector<std::string> keys;
	keys.reserve(entries_.size());
	for (const auto& pair : entries_) {
		keys.push_back(pair.first);
	}
	return keys;
}

std::string ConfigManager::Trim(std::string_view text) {
	size_t begin = 0;
	while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
	if (begin == text.size()) return {};

	size_t end = text.size() - 1;
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end]))) --end;
	return std::string(text.substr(begin, end - begin + 1));
}

std::string ConfigManager::Escape(const std::string& text) {
	std::string out;
	out.reserve(text.size());
	for (char c : text) {
		if (c == '\\' || c == ',' || c == '|') out.push_back('\\');
		out.push_back(c);
	}
	return out;
}

std::string ConfigManager::Unescape(const std::string& text) {
	std::string out;
	out.reserve(text.size());
	bool escaped = false;
	for (char c : text) {
		if (escaped) {
			out.push_back(c);
			escaped = false;
			continue;
		}
		if (c == '\\') {
			escaped = true;
			continue;
		}
		out.push_back(c);
	}
	if (escaped) out.push_back('\\');
	return out;
}

std::vector<std::string> ConfigManager::SplitEscaped(const std::string& text, char delimiter) {
	std::vector<std::string> result;
	std::string token;
	bool escaped = false;

	for (char c : text) {
		if (escaped) {
			token.push_back(c);
			escaped = false;
			continue;
		}
		if (c == '\\') {
			escaped = true;
			continue;
		}
		if (c == delimiter) {
			result.push_back(token);
			token.clear();
			continue;
		}
		token.push_back(c);
	}
	if (escaped) token.push_back('\\');
	result.push_back(token);
	return result;
}

std::string ConfigManager::SerializeValue(const ConfigValue& value) {
	std::ostringstream oss;
	switch (value.GetType()) {
	case ConfigValue::Type::Int:
		oss << "i|" << *value.AsInt();
		break;
	case ConfigValue::Type::Float:
		oss.precision(std::numeric_limits<float>::max_digits10);
		oss << "f|" << *value.AsFloat();
		break;
	case ConfigValue::Type::String:
		oss << "s|" << Escape(*value.AsString());
		break;
	case ConfigValue::Type::IntArray: {
		oss << "ia|";
		const auto arr = *value.AsIntArray();
		for (size_t i = 0; i < arr.size(); ++i) {
			if (i) oss << ",";
			oss << arr[i];
		}
		break;
	}
	case ConfigValue::Type::FloatArray: {
		oss << "fa|";
		oss.precision(std::numeric_limits<float>::max_digits10);
		const auto arr = *value.AsFloatArray();
		for (size_t i = 0; i < arr.size(); ++i) {
			if (i) oss << ",";
			oss << arr[i];
		}
		break;
	}
	case ConfigValue::Type::StringArray: {
		oss << "sa|";
		const auto arr = *value.AsStringArray();
		for (size_t i = 0; i < arr.size(); ++i) {
			if (i) oss << ",";
			oss << Escape(arr[i]);
		}
		break;
	}
	default:
		oss << "s|";
		break;
	}
	return oss.str();
}

std::optional<ConfigValue> ConfigManager::DeserializeValue(const std::string& encoded) {
	const size_t sep = encoded.find('|');
	if (sep == std::string::npos) return std::nullopt;

	const std::string type = encoded.substr(0, sep);
	const std::string payload = encoded.substr(sep + 1);

	try {
		if (type == "i") {
			size_t idx = 0;
			int v = std::stoi(payload, &idx);
			if (idx != payload.size()) return std::nullopt;
			return ConfigValue(v);
		}
		if (type == "f") {
			size_t idx = 0;
			float v = std::stof(payload, &idx);
			if (idx != payload.size()) return std::nullopt;
			return ConfigValue(v);
		}
		if (type == "s") {
			return ConfigValue(Unescape(payload));
		}
		if (type == "ia") {
			ConfigValue::IntArray arr;
			if (!payload.empty()) {
				std::stringstream ss(payload);
				std::string part;
				while (std::getline(ss, part, ',')) {
					part = Trim(part);
					if (part.empty()) return std::nullopt;
					size_t idx = 0;
					int v = std::stoi(part, &idx);
					if (idx != part.size()) return std::nullopt;
					arr.push_back(v);
				}
			}
			return ConfigValue(std::move(arr));
		}
		if (type == "fa") {
			ConfigValue::FloatArray arr;
			if (!payload.empty()) {
				std::stringstream ss(payload);
				std::string part;
				while (std::getline(ss, part, ',')) {
					part = Trim(part);
					if (part.empty()) return std::nullopt;
					size_t idx = 0;
					float v = std::stof(part, &idx);
					if (idx != part.size()) return std::nullopt;
					arr.push_back(v);
				}
			}
			return ConfigValue(std::move(arr));
		}
		if (type == "sa") {
			ConfigValue::StringArray arr;
			if (!payload.empty()) {
				auto parts = SplitEscaped(payload, ',');
				arr.reserve(parts.size());
				for (auto& p : parts) arr.push_back(p);
			}
			return ConfigValue(std::move(arr));
		}
	} catch (...) {
		return std::nullopt;
	}

	return std::nullopt;
}

bool ConfigManager::ConvertLegacyFile(const std::string& oldPath, const std::string& newPath) {
	if (!std::filesystem::exists(oldPath)) return false;

	std::ifstream in(oldPath);
	if (!in.is_open()) return false;

	const std::filesystem::path outPath(newPath);
	if (outPath.has_parent_path()) {
		std::error_code ec;
		std::filesystem::create_directories(outPath.parent_path(), ec);
	}

	std::ofstream out(newPath, std::ios::trunc);
	if (!out.is_open()) return false;

	std::string line;
	while (std::getline(in, line)) {
		const std::string trimmed = Trim(line);
		if (trimmed.empty() || trimmed[0] == '#') continue;

		const size_t eq = trimmed.find('=');
		if (eq == std::string::npos) continue;

		const std::string key = Trim(std::string_view(trimmed).substr(0, eq));
		const std::string value = Trim(std::string_view(trimmed).substr(eq + 1));
		if (key.empty()) continue;

		out << key << "=s|" << Escape(value) << "\n";
	}

	return true;
}

} // namespace beiklive
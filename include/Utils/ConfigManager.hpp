#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace beiklive {

class ConfigValue {
public:
	using IntArray = std::vector<int>;
	using FloatArray = std::vector<float>;
	using StringArray = std::vector<std::string>;
	using Variant = std::variant<std::monostate, int, float, std::string, IntArray, FloatArray, StringArray>;

	enum class Type {
		None,
		Int,
		Float,
		String,
		IntArray,
		FloatArray,
		StringArray
	};

	ConfigValue() = default;
	ConfigValue(int value);
	ConfigValue(float value);
	ConfigValue(std::string value);
	ConfigValue(const char* value);
	ConfigValue(IntArray value);
	ConfigValue(FloatArray value);
	ConfigValue(StringArray value);

	Type GetType() const;

	std::optional<int> AsInt() const;
	std::optional<float> AsFloat() const;
	std::optional<std::string> AsString() const;
	std::optional<IntArray> AsIntArray() const;
	std::optional<FloatArray> AsFloatArray() const;
	std::optional<StringArray> AsStringArray() const;

	const Variant& Raw() const;

private:
	Variant data_;
};

class ConfigManager {
public:
	explicit ConfigManager(std::string filePath);

	bool Load();
	bool Save() const;


	static bool ConvertLegacyFile(const std::string& oldPath, const std::string& newPath);

	void SetDefault(const std::string& key, const ConfigValue& value);
	void Set(const std::string& key, const ConfigValue& value, bool persist = true);
	std::optional<ConfigValue> Get(const std::string& key) const;

	bool Contains(const std::string& key) const;
	bool Remove(const std::string& key);
	void Clear();
	/// 返回所有持久化 key 列表（用于遍历数据）
	std::vector<std::string> GetAllKeys() const;

private:
	struct Entry {
		ConfigValue value;
		bool persist = true;
	};

	static std::string Trim(std::string_view text);
	static std::string Escape(const std::string& text);
	static std::string Unescape(const std::string& text);
	static std::vector<std::string> SplitEscaped(const std::string& text, char delimiter);

	static std::string SerializeValue(const ConfigValue& value);
	static std::optional<ConfigValue> DeserializeValue(const std::string& encoded);

	std::string filePath_;
	std::unordered_map<std::string, Entry> entries_;
};

} // namespace beiklive

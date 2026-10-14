#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <type_traits>
#include <istream>
#include <initializer_list>

namespace SimpleJson {

enum class Type { Null, Boolean, Number, String, Array, Object };

class Json {
public:
    Type type = Type::Null;
    bool bool_val = false;
    double num_val = 0;
    std::string str_val;
    std::vector<Json> arr_val;
    std::map<std::string, Json> obj_val;

    struct JsonIterator {
        bool is_arr = false;
        std::vector<Json>::const_iterator arr_it;
        std::map<std::string, Json>::const_iterator obj_it;
        JsonIterator() : is_arr(false) {}
        JsonIterator(std::vector<Json>::const_iterator it) : is_arr(true), arr_it(it) {}
        JsonIterator(std::map<std::string, Json>::const_iterator it) : is_arr(false), obj_it(it) {}
        bool operator!=(const JsonIterator& o) const { return is_arr ? arr_it != o.arr_it : obj_it != o.obj_it; }
        bool operator==(const JsonIterator& o) const { return !(*this != o); }
        JsonIterator& operator++() { if (is_arr) ++arr_it; else ++obj_it; return *this; }
        const Json& operator*() const {
            static const Json null_val;
            return is_arr ? *arr_it : obj_it->second;
        }
        const Json* operator->() const { return &(operator*()); }
        const std::string& key() const { return obj_it->first; }
        const Json& value() const { return operator*(); }
    };

    Json() : type(Type::Null) {}
    Json(std::nullptr_t) : type(Type::Null) {}
    Json(bool b) : type(Type::Boolean), bool_val(b) {}
    Json(int n) : type(Type::Number), num_val(n) {}
    Json(int64_t n) : type(Type::Number), num_val(static_cast<double>(n)) {}
    Json(uint64_t n) : type(Type::Number), num_val(static_cast<double>(n)) {}
    Json(double n) : type(Type::Number), num_val(n) {}
    Json(const char* s) : type(Type::String), str_val(s ? s : "") {}
    Json(const std::string& s) : type(Type::String), str_val(s) {}
    Json(std::string&& s) : type(Type::String), str_val(std::move(s)) {}

    Json(std::initializer_list<Json> init) {
        bool all_pairs = true;
        for (const auto& item : init) {
            if (!item.is_array() || item.size() != 2 || !item[0].is_string()) {
                all_pairs = false;
                break;
            }
        }
        if (all_pairs && init.size() > 0) {
            type = Type::Object;
            for (const auto& item : init) {
                obj_val[item[0].str_val] = item[1];
            }
        } else {
            type = Type::Array;
            arr_val.assign(init.begin(), init.end());
        }
    }
    Json& operator=(std::initializer_list<Json> init) {
        *this = Json(init);
        return *this;
    }

    static Json object() { Json j; j.type = Type::Object; return j; }
    static Json array() { Json j; j.type = Type::Array; return j; }

    bool is_null() const { return type == Type::Null; }
    bool is_boolean() const { return type == Type::Boolean; }
    bool is_number() const { return type == Type::Number; }
    bool is_number_integer() const { return type == Type::Number && num_val == static_cast<int64_t>(num_val); }
    bool is_number_unsigned() const { return type == Type::Number && num_val >= 0 && num_val == static_cast<uint64_t>(num_val); }
    bool is_string() const { return type == Type::String; }
    bool is_array() const { return type == Type::Array; }
    bool is_object() const { return type == Type::Object; }

    size_t size() const {
        if (type == Type::Array) return arr_val.size();
        if (type == Type::Object) return obj_val.size();
        return 0;
    }
    bool empty() const {
        if (type == Type::Array) return arr_val.empty();
        if (type == Type::Object) return obj_val.empty();
        return true;
    }

    bool contains(const std::string& key) const {
        return type == Type::Object && obj_val.find(key) != obj_val.end();
    }

    const Json& at(const std::string& key) const {
        auto it = obj_val.find(key);
        if (it == obj_val.end()) throw std::out_of_range("key not found: " + key);
        return it->second;
    }
    Json& at(const std::string& key) {
        auto it = obj_val.find(key);
        if (it == obj_val.end()) throw std::out_of_range("key not found: " + key);
        return it->second;
    }

    Json& operator[](const std::string& key) {
        if (type != Type::Object) { type = Type::Object; obj_val.clear(); }
        return obj_val[key];
    }
    const Json& operator[](const std::string& key) const {
        static const Json null_val;
        if (type != Type::Object) return null_val;
        auto it = obj_val.find(key);
        return it != obj_val.end() ? it->second : null_val;
    }

    Json& operator[](size_t idx) {
        if (type != Type::Array) { type = Type::Array; arr_val.clear(); }
        if (idx >= arr_val.size()) arr_val.resize(idx + 1);
        return arr_val[idx];
    }
    const Json& operator[](size_t idx) const {
        static const Json null_val;
        return (type == Type::Array && idx < arr_val.size()) ? arr_val[idx] : null_val;
    }

    template<typename T>
    T value(const std::string& key, const T& default_val) const {
        if (type != Type::Object) return default_val;
        auto it = obj_val.find(key);
        if (it == obj_val.end() || it->second.is_null()) return default_val;
        try { return it->second.template get<T>(); } catch (...) { return default_val; }
    }
    std::string value(const std::string& key, const char* default_val) const {
        return value<std::string>(key, std::string(default_val ? default_val : ""));
    }

    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, Json>) {
            return *this;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return is_string() ? str_val : dump();
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            return is_number() ? static_cast<T>(num_val) : 0;
        } else if constexpr (std::is_floating_point_v<T>) {
            return is_number() ? static_cast<T>(num_val) : 0.0;
        } else if constexpr (std::is_same_v<T, bool>) {
            return is_boolean() ? bool_val : false;
        } else {
            return T();
        }
    }

    bool operator==(const Json& other) const {
        if (type != other.type) return false;
        switch (type) {
            case Type::Null: return true;
            case Type::Boolean: return bool_val == other.bool_val;
            case Type::Number: return num_val == other.num_val;
            case Type::String: return str_val == other.str_val;
            case Type::Array: return arr_val == other.arr_val;
            case Type::Object: return obj_val == other.obj_val;
        }
        return false;
    }
    bool operator!=(const Json& other) const { return !(*this == other); }

    std::string dump(int indent = 0) const {
        (void)indent;
        switch (type) {
            case Type::Null: return "null";
            case Type::Boolean: return bool_val ? "true" : "false";
            case Type::Number: {
                if (num_val == static_cast<int64_t>(num_val)) return std::to_string(static_cast<int64_t>(num_val));
                std::ostringstream ss; ss << num_val; return ss.str();
            }
            case Type::String: {
                std::string out = "\"";
                for (char c : str_val) {
                    if (c == '"') out += "\\\"";
                    else if (c == '\\') out += "\\\\";
                    else if (c == '\n') out += "\\n";
                    else if (c == '\r') out += "\\r";
                    else if (c == '\t') out += "\\t";
                    else out += c;
                }
                return out + "\"";
            }
            case Type::Array: {
                std::string out = "[";
                for (size_t i = 0; i < arr_val.size(); ++i) {
                    if (i > 0) out += ",";
                    out += arr_val[i].dump();
                }
                return out + "]";
            }
            case Type::Object: {
                std::string out = "{";
                size_t i = 0;
                for (const auto& [k, v] : obj_val) {
                    if (i++ > 0) out += ",";
                    out += "\"" + k + "\":" + v.dump();
                }
                return out + "}";
            }
        }
        return "null";
    }

    JsonIterator begin() const {
        if (type == Type::Array) return JsonIterator(arr_val.begin());
        return JsonIterator(obj_val.begin());
    }
    JsonIterator end() const {
        if (type == Type::Array) return JsonIterator(arr_val.end());
        return JsonIterator(obj_val.end());
    }

    JsonIterator find(const std::string& key) const {
        if (type != Type::Object) return end();
        auto it = obj_val.find(key);
        if (it == obj_val.end()) return end();
        return JsonIterator(it);
    }

    struct Parser {
        const std::string& src;
        size_t pos = 0;
        void skip() {
            while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) ++pos;
        }
        char peek() { skip(); return pos < src.size() ? src[pos] : '\0'; }
        char get() { skip(); return pos < src.size() ? src[pos++] : '\0'; }
        bool match(const std::string& s) {
            skip();
            if (pos + s.size() <= src.size() && src.compare(pos, s.size(), s) == 0) {
                pos += s.size(); return true;
            }
            return false;
        }
        std::string parseString() {
            if (get() != '"') throw std::runtime_error("expected string");
            std::string s;
            while (pos < src.size()) {
                char c = src[pos++];
                if (c == '"') return s;
                if (c == '\\' && pos < src.size()) {
                    char esc = src[pos++];
                    if (esc == '"') s += '"';
                    else if (esc == '\\') s += '\\';
                    else if (esc == '/') s += '/';
                    else if (esc == 'n') s += '\n';
                    else if (esc == 'r') s += '\r';
                    else if (esc == 't') s += '\t';
                    else if (esc == 'b') s += '\b';
                    else if (esc == 'f') s += '\f';
                    else if (esc == 'u' && pos + 4 <= src.size()) { pos += 4; s += '?'; }
                    else s += esc;
                } else s += c;
            }
            return s;
        }
        Json parseNumber() {
            skip();
            size_t start = pos;
            if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) ++pos;
            while (pos < src.size() && (isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' || src[pos] == '+' || src[pos] == '-')) ++pos;
            std::string sub = src.substr(start, pos - start);
            try { return Json(std::stod(sub)); } catch (...) { return Json(0.0); }
        }
        Json parseArray() {
            get(); // '['
            Json arr = Json::array();
            if (peek() == ']') { get(); return arr; }
            while (pos < src.size()) {
                arr.arr_val.push_back(parseValue());
                char next = peek();
                if (next == ']') { get(); break; }
                if (next == ',') { get(); continue; }
                throw std::runtime_error("expected , or ]");
            }
            return arr;
        }
        Json parseObject() {
            get(); // '{'
            Json obj = Json::object();
            if (peek() == '}') { get(); return obj; }
            while (pos < src.size()) {
                std::string key = parseString();
                if (get() != ':') throw std::runtime_error("expected :");
                obj.obj_val[key] = parseValue();
                char next = peek();
                if (next == '}') { get(); break; }
                if (next == ',') { get(); continue; }
                throw std::runtime_error("expected , or }");
            }
            return obj;
        }
        Json parseValue() {
            char c = peek();
            if (c == '"') return Json(parseString());
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == 't' && match("true")) return Json(true);
            if (c == 'f' && match("false")) return Json(false);
            if (c == 'n' && match("null")) return Json();
            if (c == '-' || isdigit(static_cast<unsigned char>(c))) return parseNumber();
            throw std::runtime_error("unexpected json char");
        }
    };

    static Json parse(const std::string& src) {
        Parser p{src, 0};
        return p.parseValue();
    }

    static Json parse(std::istream& is) {
        std::string s((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        return parse(s);
    }
};

using JsonIterator = Json::JsonIterator;

} // namespace SimpleJson

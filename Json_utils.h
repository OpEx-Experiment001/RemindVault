#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>

inline std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

inline std::string jStr(const std::string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

inline std::string jInt(int n) {
    return std::to_string(n);
}

inline std::string jBool(bool b) {
    return b ? "true" : "false";
}

struct JsonObject {
    std::vector<std::pair<std::string, std::string>> fields;

    JsonObject& add(const std::string& key, const std::string& rawValue) {
        fields.push_back({key, rawValue});
        return *this;
    }

    JsonObject& addStr(const std::string& key, const std::string& val) {
        return add(key, jStr(val));
    }
   
    JsonObject& addInt(const std::string& key, int val) {
        return add(key, jInt(val));
    }
   
    JsonObject& addBool(const std::string& key, bool val) {
        return add(key, jBool(val));
    }

    std::string build() const {
        std::string out = "{";
        for (size_t i = 0; i < fields.size(); i++) {
            if (i > 0) out += ",";
            out += "\"" + fields[i].first + "\":" + fields[i].second;
        }
        out += "}";
        return out;
    }
};

struct JsonArray {
    std::vector<std::string> items;

    JsonArray& push(const std::string& rawItem) {
        items.push_back(rawItem);
        return *this;
    }

    std::string build() const {
        std::string out = "[";
        for (size_t i = 0; i < items.size(); i++) {
            if (i > 0) out += ",";
            out += items[i];
        }
        out += "]";
        return out;
    }
};

inline size_t skipWS(const std::string& s, size_t i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t')) i++;
    return i;
}

inline std::string parseString(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"')
        throw std::runtime_error("Expected '\"' at pos " + std::to_string(pos));
    pos++; // skip opening quote
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos+1 < s.size()) {
            pos++;
            switch (s[pos]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += s[pos]; break;
            }
        } else {
            result += s[pos];
        }
        pos++;
    }
    pos++; 
    return result;
}

struct JsonValue;
using JsonObj  = std::map<std::string, std::string>; 
using JsonArr  = std::vector<std::string>;            

inline std::string parseValue(const std::string& s, size_t& pos);

inline JsonObj parseObject(const std::string& s, size_t& pos) {
    JsonObj obj;
    if (pos >= s.size() || s[pos] != '{')
        throw std::runtime_error("Expected '{' at pos " + std::to_string(pos));
    pos++;
    pos = skipWS(s, pos);
    while (pos < s.size() && s[pos] != '}') {
        pos = skipWS(s, pos);
        std::string key = parseString(s, pos);
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ':') pos++;
        pos = skipWS(s, pos);
        std::string val = parseValue(s, pos);
        obj[key] = val;
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ',') pos++;
        pos = skipWS(s, pos);
    }
    if (pos < s.size()) pos++; 
    return obj;
}

inline JsonArr parseArray(const std::string& s, size_t& pos) {
    JsonArr arr;
    if (pos >= s.size() || s[pos] != '[')
        throw std::runtime_error("Expected '[' at pos " + std::to_string(pos));
    pos++; 
    pos = skipWS(s, pos);
    while (pos < s.size() && s[pos] != ']') {
        pos = skipWS(s, pos);
        if (s[pos] == ']') break;
        std::string val = parseValue(s, pos);
        arr.push_back(val);
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ',') pos++;
        pos = skipWS(s, pos);
    }
    if (pos < s.size()) pos++; 
    return arr;
}

inline std::string parseValue(const std::string& s, size_t& pos) {
    pos = skipWS(s, pos);
    if (pos >= s.size()) return "null";

    if (s[pos] == '"') {
      
        size_t start = pos;
        std::string str = parseString(s, pos);
        return jStr(str);
    } else if (s[pos] == '{') {
       
        size_t depth = 0, start = pos;
        bool inStr = false;
        while (pos < s.size()) {
            if (!inStr && s[pos]=='{') depth++;
            else if (!inStr && s[pos]=='}') { depth--; if(depth==0){pos++;break;} }
            else if (s[pos]=='"') inStr=!inStr;
            else if (inStr && s[pos]=='\\') pos++;
            pos++;
        }
        return s.substr(start, pos - start);
    } else if (s[pos] == '[') {

        size_t depth = 0, start = pos;
        bool inStr = false;
        while (pos < s.size()) {
            if (!inStr && s[pos]=='[') depth++;
            else if (!inStr && s[pos]==']') { depth--; if(depth==0){pos++;break;} }
            else if (s[pos]=='"') inStr=!inStr;
            else if (inStr && s[pos]=='\\') pos++;
            pos++;
        }
        return s.substr(start, pos - start);
    } else {

        size_t start = pos;
        while (pos < s.size() && s[pos]!=',' && s[pos]!='}' && s[pos]!=']' && s[pos]!=' ' && s[pos]!='\n') pos++;
        return s.substr(start, pos - start);
    }
}

inline std::string getStr(const JsonObj& obj, const std::string& key, const std::string& def="") {
    auto it = obj.find(key);
    if (it == obj.end()) return def;
    const std::string& raw = it->second;
    if (raw.size() >= 2 && raw.front()=='"') {
        size_t pos = 0;
        return parseString(raw, pos);
    }
    return def;
}

inline int getInt(const JsonObj& obj, const std::string& key, int def=0) {
    auto it = obj.find(key);
    if (it == obj.end()) return def;
    try { return std::stoi(it->second); } catch(...) { return def; }
}

inline std::vector<JsonObj> parseObjArray(const std::string& json) {
    std::vector<JsonObj> result;
    size_t pos = 0;
    pos = skipWS(json, pos);
    if (pos >= json.size() || json[pos] != '[') return result;
    JsonArr arr = parseArray(json, pos);
    for (auto& raw : arr) {
        size_t p = 0;
        p = skipWS(raw, p);
        if (p < raw.size() && raw[p] == '{') {
            result.push_back(parseObject(raw, p));
        }
    }
    return result;
}

inline std::string objToString(const JsonObj& obj) {
    std::string out = "{";
    bool first = true;
    for (auto& kv : obj) {
        if (!first) out += ",";
        out += "\"" + kv.first + "\":" + kv.second;
        first = false;
    }
    out += "}";
    return out;
}

inline std::string objArrayToString(const std::vector<JsonObj>& objs) {
    std::string out = "[\n";
    for (size_t i = 0; i < objs.size(); i++) {
        out += "  " + objToString(objs[i]);
        if (i+1 < objs.size()) out += ",";
        out += "\n";
    }
    out += "]";
    return out;
}

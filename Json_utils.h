#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
using namespace std;

inline string jsonEscape(const string& s) {
    string out;
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

inline string jStr(const string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

inline string jInt(int n) {
    return to_string(n);
}

inline string jBool(bool b) {
    return b ? "true" : "false";
}

struct JsonObject {
    vector<pair<string, string>> fields;

    JsonObject& add(const string& key, const string& rawValue) {
        fields.push_back({key, rawValue});
        return *this;
    }

    JsonObject& addStr(const string& key, const string& val) {
        return add(key, jStr(val));
    }
   
    JsonObject& addInt(const string& key, int val) {
        return add(key, jInt(val));
    }
   
    JsonObject& addBool(const string& key, bool val) {
        return add(key, jBool(val));
    }

    string build() const {
        string out = "{";
        for (size_t i = 0; i < fields.size(); i++) {
            if (i > 0) out += ",";
            out += "\"" + fields[i].first + "\":" + fields[i].second;
        }
        out += "}";
        return out;
    }
};

struct JsonArray {
    vector<string> items;

    JsonArray& push(const string& rawItem) {
        items.push_back(rawItem);
        return *this;
    }

    string build() const {
        string out = "[";
        for (size_t i = 0; i < items.size(); i++) {
            if (i > 0) out += ",";
            out += items[i];
        }
        out += "]";
        return out;
    }
};

inline size_t skipWS(const string& s, size_t i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t')) i++;
    return i;
}

inline string parseString(const string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"')
        throw runtime_error("Expected '\"' at pos " + to_string(pos));
    pos++; // skip opening quote
    string result;
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
using JsonObj  = map<string, string>; 
using JsonArr  = vector<string>;            

inline string parseValue(const string& s, size_t& pos);

inline JsonObj parseObject(const string& s, size_t& pos) {
    JsonObj obj;
    if (pos >= s.size() || s[pos] != '{')
        throw runtime_error("Expected '{' at pos " + to_string(pos));
    pos++;
    pos = skipWS(s, pos);
    while (pos < s.size() && s[pos] != '}') {
        pos = skipWS(s, pos);
        string key = parseString(s, pos);
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ':') pos++;
        pos = skipWS(s, pos);
        string val = parseValue(s, pos);
        obj[key] = val;
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ',') pos++;
        pos = skipWS(s, pos);
    }
    if (pos < s.size()) pos++; 
    return obj;
}

inline JsonArr parseArray(const string& s, size_t& pos) {
    JsonArr arr;
    if (pos >= s.size() || s[pos] != '[')
        throw runtime_error("Expected '[' at pos " + to_string(pos));
    pos++; 
    pos = skipWS(s, pos);
    while (pos < s.size() && s[pos] != ']') {
        pos = skipWS(s, pos);
        if (s[pos] == ']') break;
        string val = parseValue(s, pos);
        arr.push_back(val);
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ',') pos++;
        pos = skipWS(s, pos);
    }
    if (pos < s.size()) pos++; 
    return arr;
}

inline string parseValue(const string& s, size_t& pos) {
    pos = skipWS(s, pos);
    if (pos >= s.size()) return "null";

    if (s[pos] == '"') {
      
        size_t start = pos;
        string str = parseString(s, pos);
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

inline string getStr(const JsonObj& obj, const string& key, const string& def="") {
    auto it = obj.find(key);
    if (it == obj.end()) return def;
    const string& raw = it->second;
    if (raw.size() >= 2 && raw.front()=='"') {
        size_t pos = 0;
        return parseString(raw, pos);
    }
    return def;
}

inline int getInt(const JsonObj& obj, const string& key, int def=0) {
    auto it = obj.find(key);
    if (it == obj.end()) return def;
    try { return stoi(it->second); } catch(...) { return def; }
}

inline vector<JsonObj> parseObjArray(const string& json) {
    vector<JsonObj> result;
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

inline string objToString(const JsonObj& obj) {
    string out = "{";
    bool first = true;
    for (auto& kv : obj) {
        if (!first) out += ",";
        out += "\"" + kv.first + "\":" + kv.second;
        first = false;
    }
    out += "}";
    return out;
}

inline string objArrayToString(const vector<JsonObj>& objs) {
    string out = "[\n";
    for (size_t i = 0; i < objs.size(); i++) {
        out += "  " + objToString(objs[i]);
        if (i+1 < objs.size()) out += ",";
        out += "\n";
    }
    out += "]";
    return out;
}

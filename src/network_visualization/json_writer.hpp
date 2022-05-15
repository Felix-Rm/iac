#pragma once

#include "../exceptions.hpp"
#include "../std_provider/string.hpp"
#include "../std_provider/unordered_map.hpp"
#include "../std_provider/utility.hpp"
#include "../std_provider/vector.hpp"

namespace iac {

class JSONObject;
class JSONArray;

class JSONValue {
   public:
    typedef union {
        JSONObject* as_object = nullptr;
        JSONArray* as_array;
        string* as_string;
        int as_int;
        double as_double;
    } value_t;

    typedef enum value_types {
        NULL_,
        OBJECT,
        ARRAY,
        STRING,
        INT,
        DOUBLE
    } value_type_t;

    explicit JSONValue(JSONObject&& object);
    explicit JSONValue(JSONArray&& array);
    explicit JSONValue(string&& str);
    explicit JSONValue(string str);
    explicit JSONValue(int number);
    explicit JSONValue(double number);

    ~JSONValue() {
        destruct();
    }

    JSONValue(const JSONValue& other) {
        copy_from(other);
    }

    JSONValue(JSONValue&& other) {
        move_from(other);
    }

    JSONValue& operator=(const JSONValue& other) {
        if (&other != this)
            copy_from(other);
        return *this;
    }

    JSONValue& operator=(JSONValue&& other) {
        if (&other != this)
            move_from(other);
        return *this;
    }

    string stringify();

   private:
    void copy_from(const JSONValue& other);
    void move_from(JSONValue& other);
    void destruct();

    value_t m_value;
    value_type_t m_value_type{value_types::NULL_};
};

class JSONObject {
   public:
    void add(string key, JSONValue&& value);

    string stringify();

   private:
    unordered_map<string, JSONValue> m_values;
};

class JSONArray {
   public:
    void add(JSONValue&& value);

    string stringify();

   private:
    vector<JSONValue> m_values;
};

}  // namespace iac
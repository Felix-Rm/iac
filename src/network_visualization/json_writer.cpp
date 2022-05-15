#include "json_writer.hpp"

namespace iac {

JSONValue::JSONValue(JSONObject&& object)
    : m_value_type(value_types::OBJECT) {
    auto* obj_ptr = new JSONObject();
    *obj_ptr = iac::move(object);
    m_value.as_object = obj_ptr;
};

JSONValue::JSONValue(JSONArray&& array)
    : m_value_type(value_types::ARRAY) {
    auto* arr_ptr = new JSONArray();
    *arr_ptr = iac::move(array);
    m_value.as_array = arr_ptr;
};

JSONValue::JSONValue(string&& str)
    : m_value_type(value_types::STRING) {
    auto* str_ptr = new string();
    *str_ptr = iac::move(str);
    m_value.as_string = str_ptr;
};

JSONValue::JSONValue(string str)
    : m_value_type(value_types::STRING) {
    auto* str_ptr = new string();
    *str_ptr = iac::move(str);
    m_value.as_string = str_ptr;
};

JSONValue::JSONValue(int number)
    : m_value_type(value_types::INT) {
    m_value.as_int = number;
};

JSONValue::JSONValue(double number)
    : m_value_type(value_types::DOUBLE) {
    m_value.as_double = number;
};

void JSONValue::destruct() {
    switch (m_value_type) {
        case value_types::STRING:
            delete m_value.as_string;
            break;
        case value_types::OBJECT:
            delete m_value.as_object;
            break;
        case value_types::ARRAY:
            delete m_value.as_array;
            break;
        default:
            break;
    }
}

void JSONValue::copy_from(const JSONValue& other) {
    destruct();
    m_value_type = other.m_value_type;
    switch (m_value_type) {
        case value_types::STRING:
            m_value.as_string = new string();
            *m_value.as_string = *other.m_value.as_string;
            break;
        case value_types::INT:
            m_value.as_int = other.m_value.as_int;
            break;
        case value_types::DOUBLE:
            m_value.as_double = other.m_value.as_double;
            break;
        case value_types::OBJECT:
            m_value.as_object = new JSONObject();
            *m_value.as_object = *other.m_value.as_object;
            break;
        case value_types::ARRAY:
            m_value.as_array = new JSONArray();
            *m_value.as_array = *other.m_value.as_array;
            break;
        case value_types::NULL_:
            break;
        default:
            IAC_ASSERT_NOT_REACHED();
    }
};

void JSONValue::move_from(JSONValue& other) {
    destruct();
    m_value_type = other.m_value_type;
    m_value = other.m_value;

    other.m_value_type = NULL_;
};

bool JSONValue::operator==(const JSONValue& other) const {
    if (m_value_type != other.m_value_type)
        return false;

    switch (m_value_type) {
        case value_types::STRING:
            return m_value.as_string == other.m_value.as_string;
        case value_types::INT:
            return m_value.as_int == other.m_value.as_int;
        case value_types::DOUBLE:
            return m_value.as_double == other.m_value.as_double;
        case value_types::OBJECT:
            return m_value.as_object == other.m_value.as_object;
        case value_types::ARRAY:
            return m_value.as_array == other.m_value.as_array;
        case value_types::NULL_:
            return "null";
        default:
            IAC_ASSERT_NOT_REACHED();
    }
}

string JSONValue::stringify() const {
    switch (m_value_type) {
        case value_types::STRING:
            return '"' + *(m_value.as_string) + '"';
        case value_types::INT:
            return to_string(m_value.as_int);
        case value_types::DOUBLE:
            return to_string(m_value.as_double);
        case value_types::OBJECT:
            return m_value.as_object->stringify();
        case value_types::ARRAY:
            return m_value.as_array->stringify();
        case value_types::NULL_:
            return "null";
        default:
            IAC_ASSERT_NOT_REACHED();
    }
}

void JSONObject::add(string key, JSONValue&& value) {
    m_values.emplace(key, value);
};

string JSONObject::stringify() {
    string output = "{";

    for (auto it = m_values.begin(); it != m_values.end(); it++) {
        if (it != m_values.begin()) output += ',';
        output += '"' + it->first + "\":" + it->second.stringify();
    }

    output += '}';

    return output;
};

void JSONArray::add(JSONValue&& value) {
    m_values.emplace_back(value);
};

string JSONArray::stringify() {
    string output = "[";

    for (auto it = m_values.begin(); it != m_values.end(); it++) {
        if (it != m_values.begin()) output += ',';
        output += it->stringify();
    }

    output += ']';

    return output;
};

}  // namespace iac
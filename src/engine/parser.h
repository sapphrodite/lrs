#ifndef PARSE_H
#define PARSE_H

#include <common/raii_types.h>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <strstream>
#include <unordered_map>
#include <optional>

namespace dsl {

    //////////////////////////////////////////
    ///     PARSER OBJECT DECLARATIONS     ///
    //////////////////////////////////////////

    class object {
    public:
        virtual ~object() noexcept = default;
        virtual bool is_primitive() const = 0;
    protected:
        object() noexcept = default;
        [[no_unique_address]] no_copy disable_copy;
        [[no_unique_address]] no_move disable_move;
    };

    template <typename T>
    class primitive final : public object {
    public:
        const T& get() const noexcept { return value; }
        bool is_primitive() const { return true; }
    private:
        T value;
        friend class parser;
    };

    template <typename storage_type, typename key_type>
    class structure : public object {
    public:
        template <typename T>
        T get(key_type identifier) const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] const storage_type& data() const;
        bool is_primitive() const { return false; }
    private:
        storage_type _data;
        friend class parser;
    };

    class function : public object {
    public:
        const char* read_command(size_t idx) { return data[idx].c_str(); }
        size_t num_commands() { return data.size(); }
        void add_command(const std::string& str) { data.emplace_back(str); }
        bool is_primitive() const { return false; }
    private:
        std::vector<std::string> data;
    };

    using dict = structure<std::unordered_map<std::string, std::unique_ptr<object>>, std::string>;
    using list = structure<std::vector<std::unique_ptr<object>>, size_t>;
    using parsed_file = std::unique_ptr<dict>;


    template <typename T>
    concept is_primitive = std::is_same_v<int, T> || std::is_same_v<float, T> || std::is_same_v<std::string, T> || std::is_same_v<bool, T>;
    template <typename T>
    concept is_parser_object = std::is_base_of<object, std::remove_pointer_t<T>>::value;
    template <typename T>
    concept is_deserializable = requires(const object* obj) { T::deserialize(obj); };
    template <typename T>
    concept is_serializable = requires(T t) { t.serialize(); };
    template <typename T>
    concept is_vector_instance = std::is_same_v<std::vector<typename T::value_type>, T>;


    ///////////////////////////////////////////////
    ///     PARSER AND PRINTER DECLARATIONS     ///
    ///////////////////////////////////////////////


    class parser {
    public:
        parser(std::filebuf& fb) : source(&fb) {}
        parser(std::strstreambuf& sb) : source(&sb) {};

        std::unique_ptr<dict> parse_dict();
        static std::unique_ptr<dict> parse_file(std::string filename);
    private:
        enum class byte_class : unsigned char {
            skippable,
            identifier,
            other,
        };

        std::string parse_identifier();
        std::unique_ptr<object> parse_primitive_value();
        std::unique_ptr<primitive<std::string>> parse_string();
        std::unique_ptr<list> parse_list();
        std::unique_ptr<function> parse_function();
        std::optional<char> peek_byte();
        static byte_class classify_byte(char c) noexcept;
        bool skip_skippables();
        bool skip_comment();
        void discard_peek();

        std::istream source;
    };


    class printer {
    public:
        void write(std::string filename);
        std::string data();

        void append(std::string text);
        template <typename T>
        void append(std::string key, T& value);
        template <typename... Args>
        void append(std::string key, Args... args);
        // Functions for formatting a collection of key/value pairs
        void open_dict(std::string text);
        void close_dict();
    private:
        void regen_tab_buffer();

        std::string tab_buffer;
        std::string _data;
        int num_indents = 0;
    };


    ///////////////////////////////////////
    ///     SERIALIZATION FUNCTIONS     ///
    ///////////////////////////////////////


    template <is_serializable T>
    std::string serialize(T& value) { return value.serialize(); };
    // Specializations for parser primitives - integers, booleans and strings
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    std::string serialize(T value) { return std::to_string(value); };
    static std::string serialize(bool value) { return value ? "true" : "false"; };
    static std::string serialize(std::string& value) { return "\"" + value + "\"";}
    // Abstraction for variadic serialization
    template <typename T, typename... Args>
    std::string serialize(T value, Args... args) { return serialize(value) + ", " + serialize(args...); }
    // Specialization for std::vector
    template <typename T, std::enable_if_t<std::is_same_v<std::vector<typename T::value_type>, T>, bool> = true>
    std::string serialize(T& vector) {
        std::string serial = "{";
        for (auto& element : vector) {
            serial += serialize(element) + ", ";
        }
        return serial + "}";
    }


    /////////////////////////////////////////
    ///     DESERIALIZATION FUNCTIONS     ///
    /////////////////////////////////////////

    template <is_primitive T>
    T deserialize(const object* obj) { return obj ? dynamic_cast<const primitive<T>*>(obj)->get() : T(); }
    template <is_parser_object T>
    T deserialize(const object* obj) { return dynamic_cast<T>(obj); }
    template <is_deserializable T>
    T deserialize(const object* obj) { return T::deserialize(obj); }
    template <is_vector_instance T>
    T deserialize(const object* obj) {
        const auto* l = dynamic_cast<const list*>(obj);
        T vec;
        vec.reserve(l->size());
        for (auto& element : l->data()) {
            vec.emplace_back(deserialize<typename T::value_type>(element.get()));
        }
        return vec;
    }

    ////////////////////////////////////////
    ///     IMPLEMENTATION FUNCTIONS     ///
    ////////////////////////////////////////


    template <typename T, typename U>
    template <typename K>
    K structure<T, U>::get(U identifier) const { return deserialize<K>(_data.at(identifier).get()); }
    template <typename T, typename U>
    [[nodiscard]] size_t structure<T, U>::size() const { return _data.size(); }
    template <typename T, typename U>
    [[nodiscard]] const T& structure<T, U>::data() const { return _data; }

    template <typename T>
    void printer::append(std::string key, T& value) { _data += tab_buffer + key + " = " + serialize(value) + "\n"; }
    template <typename... Args>
    void printer::append(std::string key, Args... args) { _data += tab_buffer + key + " = {" + serialize(args...) + "}\n"; }

    static std::filebuf open_file(std::string filename) {
        std::filebuf fb;
        if(!fb.open(filename, std::ios::in))
            throw std::runtime_error("Cannot open requested file");
        return fb;
    }
}

#endif //PARSE_H

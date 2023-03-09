#include "parser.h"

#include <climits>
#include <array>
#include <memory>

namespace dsl {


    ///////////////////////////////////////////
    ///     PARSER FUNCTION DEFINITIONS     ///
    ///////////////////////////////////////////

    std::unique_ptr<dict> parser::parse_dict() {
        auto d = std::make_unique<dict>();
        for (;;) {
            if (!skip_skippables()) {
                break;
            }
            const auto first_byte = peek_byte().value();
            if (first_byte == '}') {
                discard_peek();
                break;
            }
            if (classify_byte(first_byte) != byte_class::identifier) {
                break;
            }
            auto item_name = parse_identifier();
            if (!skip_skippables()) {
                throw std::runtime_error("Expected item declaration but end of input found");
            }

            if (item_name == "function") {
                skip_skippables();
                // overwrite with the REAL function name
                auto function_name = parse_identifier();
                skip_skippables();
                const auto c = peek_byte().value();
                if (c == '{') {
                    discard_peek();
                    auto value = parse_function();
                    skip_skippables();
                    d.get()->_data.insert_or_assign(std::move(function_name), std::move(value));
                } else {
                    throw std::runtime_error(std::string("Unexpected character: ") + c);
                }
            } else {
                const auto c = peek_byte().value();
                if (c == '=') {
                    discard_peek();
                    auto value = parse_primitive_value();
                    skip_skippables();
                    d.get()->_data.insert_or_assign(std::move(item_name), std::move(value));
                } else {
                    throw std::runtime_error(std::string("Unexpected character: ") + c);
                }
            }
        }
        return d;
    }

    std::unique_ptr<dict> parser::parse_file(std::string filename) {
        std::filebuf fb = open_file(filename);
        return parser(fb).parse_dict();
    }

    std::string parser::parse_identifier() {
        std::string result;
        for (;;) {
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                break;
            }
            const auto c = *maybe_byte;
            if (classify_byte(c) != byte_class::identifier) {
                break;
            }
            discard_peek();
            result.push_back(c);
        }
        return result;
    }

    std::unique_ptr<list> parser::parse_list() {
        discard_peek();
        auto l = std::make_unique<list>();
        for (;;) {
            skip_skippables();
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                throw std::runtime_error(std::string("List unterminated at end of file"));
            }
            switch (*maybe_byte) {
                case ')': {
                    discard_peek();
                    return l;
                }
                case ',':
                    discard_peek();
                    [[fallthrough]];
                default:
                    l->_data.emplace_back(parse_primitive_value());
                    break;
            }
        }
    }

    std::unique_ptr<object> parser::parse_primitive_value() {
        const auto is_numeric = [](const std::string &string) {
            if (string.empty()) {
                return false;
            }
            const std::size_t start_index = (string[0] == '-') ? 1 : 0;
            for (auto i = start_index; i < string.size(); ++i) {
                const auto c = string[i];
                if (c < '0' || c > '9') {
                    return false;
                }
            }
            return true;
        };

        if (!skip_skippables()) {
            throw std::runtime_error("Expected a primitive value but end of input found");
        }
        if (peek_byte().value() == '"') {
            return parse_string();
        }
        if (peek_byte().value() == '(') {
            skip_skippables();
            return parse_list();
        }
        if (peek_byte().value() == '{') {
            skip_skippables();
            discard_peek();
            return parse_dict();
        }
        const auto identifier = parse_identifier();
        if (identifier == "true" || identifier == "false") {
            auto new_bool = std::make_unique<primitive<bool>>();
            new_bool->value = identifier == "true";
            return new_bool;
        } else if (is_numeric(identifier)) {
            auto new_int = std::make_unique<primitive<int>>();
            new_int->value = std::stoi(identifier);
            return new_int;
        } else {
            throw std::runtime_error("Unexpected identifier: " + identifier);
        }
    }

    std::unique_ptr<primitive<std::string>> parser::parse_string() {
        discard_peek();
        auto new_string = std::make_unique<primitive<std::string>>();
        for (;;) {
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                throw std::runtime_error("End of input found within a string literal");
            }
            discard_peek();
            const auto c = *maybe_byte;
            if (c == '"') {
                break;
            } else if (c == '\\') {
                continue;
            } else if (c == '\n') {
                continue;
            }
            new_string->value.push_back(c);
        }
        return new_string;
    }

    // this can probably be improved by treating each line as an array of identifiers....
    std::unique_ptr<function> parser::parse_function() {
        skip_skippables();
        std::string current_line = "";
        auto new_func = std::make_unique<function>();
        for (;;) {
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                throw std::runtime_error("End of input found within a function");
            }
            const auto c = *maybe_byte;
            if (c == '}') {
                discard_peek();
                break;
            } else if (c == '=') {
                discard_peek();
                skip_skippables();
            } else if (c == ';') {
                discard_peek();
                new_func->add_command(current_line);
                current_line = "";
                skip_skippables();
                continue;
            } else if (c == ' ') {
                current_line += c;
                skip_skippables();
            } else if (classify_byte(c) == byte_class::identifier) {
                current_line += parse_identifier();
            } else {
                skip_skippables();
            }
        }
        return new_func;
    }

    bool parser::skip_skippables() {
        for (;;) {
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                return false;
            }
            const auto c = *maybe_byte;
            if (c == '#') {
                discard_peek();
                if (!skip_comment()) {
                    return false;
                }
            } else if (classify_byte(c) == byte_class::skippable) {
                discard_peek();
            } else {
                return true;
            }
        }
    }

    bool parser::skip_comment() {
        for (;;) {
            const auto maybe_byte = peek_byte();
            if (!maybe_byte) {
                return false;
            }
            if (*maybe_byte == '\n') {
                return true;
            }
            discard_peek();
        }
    }

    std::optional<char> parser::peek_byte() {
        const auto maybe_byte = source.peek();
        return (maybe_byte != std::char_traits<char>::eof())
               ? std::optional(static_cast<char>(maybe_byte))
               : std::nullopt;
    }

    void parser::discard_peek() {
        source.get();
    }

    auto parser::classify_byte(char c) noexcept -> byte_class {
        static auto byte_classes = [] {
            std::array<byte_class, UCHAR_MAX> array = {};
            array.fill(byte_class::other);
            for (unsigned c = 'a'; c <= 'z'; ++c) {
                array[c] = byte_class::identifier;
            }
            for (unsigned c = 'A'; c <= 'Z'; ++c) {
                array[c] = byte_class::identifier;
            }
            for (unsigned c = '0'; c <= '9'; ++c) {
                array[c] = byte_class::identifier;
            }
            array['-'] = byte_class::identifier;
            array['_'] = byte_class::identifier;
            array['.'] = byte_class::identifier;
            array['$'] = byte_class::identifier;
            array[' '] = byte_class::skippable;
            array['#'] = byte_class::skippable;
            array[' '] = byte_class::skippable;
            array['\n'] = byte_class::skippable;
            array['\r'] = byte_class::skippable;
            array['\t'] = byte_class::skippable;
            array['\f'] = byte_class::skippable;
            array['\v'] = byte_class::skippable;
            return array;
        }();
        return byte_classes[static_cast<unsigned char>(c)];
    }


    ////////////////////////////////////////////
    ///     PRINTER FUNCTION DEFINITIONS     ///
    ////////////////////////////////////////////


    void printer::write(std::string filename) { std::ofstream(filename) << _data; };
    void printer::append(std::string text) { _data += tab_buffer + text + '\n'; }
    // These functions handle formatting a collection of key/value pairs
    void printer::open_dict(std::string text) {
        _data += tab_buffer + text + " {\n";
        num_indents++;
        regen_tab_buffer();
    }
    void printer::close_dict() {
        num_indents--;
        regen_tab_buffer();
        _data += tab_buffer + "}";
    }
    std::string printer::data() { return _data; }

    void printer::regen_tab_buffer() {
        std::string new_buffer = "";
        for (int i = 0; i < num_indents; i++) { new_buffer += "    "; }
        tab_buffer = new_buffer;
    }
}

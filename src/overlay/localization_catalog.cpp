#include "localization_catalog.hpp"

#include <cstdint>
#include <string>

namespace overlay
{
    namespace
    {
        bool ValidUtf8(std::string_view text)
        {
            for (std::size_t i = 0; i < text.size();) {
                const auto first = static_cast<unsigned char>(text[i]);
                std::size_t count{};
                std::uint32_t cp{};
                if (first <= 0x7f) { ++i; continue; }
                if (first >= 0xc2 && first <= 0xdf) { count = 2; cp = first & 0x1f; }
                else if (first >= 0xe0 && first <= 0xef) { count = 3; cp = first & 0x0f; }
                else if (first >= 0xf0 && first <= 0xf4) { count = 4; cp = first & 0x07; }
                else return false;
                if (i + count > text.size()) return false;
                for (std::size_t j = 1; j < count; ++j) {
                    const auto next = static_cast<unsigned char>(text[i + j]);
                    if ((next & 0xc0) != 0x80) return false;
                    cp = (cp << 6) | (next & 0x3f);
                }
                if ((count == 3 && cp < 0x800) || (count == 4 && cp < 0x10000) ||
                    cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
                i += count;
            }
            return true;
        }

        template <typename EntryMap>
        class JsonObjectParser
        {
        public:
            explicit JsonObjectParser(std::string_view input) : input_(input) {}

            bool Parse(EntryMap& entries,
                std::string& error)
            {
                entries_ = &entries;
                SkipSpace();
                if (!Object({}, 0)) { error = error_; return false; }
                SkipSpace();
                if (position_ != input_.size()) {
                    Fail("trailing data after catalog object");
                    error = error_;
                    return false;
                }
                return true;
            }

        private:
            bool Object(const std::string& prefix, unsigned depth)
            {
                if (depth > 16) return Fail("catalog nesting exceeds limit");
                if (!Take('{')) return Fail("expected JSON object");
                SkipSpace();
                if (Take('}')) return true;
                for (;;) {
                    std::string name;
                    if (!String(name)) return false;
                    if (name.empty()) return Fail("empty catalog key segment");
                    SkipSpace();
                    if (!Take(':')) return Fail("expected ':' after catalog key");
                    SkipSpace();
                    const std::string path = prefix.empty() ? name : prefix + "." + name;
                    if (Peek('{')) {
                        if (!Object(path, depth + 1)) return false;
                    } else {
                        std::string value;
                        if (!String(value)) return false;
                        if (!entries_->emplace(path, std::move(value)).second)
                            return Fail("duplicate flattened catalog key");
                    }
                    SkipSpace();
                    if (Take('}')) return true;
                    if (!Take(',')) return Fail("expected ',' or '}' in catalog object");
                    SkipSpace();
                }
            }

            bool String(std::string& result)
            {
                if (!Take('"')) return Fail("catalog keys and values must be strings/objects");
                while (position_ < input_.size()) {
                    const auto ch = static_cast<unsigned char>(input_[position_++]);
                    if (ch == '"') return true;
                    if (ch < 0x20) return Fail("unescaped control character in string");
                    if (ch != '\\') { result.push_back(static_cast<char>(ch)); continue; }
                    if (position_ >= input_.size()) return Fail("incomplete string escape");
                    switch (input_[position_++]) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u': if (!UnicodeEscape(result)) return false; break;
                    default: return Fail("invalid string escape");
                    }
                }
                return Fail("unterminated JSON string");
            }

            bool UnicodeEscape(std::string& result)
            {
                std::uint32_t cp{};
                if (!Hex4(cp)) return false;
                if (cp >= 0xd800 && cp <= 0xdbff) {
                    if (position_ + 2 > input_.size() || input_[position_] != '\\' ||
                        input_[position_ + 1] != 'u')
                        return Fail("unpaired high surrogate");
                    position_ += 2;
                    std::uint32_t low{};
                    if (!Hex4(low) || low < 0xdc00 || low > 0xdfff)
                        return Fail("invalid low surrogate");
                    cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                } else if (cp >= 0xdc00 && cp <= 0xdfff) {
                    return Fail("unpaired low surrogate");
                }
                AppendUtf8(result, cp);
                return true;
            }

            bool Hex4(std::uint32_t& value)
            {
                if (position_ + 4 > input_.size()) return Fail("incomplete unicode escape");
                value = 0;
                for (int i = 0; i < 4; ++i) {
                    const char ch = input_[position_++];
                    value <<= 4;
                    if (ch >= '0' && ch <= '9') value |= ch - '0';
                    else if (ch >= 'a' && ch <= 'f') value |= ch - 'a' + 10;
                    else if (ch >= 'A' && ch <= 'F') value |= ch - 'A' + 10;
                    else return Fail("invalid unicode escape");
                }
                return true;
            }

            static void AppendUtf8(std::string& out, std::uint32_t cp)
            {
                if (cp <= 0x7f) out.push_back(static_cast<char>(cp));
                else if (cp <= 0x7ff) {
                    out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
                } else if (cp <= 0xffff) {
                    out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
                } else {
                    out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
                }
            }

            bool Peek(char ch) const noexcept
            { return position_ < input_.size() && input_[position_] == ch; }
            bool Take(char ch)
            {
                if (!Peek(ch)) return false;
                ++position_;
                return true;
            }
            void SkipSpace()
            {
                while (position_ < input_.size() && (input_[position_] == ' ' ||
                    input_[position_] == '\t' || input_[position_] == '\r' ||
                    input_[position_] == '\n')) ++position_;
            }
            bool Fail(const char* message)
            {
                if (error_.empty()) error_ = message;
                return false;
            }

            std::string_view input_;
            std::size_t position_{};
            std::string error_;
            EntryMap* entries_{};
        };
    }

    bool LocalizationCatalog::LoadJson(std::string_view json, std::string& error)
    {
        loaded_ = false;
        entries_.clear();
        if (!ValidUtf8(json)) { error = "catalog is not valid UTF-8"; return false; }
        decltype(entries_) parsed;
        JsonObjectParser<decltype(parsed)> parser(json);
        if (!parser.Parse(parsed, error)) return false;
        entries_ = std::move(parsed);
        loaded_ = true;
        return true;
    }

    std::string_view LocalizationCatalog::Find(loc::Key key) const
    {
        const auto found = entries_.find(key.path);
        return loaded_ && found != entries_.end()
            ? std::string_view(found->second) : std::string_view{};
    }

    bool LocalizationCatalog::Contains(loc::Key key) const
    { return loaded_ && entries_.contains(key.path); }

    std::vector<std::string_view> LocalizationCatalog::Keys() const
    {
        std::vector<std::string_view> keys;
        keys.reserve(entries_.size());
        for (const auto& [key, value] : entries_) {
            (void)value;
            keys.emplace_back(key);
        }
        return keys;
    }
}

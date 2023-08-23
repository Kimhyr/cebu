#pragma once
#define CEBU_INCLUDED_LEXER_H

#include <cstdint>
#include <unordered_map>

#include <cebu/token.h>
#include <cebu/diagnostics.h>

namespace cebu
{

enum class lexing_error
{
   incomplete_character,
   unknown_character,
   number_overflow,
   unknown_escaped_character,
   multiple_decimal_points
};

struct lexer_marker
{
    using const_pointer = char const*;

    const_pointer pointer{nullptr};
    const_pointer line_pointer{pointer};
    std::size_t   line_number{1}; 
};

struct lexer_flags
{
    std::uint64_t
        reverted : 1 = false,
        padding : 63;
};

class lexer
{
   friend class parser;

public:
   using const_pointer = char const*;
      
   lexer() noexcept
   {}
      
   void load(std::string_view file_path, const_pointer pointer) noexcept
   {
      m_file_path = file_path;
      m_marker.pointer = pointer;
      m_marker.line_pointer = pointer;
      m_marker.line_number = 1;
   }

   result lex(token&) noexcept;

    /// WARNING: This assumes the lexer already lexed something.
    void revert() noexcept
    {
        m_flags.reverted = true;
        lexer_marker temp{m_marker};
        m_marker = m_prior_marker;
        m_prior_marker = temp;
    }

   [[nodiscard]]
   position position() const noexcept
   {
      return {
         line_number(),
         static_cast<std::size_t>(pointer() - line_pointer() + 1)
      };
   }

   [[nodiscard]]
   location location() const noexcept
   {
      return {
         file_path(),
         position()
      };
   }

   [[nodiscard]]
   static std::unordered_map<std::string_view, token_type> const&
      get_keywords() noexcept
   {
      [[clang::no_destroy]]
      static std::unordered_map<std::string_view, token_type> keywords {
         {"b8"    , token_type::b8},
         {"b16"   , token_type::b16},
         {"b32"   , token_type::b32},
         {"b64"   , token_type::b64},
         {"i8"    , token_type::i8},
         {"i16"   , token_type::i16},
         {"i32"   , token_type::i32},
         {"i64"   , token_type::i64},
         {"f16"   , token_type::f16},
         {"f32"   , token_type::f32},
         {"f64"   , token_type::f64},
         {"method", token_type::method},
         {"trait" , token_type::trait},
         {"type"  , token_type::type},
         {"static", token_type::static_},
         {"let"   , token_type::let},
         {"if"    , token_type::if_},
         {"else"  , token_type::else_},
         {"elif"  , token_type::elif},
         {"return", token_type::return_},
      };
      return keywords;
   }

   [[nodiscard]]
   char current() const noexcept
   {
      return *pointer();
   }

   [[nodiscard]]
   char peek() const noexcept
   {
      return pointer()[1];
   }

   [[nodiscard]]
   const_pointer pointer() const noexcept
   {
      return m_marker.pointer;
   }

   [[nodiscard]]
   const_pointer line_pointer() const noexcept
   {
      return m_marker.line_pointer;
   }

   [[nodiscard]]
   std::size_t line_number() const noexcept
   {
      return m_marker.line_number;
   }

   [[nodiscard]]
   std::string_view const& file_path() const noexcept
   {
      return m_file_path;
   }

    [[nodiscard]]
    lexer_flags const& flags() const noexcept
    {
        return m_flags;
    }
 
private:
    std::string_view m_file_path;
    lexer_marker     m_prior_marker;
    lexer_marker     m_marker;
    lexer_flags      m_flags;

    enum class character_result
    {
        failure = -1,
        regular = 0,
        escaped = 1
    };

   character_result lex_escaped_character() noexcept;

   template<lexing_error Error, typename ...Args>
   void report(struct position const&, Args&&...);

   void consume() noexcept
   {
      if (current() == '\0') [[unlikely]]
         return;
      if (current() == '\n') [[unlikely]]
         m_marker.line_pointer = pointer();
      ++m_marker.pointer;
   }
};

}

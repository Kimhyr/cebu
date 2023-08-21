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
   unknown_escaped_character
};

class lexer
{
   friend class parser;

public:
   using const_pointer = std::string::const_iterator;
      
   lexer() noexcept
   {}
      
   void load(std::string_view file_path, const_pointer pointer) noexcept
   {
      m_file_path = file_path;
      m_pointer = pointer;
      m_line_pointer = pointer;
      m_line_number = 1;
   }

   result lex(token&) noexcept;

   [[nodiscard]]
   position position() const noexcept
   {
      return {
         line_number(),
         static_cast<std::size_t>(pointer() - line_pointer())
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
      return *m_pointer;   
   }

   [[nodiscard]]
   char peek() const noexcept
   {
      return m_pointer[1];
   }

   [[nodiscard]]
   const_pointer pointer() const noexcept
   {
      return m_pointer;
   }

   [[nodiscard]]
   const_pointer line_pointer() const noexcept
   {
      return m_line_pointer;
   }

   [[nodiscard]]
   std::size_t line_number() const noexcept
   {
      return m_line_number;
   }

   [[nodiscard]]
   std::string_view file_path() const noexcept
   {
      return m_file_path;   
   }
 
private:
   std::string_view m_file_path;
   const_pointer m_pointer{nullptr};
   const_pointer m_line_pointer{m_pointer};
   std::size_t m_line_number{1};

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
         m_line_pointer = m_pointer;
      ++m_pointer;
   }
};

}

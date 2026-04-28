#include "../../include/config/Tokenizer.hpp"
#include <climits>
#include <cstddef>
#include <stdexcept>

std::string trim(std::string s) 
{
  int start = 0;
  int end = (int)s.size() - 1;

  while (start <= end &&
         (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' ||
          s[start] == '\v' || s[start] == '\f' || s[start] == '\r')) 
  {
    start++;
  }

  while (end >= start &&
         (s[end] == ' ' || s[end] == '\t' || s[end] == '\n' ||
          s[end] == '\v' || s[end] == '\f' || s[end] == '\r')) 
  {
    end--;
  }

  if (start > end)
    return "";

  return s.substr(start, end - start + 1);
}

void Tokenizer::tokenize(std::string s) 
{
  std::string current_token;
  for (size_t i = 0; i < s.size(); i++) 
  {
    char c = s[i];
    if (std::isspace(static_cast<unsigned char>(c))) 
    {
      if (!current_token.empty()) 
      {
        tokens.push_back(current_token);
        current_token.clear();
      }
    } 
    else if (c == '{' || c == '}' || c == ';') 
    {
      if (!current_token.empty()) 
      {
        tokens.push_back(current_token);
        current_token.clear();
      }
      tokens.push_back(std::string(1, c));
    } 
    else 
    {
      current_token += c;
    }
  }
  if (!current_token.empty())
    tokens.push_back(current_token);
}

void Tokenizer::parse(const char *file_path) 
{
  std::ifstream file(file_path);
  if (!file.is_open())
    throw std::runtime_error("could not open file");
  std::string s;
  while (std::getline(file, s)) 
  {
    size_t comment_pos = s.find('#');
    if (comment_pos != std::string::npos)
      s = s.substr(0, comment_pos);
    s = trim(s);
    if (s.empty())
      continue;
    tokenize(s);
  }
  if (tokens.empty())
    throw std::runtime_error("config file is empty");
}

Tokenizer::Tokenizer(const char *s) { parse(s); }

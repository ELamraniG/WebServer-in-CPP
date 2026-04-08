#pragma once

#include <cstddef>
#include <map>
#include <string>

class HTTPRequest {
    private:
  std::string method;
  std::string uri;
  std::string vers;
  std::map<std::string, std::string> headers;
  std::string body;
  std::string querString;
  bool isComplete;
  bool isChunked;
  size_t contentLength;
public:
  HTTPRequest();

  const std::string &getMethod() const;
  const std::string &getURI() const;
  const std::string &getVersion() const;
  const std::string &getQueryString() const;
  const std::string &getBody() const;
  std::string getHeader(const std::string &key) const;

  bool isItCompleted() const;
  bool isItChunked() const;
  size_t getContentLength() const;

  void setMethod(const std::string &method);
  void setURI(const std::string &uri);
  void setVersion(const std::string &version);
  void setQueryString(const std::string &query);
  void setBody(const std::string &body);
  void setHeader(const std::string &key, const std::string &value);
  void setCompleted(bool complete);
  void setChunked(bool chunked);
  void setContentLength(size_t length);
};

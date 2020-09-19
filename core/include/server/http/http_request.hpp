#pragma once

/// @file server/http/http_request.hpp

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <server/http/http_method.hpp>
#include <server/http/http_response.hpp>
#include <utils/projecting_view.hpp>
#include <utils/str_icase.hpp>

namespace server {
namespace http {

class HttpRequestImpl;

class HttpRequest final {
 public:
  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseEqual>;

  using HeadersMapKeys = decltype(utils::MakeKeysView(HeadersMap()));

  using CookiesMap = std::unordered_map<std::string, std::string>;

  using CookiesMapKeys = decltype(utils::MakeKeysView(CookiesMap()));

  explicit HttpRequest(const HttpRequestImpl& impl);
  ~HttpRequest() = default;

  request::ResponseBase& GetResponse() const;
  HttpResponse& GetHttpResponse() const;

  const HttpMethod& GetMethod() const;
  const std::string& GetMethodStr() const;
  int GetHttpMajor() const;
  int GetHttpMinor() const;
  const std::string& GetUrl() const;
  const std::string& GetRequestPath() const;
  const std::string& GetPathSuffix() const;
  std::chrono::duration<double> GetRequestTime() const;
  std::chrono::duration<double> GetResponseTime() const;

  const std::string& GetHost() const;

  const std::string& GetArg(const std::string& arg_name) const;
  const std::vector<std::string>& GetArgVector(
      const std::string& arg_name) const;
  bool HasArg(const std::string& arg_name) const;
  size_t ArgCount() const;
  std::vector<std::string> ArgNames() const;

  /// get named argument from URL path with wildcards
  const std::string& GetPathArg(const std::string& arg_name) const;
  /// get argument from URL path with wildcards by its 0-based index
  const std::string& GetPathArg(size_t index) const;
  bool HasPathArg(const std::string& arg_name) const;
  bool HasPathArg(size_t index) const;
  /// @returns number of wildcard arguments in URL path
  size_t PathArgCount() const;

  const std::string& GetHeader(const std::string& header_name) const;
  bool HasHeader(const std::string& header_name) const;
  size_t HeaderCount() const;
  HeadersMapKeys GetHeaderNames() const;

  const std::string& GetCookie(const std::string& cookie_name) const;
  bool HasCookie(const std::string& cookie_name) const;
  size_t CookieCount() const;
  CookiesMapKeys GetCookieNames() const;

  const std::string& RequestBody() const;
  void SetResponseStatus(HttpStatus status) const;

 private:
  const HttpRequestImpl& impl_;
};

}  // namespace http
}  // namespace server

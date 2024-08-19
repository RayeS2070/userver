#pragma once

#include <nghttp2/nghttp2.h>

#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>

#include <userver/server/request/request_config.hpp>

#include "http_request_constructor.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

constexpr std::string_view kSwitchingProtocolResponse{
    "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: "
    "h2c\r\n\r\n"};

constexpr std::size_t kDefaultMaxConcurrentStreams = 100;

class HttpRequestParser;

class Http2RequestParser final : public request::RequestParser {
 public:
  using OnNewRequestCb =
      std::function<void(std::shared_ptr<request::RequestBase>&&)>;

  using StreamId = std::int32_t;

  struct StreamData final {
    StreamData(HttpRequestConstructor::Config config,
               const HandlerInfoIndex& handler_info_index,
               request::ResponseDataAccounter& data_accounter,
               StreamId stream_id_, engine::io::Sockaddr remote_address);

    HttpRequestConstructor constructor;
    const StreamId stream_id;
  };

  using Streams = std::unordered_map<StreamId, StreamData>;

  Http2RequestParser(const HandlerInfoIndex& handler_info_index,
                     const request::HttpRequestConfig& request_config,
                     OnNewRequestCb&& on_new_request_cb,
                     net::ParserStats& stats,
                     request::ResponseDataAccounter& data_accounter,
                     engine::io::Sockaddr remote_address);

  Http2RequestParser(const Http2RequestParser&) = delete;
  Http2RequestParser(Http2RequestParser&&) = delete;
  Http2RequestParser& operator=(const Http2RequestParser&) = delete;
  Http2RequestParser& operator=(Http2RequestParser&&) = delete;

  bool Parse(const char* data, size_t size) override;

  nghttp2_session* GetNghttp2SessionPtr() { return session_.get(); }

  bool DoUpgrade(std::string_view data);

  void UpgradeToHttp2(std::string_view client_magic);

 private:
  static int OnFrameRecv(nghttp2_session* session, const nghttp2_frame* frame,
                         void* user_data);

  static int OnHeader(nghttp2_session* session, const nghttp2_frame* frame,
                      const uint8_t* name, size_t namelen, const uint8_t* value,
                      size_t valuelen, uint8_t flags, void* user_data);

  static int OnStreamClose(nghttp2_session* session, int32_t stream_id,
                           uint32_t error_code, void* user_data);

  static int OnBeginHeaders(nghttp2_session* session,
                            const nghttp2_frame* frame, void* user_data);

  static int OnDataChunkRecv(nghttp2_session* session, uint8_t flags,
                             int32_t stream_id, const uint8_t* data, size_t len,
                             void* user_data);

  static long OnSend(nghttp2_session* session, const uint8_t* data,
                     size_t length, int flags, void* user_data);

  void RegisterStreamData(StreamId stream_id);
  void RemoveStreamData(StreamId stream_id);
  void SubmitRequest(std::shared_ptr<request::RequestBase>&& request);
  int FinalizeRequest(StreamData& stream_data);

 private:
  using SessionPtr =
      std::unique_ptr<nghttp2_session,
                      std::function<decltype(nghttp2_session_del)>>;

  SessionPtr session_{nullptr};
  Streams streams_;
  std::size_t max_concurrent_streams_{kDefaultMaxConcurrentStreams};

  const HandlerInfoIndex& handler_info_index_;
  const HttpRequestConstructor::Config request_constructor_config_;

  bool upgrade_completed_{false};

  OnNewRequestCb on_new_request_cb_;

  net::ParserStats& stats_;
  request::ResponseDataAccounter& data_accounter_;
  engine::io::Sockaddr remote_address_;
  // This is need to parse a upgrade request
  std::unique_ptr<HttpRequestParser> http1_parser_{nullptr};
};

}  // namespace server::http

USERVER_NAMESPACE_END
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/propagation/http_text_format.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"

#define HAVE_HTTP_DEBUG 1
#include "opentelemetry/ext/http/client/curl/http_client_curl.h"
#include "opentelemetry/ext/http/server/http_server.h"

#include "nlohmann/json.hpp"

// Using an exporter that simply dumps span data to stdout.
#include "opentelemetry/exporters/ostream/span_exporter.h"

#include <algorithm>

namespace
{
static void Setter(std::map<std::string, std::string> &carrier,
                   nostd::string_view trace_type        = "traceparent",
                   nostd::string_view trace_description = "")
{
  carrier[std::string(trace_type)] = std::string(trace_description);
}

static nostd::string_view Getter(const std::map<std::string, std::string> &carrier,
                                 nostd::string_view trace_type = "traceparent")
{
  auto it = carrier.find(std::string(trace_type));
  if (it != carrier.end())
  {
    return nostd::string_view(it->second);
  }
  return "";
}

void initTracer()
{
  auto exporter = std::unique_ptr<sdktrace::SpanExporter>(
      new opentelemetry::exporter::trace::OStreamSpanExporter);
  auto processor = std::shared_ptr<sdktrace::SpanProcessor>(
      new sdktrace::SimpleSpanProcessor(std::move(exporter)));
  auto provider = nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
      new sdktrace::TracerProvider(processor));
  // Set the global trace provider
  opentelemetry::trace::Provider::SetTracerProvider(provider);
}

nostd::shared_ptr<opentelemetry::trace::Tracer> get_tracer()
{
  auto provider = opentelemetry::trace::Provider::GetTracerProvider();
  return provider->GetTracer("w3c_tracecontext_test");
}

struct Uri
{
  std::string host;
  uint16_t port;
  std::string path;

  Uri(std::string uri)
  {
    size_t host_end = uri.substr(7, std::string::npos).find(":");
    size_t port_end = uri.substr(host_end + 1, std::string::npos).find("/");

    host = uri.substr(0, host_end + 7);
    port = std::stoi(uri.substr(7 + host_end + 1, port_end));
    path = uri.substr(host_end + port_end + 2, std::string::npos);
  }
};
}  // namespace

// A noop event handler for making HTTP requests. We don't care about response bodies and error
// messages.
class NoopEventHandler : public opentelemetry::ext::http::client::EventHandler
{
public:
  void OnEvent(opentelemetry::ext::http::client::SessionState state,
               opentelemetry::nostd::string_view reason) noexcept override
  {}

  void OnConnecting(const opentelemetry::ext::http::client::SSLCertificate &) noexcept override {}

  void OnResponse(opentelemetry::ext::http::client::Response &response) noexcept override {}
};

// Sends an HTTP POST request to the given url, with the given body.
void send_request(opentelemetry::ext::http::client::curl::SessionManager &client,
                  const std::string &url,
                  const std::string &body)
{
  static opentelemetry::trace::propagation::HttpTraceContext<std::map<std::string, std::string>>
      format;
  static std::unique_ptr<opentelemetry::ext::http::client::EventHandler> handler(
      new NoopEventHandler());

  auto request_span = get_tracer()->StartSpan(__func__);
  opentelemetry::trace::Scope scope(request_span);

  Uri uri{url};

  auto session = client.CreateSession(uri.host, uri.port);
  auto request = session->CreateRequest();

  request->SetMethod(opentelemetry::ext::http::client::Method::Post);
  request->SetUri(uri.path);
  opentelemetry::ext::http::client::Body b = {body.c_str(), body.c_str() + body.size()};
  request->SetBody(b);
  request->AddHeader("Content-Type", "application/json");
  request->AddHeader("Content-Length", std::to_string(body.size()));

  std::map<std::string, std::string> headers;
  format.Inject(Setter, headers, opentelemetry::context::RuntimeContext::GetCurrent());

  for (auto const &hdr : headers)
  {
    request->AddHeader(hdr.first, hdr.second);
  }

  session->SendRequest(*handler);
  session->FinishSession();
}

// This application receives requests from the W3C test service. Each request has a JSON body which
// consists of an array of objects, each containing an URL to which to post to, and arguments which
// need to be used as body when posting to the given URL.
int main()
{
  static opentelemetry::trace::propagation::HttpTraceContext<std::map<std::string, std::string>>
      format;

  initTracer();

  constexpr char host[]   = "localhost";
  constexpr uint16_t port = 30000;

  auto root_span = get_tracer()->StartSpan(__func__);
  opentelemetry::trace::Scope scope(root_span);

  testing::HttpServer server(host, port);
  opentelemetry::ext::http::client::curl::SessionManager client;

  testing::HttpRequestCallback test_cb{
      [&](testing::HttpRequest const &req, testing::HttpResponse &resp) {
        auto body = nlohmann::json::parse(req.content);

        std::cout << "Received request with body :\n" << req.content << "\n";

        for (auto &part : body)
        {
          auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
          auto ctx         = format.Extract(Getter, req.headers, current_ctx);
          auto token       = opentelemetry::context::RuntimeContext::Attach(ctx);

          auto url       = part["url"].get<std::string>();
          auto arguments = part["arguments"].dump();

          std::cout << "  Sending request to " << url << "\n";

          send_request(client, url, arguments);
        }

        std::cout << "\n";

        resp.code = 200;
        return 0;
      }};
  server["/test"] = test_cb;

  // Start server
  server.start();

  std::cout << "Listening at http://" << host << ":" << port << "/test\n";

  // Wait for console input
  std::cin.get();

  // Stop server
  server.stop();
}

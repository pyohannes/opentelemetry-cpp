#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"

#define HAVE_HTTP_DEBUG 1
#include "opentelemetry/ext/http/server/http_server.h"
#include "opentelemetry/ext/http/client/curl/http_client_curl.h"

#include "nlohmann/json.hpp"

// Using an exporter that simply dumps span data to stdout.
#include "opentelemetry/exporters/ostream/span_exporter.h"

#include <algorithm>
/*
namespace
{
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
  return provider->GetTracer("foo_library");
}
}  // namespace
*/

class CustomEventHandler : public opentelemetry::ext::http::client::EventHandler
{
    public:
	  virtual void OnResponse(opentelemetry::ext::http::client::Response &response) noexcept override{
	          auto body = response.GetBody();
		  std::string b(body.at(0), body.size());
		  std::cout << "### OnResponse with code " << response.GetStatusCode() << " and body " << b << "\n";}
	    virtual void OnEvent(opentelemetry::ext::http::client::SessionState state,
		                           opentelemetry::nostd::string_view reason) noexcept override
		  {
		 std::cout << "### Callback state " << (int)state << " with reason " << reason << "\n";}
	      virtual void OnConnecting(const opentelemetry::ext::http::client::SSLCertificate &) noexcept {
		  std::cout << "### OnConnecting\n";}
	        virtual ~CustomEventHandler() = default;
};
int main()
{
  /*
  initTracer();

  auto root_span = get_tracer()->StartSpan(__func__);
  opentelemetry::trace::Scope scope(root_span);
  */

  testing::HttpServer server("localhost", 30000);
  opentelemetry::ext::http::client::curl::SessionManager client;

  testing::HttpRequestCallback test_cb{[&](testing::HttpRequest const &req, testing::HttpResponse &resp) {
      auto body = nlohmann::json::parse(req.content);

      for (auto& part : body) {
	  auto url = part["url"].get<std::string>();
	  auto arguments = part["arguments"].dump();

	  size_t host_end = url.substr(7, std::string::npos).find(":");
	  size_t port_end = url.substr(host_end + 1, std::string::npos).find("/");

	  auto host = url.substr(0, host_end + 7);
	  std::cout << "### Finish session with host " << host << " host_end " << host_end << " port _end " << port_end << "\n";
	  std::cout << "### arguments: " << arguments <<"\n";
	  auto port = std::stoi(url.substr(7 + host_end + 1, port_end));
	  auto path = url.substr(host_end + port_end + 2, std::string::npos);

	  auto session = client.CreateSession(host, port);
	  auto request = session->CreateRequest();

	  request->SetMethod(opentelemetry::ext::http::client::Method::Post);
	  request->SetUri(path);
	  arguments = "hi";
	  opentelemetry::ext::http::client::Body b = {'a', 'b', 'c', 'd', 0};
	  request->SetBody(b);
	  request->AddHeader("Content-Type", "application/json");
	  request->AddHeader("Content-Length", std::to_string(arguments.size()));

	  std::unique_ptr<opentelemetry::ext::http::client::EventHandler> handler(new CustomEventHandler());
	  std::cout << "### Send Request\n";
	  session->SendRequest(*handler);
	  std::cout << "### Finish session with host " << host << " and port " << port << " and url " << path << " host_end " << host_end << " port _end " << port_end << "\n";
	  session->FinishSession();
	  std::cout << "### Done\n";
      }

      resp.code = 200;
      return 0; 
  }};
  server["/test"] = test_cb;

  // Start server
  server.start();
  
  // Wait for console input
  std::cout << "Presss <ENTER> to stop...\n";
  std::cin.get();
  
  // Stop server
  server.stop();
  std::cout << "Server stopped.\n";
}

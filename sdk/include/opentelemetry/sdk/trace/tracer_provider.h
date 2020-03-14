#pragma once

#include <map>
#include <memory>
#include <string>

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace trace
{
class TracerProvider : public opentelemetry::trace::TracerProvider
{
public:
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> GetTracer(nostd::string_view library_name,
                          nostd::string_view library_version = "") override;

private:
  std::map<std::pair<std::string, std::string>, nostd::shared_ptr<Tracer>> tracers_;
};
}  // namespace trace
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE

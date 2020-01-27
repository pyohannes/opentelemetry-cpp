#include "opentelemetry/trace/global_provider.h"

#include <gtest/gtest.h>

using opentelemetry::trace::Provider;
using opentelemetry::trace::Tracer;

class TestProvider : public opentelemetry::trace::TracerProvider
{
  Tracer *const GetTracer(opentelemetry::nostd::string_view library_name,
                          opentelemetry::nostd::string_view library_version) override
  {
    return nullptr;
  }
};

TEST(Provider, GetTracerProviderDefault)
{
  auto tf = Provider::GetTracerProvider();
  ASSERT_NE(tf, nullptr);
}

TEST(Provider, SetTracerProvider)
{
  auto tf = new TestProvider();
  Provider::SetTracerProvider(tf);
  ASSERT_EQ(Provider::GetTracerProvider(), tf);
  Provider::SetTracerProvider(nullptr);
}

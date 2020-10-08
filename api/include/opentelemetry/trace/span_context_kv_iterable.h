#pragma once

#include "opentelemetry/common/attribute_value.h"
#include "opentelemetry/nostd/function_ref.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace trace
{
/**
 * Supports internal iteration over a collection of SpanContext/key-value pairs.
 */
class SpanContextKeyValueIterable
{
public:
  virtual ~SpanContextKeyValueIterable() = default;

  /**
   * Iterate over SpanContext/key-value pairs
   * @param callback a callback to invoke for each key-value for each SpanContext.
   * If the callback returns false, the iteration is aborted.
   * @return true if every SpanContext/key-value pair was iterated over
   */
  virtual bool ForEachKeyValue(
      nostd::function_ref<bool(SpanContext, nostd::string_view, common::AttributeValue)> callback)
      const noexcept = 0;

  /**
   * @return the number of key-value pairs
   */
  virtual size_t size() const noexcept = 0;
};
}  // namespace trace
OPENTELEMETRY_END_NAMESPACE

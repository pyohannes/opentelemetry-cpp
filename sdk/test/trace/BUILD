load("//bazel:otel_cc_benchmark.bzl", "otel_cc_benchmark")

cc_test(
    name = "noop_test",
    srcs = [
        "noop_test.cc",
    ],
    deps = [
        "//api",
        "@com_google_googletest//:gtest_main",
    ],
)

otel_cc_benchmark(
    name = "span_id_benchmark",
    srcs = ["span_id_benchmark.cc"],
    deps = ["//api"],
)

cc_test(
    name = "span_id_test",
    srcs = [
        "span_id_test.cc",
    ],
    deps = [
        "//api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trace_flags_test",
    srcs = [
        "trace_flags_test.cc",
    ],
    deps = [
        "//api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trace_id_test",
    srcs = [
        "trace_id_test.cc",
    deps = [
        "//api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
      name = "tracer_provider",
      srcs = [
          "tracer_provider_test.cc",
      ],
    deps = [
        "//sdk",
        "@com_google_googletest//:gtest_main",
    ],
)
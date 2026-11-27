// The kernels of core/fn/Approx.hpp, included by ApproxCommand.cpp: name, meaning, form, interval
// of x, degree of p, largest relative error, the function (CORE-MATH), and optionally p's target.
// Each interval is where Approx.hpp's argument reduction leaves x; the bound keeps every function
// built on the kernel well inside its own bound (1e-10 for double, one ulp for float).
{"ExpRest", "e^x - 1", Form::Rest, -0.34657359027997264, 0.34657359027997264, 14, 1e-13,
 [](double x) { return exact::expm1(x); }},
{"LogOdd", "log((1 + s) / (1 - s)) = 2 atanh(s)", Form::Odd, 0.0, 0.17157287525380990, 10, 1e-13,
 [](double s) { return 2 * exact::atanh(s); }},
{"SinOdd", "sin(x)", Form::Odd, 0.0, 0.78539816339744831, 10, 1e-13,
 [](double x) { return exact::sin(x); }},
{"CosOne", "cos(x)", Form::Cosine, 0.0, 0.78539816339744831, 10, 1e-13,
 [](double x) { return exact::cos(x); },
 [](double t) { const double x = std::sqrt(t); if (x < 1e-9) return -0.5; const double h = exact::sin(0.5 * x); return -2 * h * h / t; }},
{"AtanOdd", "atan(u)", Form::Odd, 0.0, 0.41421356237309515, 20, 1e-13,
 [](double u) { return exact::atan(u); }},
{"AsinOdd", "asin(x)", Form::Odd, 0.0, 0.5, 20, 1e-13,
 [](double x) { return exact::asin(x); }},
{"CbrtStart", "cbrt(x), the start of Halley's iteration", Form::Plain, 1.0, 2.0, 8, 1e-4,
 [](double x) { return exact::cbrt(x); }},

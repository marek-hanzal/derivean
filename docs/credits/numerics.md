# Credits: elementary functions

Part of [Credits](../credits.md); the entry format is explained there. The correctly rounded library itself, CORE-MATH, is under [libraries](libraries.md).

### Trefethen 2019: Chebyshev interpolation
Trefethen, L. N. (2019). *Approximation Theory and Approximation Practice*, extended edition. SIAM, Philadelphia. ISBN 978-1-611975-93-2. (First edition 2013.)
- **Model:** a smooth function interpolated at the Chebyshev points of its interval comes within a small factor of the best polynomial of the same degree; the coefficients of the Chebyshev series from the values at those points, and the three-term recurrence that turns the series into powers of the variable.
- **Used in:** `@simulation/tool/ApproxCommand.cpp:86` (`terra approx`), whose output is `@simulation/src/core/constant/ApproxCoefficients.hpp`.
- **Checked by:** `terra approx` itself, which measures each kernel against CORE-MATH and fails over its bound; `@simulation/test/Approx.cpp`.
- **Approximations:** interpolation, not the minimax (Remez) polynomial: a kernel may need one degree more than the best polynomial would.

### Cody and Waite 1980: argument reduction with split constants
Cody, W. J., Jr., Waite, W. (1980). *Software Manual for the Elementary Functions*. Prentice-Hall, Englewood Cliffs, NJ. ISBN 978-0-13-822064-8 (unverified: ISBN from booksellers).
- **Model:** x − k·C reduced with C split into a leading part and corrections, so that the reduced argument keeps its relative accuracy; here with fused multiply-adds, ln 2 in two parts and π/2 in three.
- **Used in:** `@simulation/src/core/fn/Approx.hpp:102` (exp), `:120` (log; log2 and pow take the same split of x), `@simulation/src/core/fn/ApproxAngle.hpp:32` (sine and cosine).
- **Checked by:** `@simulation/test/Approx.cpp` (runs of neighbouring floats at every reduction boundary), `terra approx-check` (every float).

### Muller 2016: elementary functions
Muller, J.-M. (2016). *Elementary Functions: Algorithms and Implementation*, 3rd edition. Birkhäuser, Boston. doi:10.1007/978-1-4899-7983-4
- **Model:** the reductions behind each function: e^x as 2^k · e^r; log m through s = (m − 1)/(m + 1) and the odd series of 2 atanh s; x^y as 2^(y log2 x); tanh from e^(2x) − 1; atan above tan π/8 through atan((t − 1)/(t + 1)); acos through asin √((1 − x)/2); the cube root from a polynomial start and Halley's iteration.
- **Used in:** `@simulation/src/core/fn/Approx.hpp`, `@simulation/src/core/fn/ApproxAngle.hpp`.
- **Checked by:** `@simulation/test/Approx.cpp`, `terra approx-check`.

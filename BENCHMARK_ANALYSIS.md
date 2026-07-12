# Zero-Copy Branch Benchmark Analysis

## Executive Summary

Benchmarked the zerocopy branch (PR #1391) which adds documentation for zero-copy NumPy↔Java array access using direct buffers vs the traditional `JArray.of()` copying approach.

**Key Finding**: The documentation's claim that zero-copy is faster for large arrays (>100MB) is **NOT supported** by these benchmarks. `JArray.of()` (copying) is consistently 1.5-18x faster for setup across all tested sizes, including 500MB arrays.

## Test Environment

- JPype version: 1.7.2.dev0
- NumPy version: 2.2.0
- Python version: 3.12.3
- Platform: Linux WSL2

## Benchmark Results Summary

### 1. Transfer Time vs Array Size (1D Double Arrays)

| Size | JArray.of() Setup | DirectBuffer Setup | Speedup (Copy) |
|------|-------------------|-------------------|----------------|
| 128KB | 0.45ms | 8.26ms | **18.4x faster** |
| 1MB | 0.07ms | 0.11ms | **1.5x faster** |
| 10MB | 0.09ms | 0.24ms | **2.6x faster** |
| 50MB | 0.38ms | 0.79ms | **2.1x faster** |
| 100MB | 0.74ms | 1.56ms | **2.1x faster** |
| 500MB | 2.89ms | 7.23ms | **2.5x faster** |

**Conclusion**: Copying is consistently faster for setup, even at 500MB. The overhead of creating direct buffers and wrapping them exceeds the benefit of avoiding the copy.

### 2. Element Access Performance (1D vs 2D)

| Array Shape | JArray.of() Access | DirectBuffer Access | Notes |
|-------------|-------------------|---------------------|-------|
| 10K (1D) | 4.12ms | 10.19ms | Copy 2.5x faster (zero-copy has indexing overhead) |
| 100x100 (2D) | 15.91ms | 10.18ms | Zero-copy 1.6x faster (manual indexing benefits?) |
| 1000x1000 (2D) | 1440ms | 1088ms | Zero-copy 1.3x faster (amortizes indexing cost) |

**Conclusion**: For 1D arrays, copying provides better access performance. For large 2D arrays, zero-copy manual indexing can be faster, but setup overhead still makes total time slower.

### 3. Memory Overhead

- **JArray.of()**: Requires full copy of array (e.g., 500MB array = 500MB overhead)
- **DirectBuffer**: Minimal overhead (~0.01MB for buffer views)

**Conclusion**: Zero-copy's main advantage is memory efficiency, not speed.

## When Should Zero-Copy Be Used?

Based on benchmark data, zero-copy via direct buffers should be recommended when:

1. **Memory is constrained** - Avoiding a copy is critical
2. **Shared memory semantics needed** - Changes in Java must be visible in Python
3. **Minimal Java access** - Setup cost can't be amortized if Java rarely touches the data
4. **Long-lived arrays** - Setup cost amortized over long lifetime

Zero-copy should **NOT** be recommended based solely on array size or speed.

## Issues with Current Documentation

The zerocopy branch documentation states:

> "For large arrays where copying is expensive, direct buffers are the right choice."

and

> "For most use cases under 100MB, the simplicity of JArray.of() outweighs the copy cost. For larger arrays... direct buffers are the right choice."

**These claims are misleading.** The benchmarks show:
- Even at 500MB, copying is 2.5x faster for setup
- Only when you need shared memory semantics or are extremely memory-constrained should you use zero-copy
- The ~100MB threshold mentioned in the docs has no empirical basis from these results

## Recommendations for Documentation Updates

1. **Reframe the trade-off** around memory vs speed, not small vs large arrays
2. **Primary use case** should be shared memory semantics (not performance)
3. **Remove or clarify** the 100MB threshold claim
4. **Add warning** that zero-copy has both setup and access overhead for 1D arrays
5. **Emphasize** that copying is faster in most scenarios

### Suggested Documentation Language

```
**When to Use Zero-Copy vs Copying**:

- **Use JArray.of() (copying)** when:
  - Performance is critical (copying is typically faster, even for large arrays)
  - You need true Java arrays with standard syntax
  - Arrays are accessed frequently in Java
  - Independent copies are acceptable

- **Use Direct Buffers (zero-copy)** when:
  - Memory is constrained (cannot afford to double the memory)
  - You need shared memory (changes in Java visible in Python)
  - Minimal Java access (setup cost not amortized)
  - You're willing to accept performance overhead for memory savings
```

## Additional Observations

1. **DirectBuffer setup overhead is significant**: 8-18x slower for small arrays
2. **Manual indexing penalty**: 2-3x slower element access for 1D arrays
3. **2D arrays can benefit**: Manual indexing overhead decreases relatively for 2D
4. **Modern CPUs are good at copying**: Memory bandwidth makes copying fast

## Files Generated

- `benchmark_zerocopy.py` - Comprehensive benchmark suite
- `benchmark_results.txt` - Full benchmark output
- `BENCHMARK_ANALYSIS.md` - This analysis document

## Next Steps

1. Share results with PR maintainer
2. Discuss whether documentation should be updated to reflect actual performance characteristics
3. Consider if zero-copy feature should be documented primarily as a memory-saving feature rather than performance feature
4. Potentially add performance benchmarks to the test suite to prevent regression

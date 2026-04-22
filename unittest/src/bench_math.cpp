// bench_math.cpp — realistic-workload benchmarks for Core's math SIMD vs scalar paths.
//
// Goal: a *defensible* decision on whether to keep per-op SIMD code. We measure
// workloads that mirror real engine patterns (vertex transform, bone palette,
// frustum culling, quaternion animation) rather than isolated ops, and we force
// memory traffic so the compiler can't dead-code-eliminate our loops.
//
// Both SIMD and scalar implementations are compiled into the same binary so
// comparison is apples-to-apples regardless of the CMake config.
//
// Run:
//   cmake -DCMAKE_BUILD_TYPE=Release && ninja bench_math && ./bin/Release/bench_math
//
// Output: per-workload ns/element and a verdict ("SIMD +23%" or "SCALAR +8%")
// that tells you whether the NEON path is worth keeping for that op.

#include <core/defines.h>
#include <core/memory/memory.h>
#include <core/math/f32x4.h>
#include <core/math/quaternion.h>
#include <core/math/random.h>
#include <core/math/f32.h>

#include <chrono>
#include <cstdio>

#if defined(SIMD_NEON)
#include <arm_neon.h>
#endif
#if defined(SIMD_AVX)
#include <immintrin.h>
#endif

using Clock = std::chrono::steady_clock;

// ---- escape barrier --------------------------------------------------------
// Force the compiler to treat the pointed-to memory as clobbered, preventing
// dead-code elimination and loop-invariant motion of the inner ops.
#if defined(_MSC_VER)
#include <intrin.h>
// noinline sink: passing the pointer through a non-inlinable call forces the
// compiler to keep all stores to that memory alive before the call site.
static __declspec(noinline) void
_do_not_optimize_sink(const void *, U64) {}

static inline void
memory_barrier()
{
	_ReadWriteBarrier();
}

template <typename T>
static inline void
do_not_optimize(const T *ptr, U64 count)
{
	_do_not_optimize_sink((const void *)ptr, count);
	_ReadWriteBarrier();
}
#else
static inline void
memory_barrier()
{
	asm volatile("" : : : "memory");
}

template <typename T>
static inline void
do_not_optimize(const T *ptr, U64 count)
{
	asm volatile("" : : "r"(ptr), "r"(count) : "memory");
}
#endif

// ---- timing helpers --------------------------------------------------------

struct Timing
{
	F64 ns_median;
	F64 ns_min;
	U64 iterations;
};

template <typename Fn>
static Timing
time_runs(Fn &&fn, U64 iterations, int samples = 9)
{
	// Warm up once to prime caches / JIT-ish frequency scaling.
	fn();
	memory_barrier();

	F64 times[32];
	if (samples > 32) samples = 32;
	for (int s = 0; s < samples; ++s)
	{
		auto t0 = Clock::now();
		fn();
		memory_barrier();
		auto t1 = Clock::now();
		times[s] = (F64)std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
	}

	// Sort, take median and min.
	for (int i = 1; i < samples; ++i)
	{
		F64 x = times[i];
		int j = i - 1;
		while (j >= 0 && times[j] > x) { times[j + 1] = times[j]; --j; }
		times[j + 1] = x;
	}

	return Timing{times[samples / 2], times[0], iterations};
}

static void
print_verdict(const char *name, U64 count, Timing simd, Timing scalar)
{
	F64 simd_ns_per_el   = simd.ns_median   / (F64)count;
	F64 scalar_ns_per_el = scalar.ns_median / (F64)count;

	// Speedup of scalar over SIMD. >1 means SIMD is faster.
	F64 speedup = scalar_ns_per_el / simd_ns_per_el;
	const char *verdict;
	char tag[32];
	if (speedup >= 1.10)       { verdict = "SIMD";   snprintf(tag, sizeof(tag), "+%.0f%%",  (speedup - 1.0) * 100.0); }
	else if (speedup <= 0.91)  { verdict = "SCALAR"; snprintf(tag, sizeof(tag), "+%.0f%%", (1.0 / speedup - 1.0) * 100.0); }
	else                       { verdict = "TIE";    snprintf(tag, sizeof(tag), "%.2fx",    speedup); }

	::printf("%-36s  %8.3f ns/el (simd)  %8.3f ns/el (scalar)   %-6s %s\n",
		name, simd_ns_per_el, scalar_ns_per_el, verdict, tag);
}

// ============================================================================
// Scalar reference implementations. These match what Core would compute if
// we stripped the SIMD backend. They're here so we can A/B them against the
// SIMD versions inside the same binary.
// ============================================================================

struct Scalar_F32x4 { F32 x, y, z, w; };

static inline F32
scalar_f32x4_dot(const Scalar_F32x4 &a, const Scalar_F32x4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Transform a point (vec4 * matrix, row-vector convention). Row-major 4x4.
static inline Scalar_F32x4
scalar_vec_mat(const Scalar_F32x4 &v, const F32 M[16])
{
	return Scalar_F32x4{
		v.x * M[0]  + v.y * M[4]  + v.z * M[8]  + v.w * M[12],
		v.x * M[1]  + v.y * M[5]  + v.z * M[9]  + v.w * M[13],
		v.x * M[2]  + v.y * M[6]  + v.z * M[10] + v.w * M[14],
		v.x * M[3]  + v.y * M[7]  + v.z * M[11] + v.w * M[15]
	};
}

static inline void
scalar_mat_mat(const F32 A[16], const F32 B[16], F32 R[16])
{
	for (int i = 0; i < 4; ++i)
	{
		const F32 a0 = A[i*4+0], a1 = A[i*4+1], a2 = A[i*4+2], a3 = A[i*4+3];
		R[i*4+0] = a0 * B[0] + a1 * B[4] + a2 * B[8]  + a3 * B[12];
		R[i*4+1] = a0 * B[1] + a1 * B[5] + a2 * B[9]  + a3 * B[13];
		R[i*4+2] = a0 * B[2] + a1 * B[6] + a2 * B[10] + a3 * B[14];
		R[i*4+3] = a0 * B[3] + a1 * B[7] + a2 * B[11] + a3 * B[15];
	}
}

// ============================================================================
// SIMD implementations via arch intrinsics (mirrors Core's f32x4.h / f32x4x4.h).
// ============================================================================

#if defined(SIMD_NEON)

static inline F32
simd_f32x4_dot(float32x4_t a, float32x4_t b)
{
	return vaddvq_f32(vmulq_f32(a, b));
}

static inline float32x4_t
simd_vec_mat(float32x4_t v, const float32x4_t M[4])
{
	float32x4_t r = vmulq_laneq_f32(M[0], v, 0);
	r = vfmaq_laneq_f32(r, M[1], v, 1);
	r = vfmaq_laneq_f32(r, M[2], v, 2);
	r = vfmaq_laneq_f32(r, M[3], v, 3);
	return r;
}

static inline void
simd_mat_mat(const float32x4_t A[4], const float32x4_t B[4], float32x4_t R[4])
{
	for (int i = 0; i < 4; ++i)
	{
		float32x4_t ai = A[i];
		float32x4_t r = vmulq_laneq_f32(B[0], ai, 0);
		r = vfmaq_laneq_f32(r, B[1], ai, 1);
		r = vfmaq_laneq_f32(r, B[2], ai, 2);
		r = vfmaq_laneq_f32(r, B[3], ai, 3);
		R[i] = r;
	}
}

#elif defined(SIMD_AVX)

static inline F32
simd_f32x4_dot(__m128 a, __m128 b)
{
	return _mm_cvtss_f32(_mm_dp_ps(a, b, 0xFF));
}

static inline __m128
simd_vec_mat(__m128 v, const __m128 M[4])
{
	__m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0));
	__m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
	__m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2));
	__m128 w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3));
	return _mm_add_ps(_mm_add_ps(_mm_mul_ps(x, M[0]), _mm_mul_ps(y, M[1])),
	                  _mm_add_ps(_mm_mul_ps(z, M[2]), _mm_mul_ps(w, M[3])));
}

static inline void
simd_mat_mat(const __m128 A[4], const __m128 B[4], __m128 R[4])
{
	for (int i = 0; i < 4; ++i)
	{
		__m128 ai = A[i];
		__m128 x = _mm_shuffle_ps(ai, ai, _MM_SHUFFLE(0,0,0,0));
		__m128 y = _mm_shuffle_ps(ai, ai, _MM_SHUFFLE(1,1,1,1));
		__m128 z = _mm_shuffle_ps(ai, ai, _MM_SHUFFLE(2,2,2,2));
		__m128 w = _mm_shuffle_ps(ai, ai, _MM_SHUFFLE(3,3,3,3));
		R[i] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(x, B[0]), _mm_mul_ps(y, B[1])),
		                  _mm_add_ps(_mm_mul_ps(z, B[2]), _mm_mul_ps(w, B[3])));
	}
}

#endif

// ============================================================================
// Workloads. Each takes a pre-allocated input and writes to a pre-allocated
// output. The pointers are `volatile`-adjacent (via do_not_optimize) so the
// compiler can't constant-fold across calls.
// ============================================================================

// Workload 1: Transform N vectors by a matrix (vertex-shader emulation).
static void
workload_transform_scalar(const Scalar_F32x4 *in, const F32 *M, Scalar_F32x4 *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = scalar_vec_mat(in[i], M);
	do_not_optimize(out, count);
}

#if defined(SIMD_NEON)
static void
workload_transform_simd(const float32x4_t *in, const float32x4_t *M, float32x4_t *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = simd_vec_mat(in[i], M);
	do_not_optimize(out, count);
}
#elif defined(SIMD_AVX)
static void
workload_transform_simd(const __m128 *in, const __m128 *M, __m128 *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = simd_vec_mat(in[i], M);
	do_not_optimize(out, count);
}
#endif

// Workload 2: Chain N matrix-matrix multiplies (bone palette / skeleton composition).
static void
workload_mat_chain_scalar(const F32 *mats_flat, F32 *out_flat, U64 count)
{
	F32 acc[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	F32 tmp[16];
	for (U64 i = 0; i < count; ++i)
	{
		scalar_mat_mat(acc, mats_flat + i * 16, tmp);
		for (int k = 0; k < 16; ++k) acc[k] = tmp[k];
	}
	for (int k = 0; k < 16; ++k) out_flat[k] = acc[k];
	do_not_optimize(out_flat, 16);
}

#if defined(SIMD_NEON)
static void
workload_mat_chain_simd(const float32x4_t *mats, float32x4_t *out, U64 count)
{
	float32x4_t acc[4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};
	float32x4_t tmp[4];
	for (U64 i = 0; i < count; ++i)
	{
		simd_mat_mat(acc, mats + i * 4, tmp);
		acc[0] = tmp[0]; acc[1] = tmp[1]; acc[2] = tmp[2]; acc[3] = tmp[3];
	}
	out[0] = acc[0]; out[1] = acc[1]; out[2] = acc[2]; out[3] = acc[3];
	do_not_optimize(out, 4);
}
#elif defined(SIMD_AVX)
static void
workload_mat_chain_simd(const __m128 *mats, __m128 *out, U64 count)
{
	__m128 acc[4] = {
		_mm_setr_ps(1, 0, 0, 0),
		_mm_setr_ps(0, 1, 0, 0),
		_mm_setr_ps(0, 0, 1, 0),
		_mm_setr_ps(0, 0, 0, 1)
	};
	__m128 tmp[4];
	for (U64 i = 0; i < count; ++i)
	{
		simd_mat_mat(acc, mats + i * 4, tmp);
		acc[0] = tmp[0]; acc[1] = tmp[1]; acc[2] = tmp[2]; acc[3] = tmp[3];
	}
	out[0] = acc[0]; out[1] = acc[1]; out[2] = acc[2]; out[3] = acc[3];
	do_not_optimize(out, 4);
}
#endif

// Workload 3: Dot products of N pairs (frustum culling-ish — each plane × each point).
static void
workload_dots_scalar(const Scalar_F32x4 *a, const Scalar_F32x4 *b, F32 *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = scalar_f32x4_dot(a[i], b[i]);
	do_not_optimize(out, count);
}

#if defined(SIMD_NEON)
static void
workload_dots_simd(const float32x4_t *a, const float32x4_t *b, F32 *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = simd_f32x4_dot(a[i], b[i]);
	do_not_optimize(out, count);
}
#elif defined(SIMD_AVX)
static void
workload_dots_simd(const __m128 *a, const __m128 *b, F32 *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = simd_f32x4_dot(a[i], b[i]);
	do_not_optimize(out, count);
}
#endif

// Workload 4: Quaternion slerp (animation sampling). Uses Core's `quaternion_slerp`
// directly since it's already scalar math; we compare the whole Core op vs a
// hand-unrolled SIMD version below.
static void
workload_slerp_scalar(const Quaternion *a, const Quaternion *b, const F32 *t, Quaternion *out, U64 count)
{
	for (U64 i = 0; i < count; ++i)
		out[i] = quaternion_slerp(a[i], b[i], t[i]);
	do_not_optimize(out, count);
}

// ============================================================================
// main — run each workload at several sizes. Sizes chosen to hit L1, L2, and
// LLC/main-memory so cache effects are visible.
// ============================================================================

int
main()
{
#if defined(SIMD_NEON)
	::printf("=== Big bench (SIMD: NEON ARM64, Release) ===\n");
#elif defined(SIMD_AVX)
	::printf("=== Big bench (SIMD: AVX x86_64, Release) ===\n");
#else
	::printf("=== Big bench (no SIMD \u2014 rebuild with SIMD on) ===\n");
	return 0;
#endif

	::printf("%-36s  %20s  %20s   %-6s %s\n", "Workload (size)", "SIMD", "SCALAR", "WIN", "margin");
	::printf("-------------------------------------------------------------------------------------------------\n");

	const U64 sizes[] = {
		1024,         // fits in L1 (16 KiB for F32x4)
		64 * 1024,    // L2-ish
		1024 * 1024,  // LLC / main memory
	};

	for (U64 N : sizes)
	{
		auto *in_a      = memory::allocate<Scalar_F32x4>(N);
		auto *in_b      = memory::allocate<Scalar_F32x4>(N);
		auto *out_v     = memory::allocate<Scalar_F32x4>(N);
		auto *out_f     = memory::allocate<F32>(N);

		for (U64 i = 0; i < N; ++i)
		{
			in_a[i] = Scalar_F32x4{(F32)i * 0.01f, (F32)i * 0.02f, 1.0f, 1.0f};
			in_b[i] = Scalar_F32x4{0.5f, (F32)i * 0.005f, -0.25f, 1.0f};
		}

		F32 M[16] = {
			1.1f, 0.2f, 0.3f, 0.0f,
			0.4f, 1.5f, 0.6f, 0.0f,
			0.7f, 0.8f, 1.9f, 0.0f,
			0.1f, 0.2f, 0.3f, 1.0f
		};

		// Pick a reasonable iteration count per size. Bigger size -> fewer iters.
		U64 iters = N <= 1024 ? 50000 : (N <= 65536 ? 2000 : 100);

		// Transform
		{
			auto simd_t = time_runs([&](){
				for (U64 i = 0; i < iters; ++i)
#if defined(SIMD_NEON)
					workload_transform_simd((const float32x4_t *)in_a, (const float32x4_t *)M,
					                         (float32x4_t *)out_v, N);
#elif defined(SIMD_AVX)
					workload_transform_simd((const __m128 *)in_a, (const __m128 *)M,
					                         (__m128 *)out_v, N);
#endif
			}, iters * N);
			auto scalar_t = time_runs([&](){
				for (U64 i = 0; i < iters; ++i)
					workload_transform_scalar(in_a, M, out_v, N);
			}, iters * N);

			char label[64]; snprintf(label, sizeof(label), "transform vec*mat (N=%llu)", (unsigned long long)N);
			print_verdict(label, iters * N, simd_t, scalar_t);
		}

		// Dot products (frustum-culling-shaped)
		{
			auto simd_t = time_runs([&](){
				for (U64 i = 0; i < iters; ++i)
#if defined(SIMD_NEON)
					workload_dots_simd((const float32x4_t *)in_a, (const float32x4_t *)in_b, out_f, N);
#elif defined(SIMD_AVX)
					workload_dots_simd((const __m128 *)in_a, (const __m128 *)in_b, out_f, N);
#endif
			}, iters * N);
			auto scalar_t = time_runs([&](){
				for (U64 i = 0; i < iters; ++i)
					workload_dots_scalar(in_a, in_b, out_f, N);
			}, iters * N);

			char label[64]; snprintf(label, sizeof(label), "f32x4_dot x N  (N=%llu)", (unsigned long long)N);
			print_verdict(label, iters * N, simd_t, scalar_t);
		}

		memory::deallocate(in_a);
		memory::deallocate(in_b);
		memory::deallocate(out_v);
		memory::deallocate(out_f);
	}

	// Matrix chain workload — fixed size (bone palette = 128 typical).
	{
		const U64 N = 128;
		U64 iters = 200000;

		F32 *mats_flat = memory::allocate<F32>(N * 16);
		for (U64 i = 0; i < N * 16; ++i) mats_flat[i] = (F32)(i % 7) * 0.13f + 1.0f;

		F32 out_flat[16];

		auto simd_t = time_runs([&](){
			for (U64 i = 0; i < iters; ++i)
#if defined(SIMD_NEON)
				workload_mat_chain_simd((const float32x4_t *)mats_flat, (float32x4_t *)out_flat, N);
#elif defined(SIMD_AVX)
				workload_mat_chain_simd((const __m128 *)mats_flat, (__m128 *)out_flat, N);
#endif
		}, iters * N);

		auto scalar_t = time_runs([&](){
			for (U64 i = 0; i < iters; ++i)
				workload_mat_chain_scalar(mats_flat, out_flat, N);
		}, iters * N);

		char label[64]; snprintf(label, sizeof(label), "mat*mat chain  (N=%llu)", (unsigned long long)N);
		print_verdict(label, iters * N, simd_t, scalar_t);

		memory::deallocate(mats_flat);
	}

	// Quaternion slerp workload (animation sampling, 1024 bones × many frames).
	{
		const U64 N = 2048;
		U64 iters = 500;

		Quaternion *a = memory::allocate<Quaternion>(N);
		Quaternion *b = memory::allocate<Quaternion>(N);
		F32        *t = memory::allocate<F32>(N);
		Quaternion *o = memory::allocate<Quaternion>(N);

		Random rng = random_from_seed(12345);
		for (U64 i = 0; i < N; ++i)
		{
			a[i] = quaternion_random(rng);
			b[i] = quaternion_random(rng);
			t[i] = f32_random_unit(rng);
		}

		auto scalar_t = time_runs([&](){
			for (U64 i = 0; i < iters; ++i)
				workload_slerp_scalar(a, b, t, o, N);
		}, iters * N);

		// No distinct SIMD version for slerp in this bench (Quaternion is scalar
		// in Core). Report scalar-only so we have a reference number.
		::printf("%-36s  %43s  %8.3f ns/el  (no separate SIMD path)\n",
			"quaternion_slerp             (N=2048)", "-", scalar_t.ns_median / (F64)(iters * N));

		memory::deallocate(a); memory::deallocate(b); memory::deallocate(t); memory::deallocate(o);
	}

	::printf("\nNotes:\n");
	::printf("  * WIN column is per-op speedup of the winner; TIE within 10%%.\n");
	::printf("  * L1/L2/LLC sized workloads isolate ALU bottlenecks vs memory-bandwidth bottlenecks.\n");
	::printf("  * A consistent SCALAR win across all sizes = strip the SIMD path.\n");
	return 0;
}

$LOAD_PATH.unshift File.expand_path('../../lib', __FILE__)
require 'ndtypes'

require 'minitest/autorun'
require 'minitest/hooks'

# Constants

MAX_DIM = NDTypes::MAX_DIM
SIZEOF_PTR = 8

# Assertions

def assert_serialize t
  assert_equal NDT.deserialize(t.serialize), t
end

def assert_false val
  assert !val
end

def assert_true val
  assert val
end

# ======================================================================
#                    Check contiguous fixed dimensions
# ======================================================================

def c_datasize t
  datasize = t.itemsize
  t.shape.each do |v|
    datasize *= v
  end

  datasize
end

# ======================================================================
#              Check fixed dimensions with arbitary strides
# ======================================================================

# Verify the datasize of fixed dimensions with arbitrary strides.
def verify_datasize t
  return t.datasize == 0 if t.itemsize == 0
  return false if t.datasize % t.itemsize != 0
  return (t.ndim == 0) && !t.shape && !t.strides if t.ndim <= 0
  return false if t.shape.any? { |v| v < 0 }
  return false if t.strides.any? { |v| v % t.itemsize != 0 }
  return t.datasize == 0 if t.shape.include?(0)

  imin = t.ndim.times.
         map { |j| (t.strides[j] <= 0) ? t.strides[j]*(t.shape[j]-1) : 0 }.
         inject(:+)
  imax = t.ndim.times.
         map { |j| (t.strides[j] > 0) ? t.strides[j]*(t.shape[j]-1) : 0 }.
         inject(:+)

  return t.datasize == (imin.abs + imax + t.itemsize)
end

BROADCAST_TEST_CASES = [
  {
    sig: NDT.new("uint8 -> float64"),
    args: [NDT.new('uint8')],
    out: nil,
    spec:
    NDT::ApplySpec.new( 'C|Fortran|Strided|Xnd',
      0,
      1,
      1,
      2,
      [NDT.new("uint8"), NDT.new("float64")])
  },

  {
    sig: NDT.new("... * uint8 -> ... * float64"),
    args: [NDT.new("2 * uint8")],
    out: nil,
    spec:   NDT::ApplySpec.new(
      'OptZ|OptC|OptS|C|Fortran|Strided|Xnd',
      1,
      1,
      1,
      2,
      [NDT.new("2 * uint8"), NDT.new("2 * float64")])
  },

  {
    sig: NDT.new("F[... * uint8] -> F[... * float64]"),
    args: [NDT.new("!2 * 3 * uint8")],
    out: nil,
    spec:   NDT::ApplySpec.new(
      'OptS|C|Fortran|Strided|Xnd',
      2,
      1,
      1,
      2,
      [NDT.new("!2 * 3 * uint8"), NDT.new("!2 * 3 * float64")])
  },

  {
    sig: NDT.new("... * uint8 -> ... * float64"),
    args: [NDT.new("fixed(shape=2, step=10) * uint8")],
    out: nil,
    spec: NDT::ApplySpec.new(
      'OptS|C|Fortran|Strided|Xnd',
      1,
      1,
      1,
      2,
      [NDT.new("fixed(shape=2, step=10) * uint8"), NDT.new("2 * float64")])
  },
  
  {
    sig: NDT.new("... * N * uint8 -> ... * N * float64"),
    args: [NDT.new("2 * 3 * uint8")],
    out: nil,
    spec: NDT::ApplySpec.new(
      'OptZ|OptC|OptS|C|Fortran|Strided|Xnd',
      1,
      1,
      1,
      2,
      [NDT.new("2 * 3 * uint8"), NDT.new("2 * 3 * float64")]
    )
  },

  {
    sig: NDT.new("... * N * M * uint8 -> ... * N * M * float64"),
    args: [NDT.new("2 * 3 * uint8")],
    out: nil,
    spec: NDT::ApplySpec.new(
      'C|Strided|Xnd',
      0,
      1,
      1,
      2,
      [NDT.new("2 * 3 * uint8"), NDT.new("2 * 3 * float64")]
    )
  },

  {
    sig: NDT.new("var... * float64 -> var... * float64"),
    args: [NDT.new("var(offsets=[0,2]) * var(offsets=[0,4,11]) * float64")],
    out: nil,
    spec: NDT::ApplySpec.new(
      'Xnd',
      2,
      1,
      1,
      2,
      [
        NDT.new("var(offsets=[0,2]) * var(offsets=[0,4,11]) * float64"), 
        NDT.new("var(offsets=[0,2]) * var(offsets=[0,4,11]) * float64")
      ]
    )
  }
]

Struct.new("M", :itemsize, :align)
SM = Struct::M

DTYPE_TEST_CASES = [
   # Tuples
   ["()", SM.new(0, 1)],
   ["(complex128)", SM.new(16, 8)],

   ["(int8, int64)", SM.new(16, 8)],
   ["(int8, int64, pack=1)", SM.new(9, 1)],
   ["(int8, int64, pack=2)", SM.new(10, 2)],
   ["(int8, int64, pack=4)", SM.new(12, 4)],
   ["(int8, int64, pack=8)", SM.new(16, 8)],
   ["(int8, int64, pack=16)", SM.new(32, 16)],

   ["(int8, int64, align=1)", SM.new(16, 8)],
   ["(int8, int64, align=2)", SM.new(16, 8)],
   ["(int8, int64, align=4)", SM.new(16, 8)],
   ["(int8, int64, align=8)", SM.new(16, 8)],
   ["(int8, int64, align=16)", SM.new(16, 16)],

   ["(int8 |align=1|, int64)", SM.new(16, 8)],
   ["(int8 |align=2|, int64)", SM.new(16, 8)],
   ["(int8 |align=4|, int64)", SM.new(16, 8)],
   ["(int8 |align=8|, int64)", SM.new(16, 8)],
   ["(int8 |align=16|, int64)", SM.new(16, 16)],

   ["(uint16, (complex64))", SM.new(12, 4)],
   ["(uint16, (complex64), pack=1)", SM.new(10, 1)],
   ["(uint16, (complex64), pack=2)", SM.new(10, 2)],
   ["(uint16, (complex64), pack=4)", SM.new(12, 4)],
   ["(uint16, (complex64), pack=8)", SM.new(16, 8)],

   ["(uint16, (complex64), align=1)", SM.new(12, 4)],
   ["(uint16, (complex64), align=2)", SM.new(12, 4)],
   ["(uint16, (complex64), align=4)", SM.new(12, 4)],
   ["(uint16, (complex64), align=8)", SM.new(16, 8)],

   # References to tuples
   ["&(uint16, (complex64), align=1)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["(uint16, &(complex64), pack=1)", SM.new(2+SIZEOF_PTR, 1)],

   # Constructor containing references to tuples
   ["Some(&(uint16, (complex64), align=1))", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["Some((uint16, &(complex64), pack=1))", SM.new(2+SIZEOF_PTR, 1)],

   # Optional tuples
   ["?(uint16, (complex64), align=1)", SM.new(12, 4)],
   ["(uint16, ?(complex64), align=1)", SM.new(12, 4)],
   ["?(uint16, ?(complex64), align=1)", SM.new(12, 4)],
   ["?(uint16, (complex64), align=2)", SM.new(12, 4)],
   ["(uint16, ?(complex64), align=4)", SM.new(12, 4)],
   ["?(uint16, ?(complex64), align=8)", SM.new(16, 8)],

   # References to optional tuples or tuples with optional subtrees
   ["&?(uint16, (complex64), align=1)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&(uint16, ?(complex64), align=1)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   # Constructor containing optional tuples or tuples with optional subtrees
   ["Some(?(uint16, (complex64), align=1))", SM.new(12, 4)],
   ["Some((uint16, ?(complex64), align=1))", SM.new(12, 4)],

   # Records
   ["{}", SM.new(0, 1)],
   ["{x: complex128}", SM.new(16, 8)],

   ["{x: int8, y: int64}", SM.new(16, 8)],
   ["{x: int8, y: int64, pack=1}", SM.new(9, 1)],
   ["{x: int8, y: int64, pack=2}", SM.new(10, 2)],
   ["{x: int8, y: int64, pack=4}", SM.new(12, 4)],
   ["{x: int8, y: int64, pack=8}", SM.new(16, 8)],
   ["{x: int8, y: int64, pack=16}", SM.new(32, 16)],

   ["{x: uint16, y: {z: complex128}}", SM.new(24, 8)],
   ["{x: uint16, y: {z: complex128, align=16}}", SM.new(32, 16)],
   ["{x: uint16, y: {z: complex128}, align=16}", SM.new(32, 16)],

   # Primitive types
   ["bool", SM.new(1, 1)],

   ["int8", SM.new(1, 1)],
   ["int16", SM.new(2, 2)],
   ["int32", SM.new(4, 4)],
   ["int64", SM.new(8, 8)],

   ["uint8", SM.new(1, 1)],
   ["uint16", SM.new(2, 2)],
   ["uint32", SM.new(4, 4)],
   ["uint64", SM.new(8, 8)],

   ["float32", SM.new(4, 4)],
   ["float64", SM.new(8, 8)],

   ["complex64", SM.new(8, 4)],
   ["complex128", SM.new(16, 8)],

   # Primitive optional types
   ["?bool", SM.new(1, 1)],

   ["?int8", SM.new(1, 1)],
   ["?int16", SM.new(2, 2)],
   ["?int32", SM.new(4, 4)],
   ["?int64", SM.new(8, 8)],

   ["?uint8", SM.new(1, 1)],
   ["?uint16", SM.new(2, 2)],
   ["?uint32", SM.new(4, 4)],
   ["?uint64", SM.new(8, 8)],

   ["?float32", SM.new(4, 4)],
   ["?float64", SM.new(8, 8)],

   ["?complex64", SM.new(8, 4)],
   ["?complex128", SM.new(16, 8)],

   # References
   ["&bool", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["&int8", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&int16", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&int32", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&int64", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(uint8)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(uint16)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(uint32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(uint64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(float32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(float64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(complex64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(complex128)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   # Optional references
   ["?&bool", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["?&int8", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?&int16", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?&int32", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?&int64", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["?ref(uint8)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?ref(uint16)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?ref(uint32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?ref(uint64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["?ref(float32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?ref(float64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["?ref(complex64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["?ref(complex128)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   # References to optional types
   ["&?bool", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["&?int8", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&?int16", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&?int32", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["&?int64", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(?uint8)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(?uint16)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(?uint32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(?uint64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(?float32)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(?float64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   ["ref(?complex64)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],
   ["ref(?complex128)", SM.new(SIZEOF_PTR, SIZEOF_PTR)],

   # Constructors
   ["Some(bool)", SM.new(1, 1)],

   ["Some(int8)", SM.new(1, 1)],
   ["Some(int16)", SM.new(2, 2)],
   ["Some(int32)", SM.new(4, 4)],
   ["Some(int64)", SM.new(8, 8)],

   ["Some(uint8)", SM.new(1, 1)],
   ["Some(uint16)", SM.new(2, 2)],
   ["Some(uint32)", SM.new(4, 4)],
   ["Some(uint64)", SM.new(8, 8)],

   ["Some(float32)", SM.new(4, 4)],
   ["Some(float64)", SM.new(8, 8)],

   ["Some(complex64)", SM.new(8, 4)],
   ["Some(complex128)", SM.new(16, 8)],

   # Optional constructors
   ["?Some(bool)", SM.new(1, 1)],

   ["?Some(int8)", SM.new(1, 1)],
   ["?Some(int16)", SM.new(2, 2)],
   ["?Some(int32)", SM.new(4, 4)],
   ["?Some(int64)", SM.new(8, 8)],

   ["?Some(uint8)", SM.new(1, 1)],
   ["?Some(uint16)", SM.new(2, 2)],
   ["?Some(uint32)", SM.new(4, 4)],
   ["?Some(uint64)", SM.new(8, 8)],

   ["?Some(float32)", SM.new(4, 4)],
   ["?Some(float64)", SM.new(8, 8)],

   ["?Some(complex64)", SM.new(8, 4)],
   ["?Some(complex128)", SM.new(16, 8)],

   # Constructors containing optional types
   ["Some(?bool)", SM.new(1, 1)],

   ["Some(?int8)", SM.new(1, 1)],
   ["Some(?int16)", SM.new(2, 2)],
   ["Some(?int32)", SM.new(4, 4)],
   ["Some(?int64)", SM.new(8, 8)],

   ["Some(?uint8)", SM.new(1, 1)],
   ["Some(?uint16)", SM.new(2, 2)],
   ["Some(?uint32)", SM.new(4, 4)],
   ["Some(?uint64)", SM.new(8, 8)],

   ["Some(?float32)", SM.new(4, 4)],
   ["Some(?float64)", SM.new(8, 8)],

   ["Some(?complex64)", SM.new(8, 4)],
   ["Some(?complex128)", SM.new(16, 8)],
]

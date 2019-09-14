$LOAD_PATH.unshift File.expand_path('../../lib', __FILE__)
require 'gumath'

require 'minitest/autorun'

Gm = Gumath
Fn = Gumath::Functions
Ex = Gumath::Examples

# ======================================================================
#                          Minimal test cases
# ======================================================================

TEST_CASES = [
  [2000.times.map { |i| Float(i)/100.0 }, "2000 * float64", "float64"],
  
  [[1000.times.map { |i| Float(i)/100.0 }, 1000.times.map { |i| Float(i+1) }],
   "2 * 1000 * float64", "float64"],
  
  [[2.times.map { |i| Float(i+1) }] * 1000, "1000 * 2 * float64", "float64"],
  
  [2000.times.map { |i| Float(i)/10.0 }, "2000 * float32", "float32"],
  
  [[1000.times.map { |i| Float(i)/10.0 }, 1000.times.map { |i| Float(i+1) }],
   "2 * 1000 * float32", "float32"],

  [[2.times.map { |i| Float(i+1) }] * 1000, "1000 * 2 * float32", "float32"]
]

class Graph < XND
  def initialize *args
    super(*args, typedef: "graph")
  end

  def shortest_paths start
    Ex.single_source_shortest_paths start
  end
end

def assert_array_in_delta arr1, arr2, delta
  assert_equal arr1.size, arr2.size

  flat1 = arr1.flatten
  flat2 = arr2.flatten
  
  flat1.each_with_index do |d, i|
    assert_in_delta flat1[i], flat2[i], delta
  end
end


def assert_array arr1, arr2
  assert_equal arr1.size, arr2.size

  arr1.size.times do |i|
    assert_equal arr1[i], arr2[i]
  end
end

def compute func, data
  if data.is_a? Array
    data = data.map do |d|
      compute func, d
    end
  else
    Math.send(func, data)
  end
end

# ======================================================================
#            Definition of generalized slicing and indexing
# ======================================================================

def have_nil arr
  # TODO
end

def sinrec arr
  # TODO
end

def mulrec arr1, arr2
  # TODO
end

def maxlevel arr
  # TODO
end

def getitem arr, indices
  # TODO
end

class NDArray < Array
  # TODO
end

# ======================================================================
#                          Generate test cases 
# ======================================================================

SUBSCRIPT_FIXED_TEST_CASES = [
  [],
  [[]],
  [[], []],
  [[0], [1]],
  [[0], [1], [2]],
  [[0, 1], [1, 2], [2 ,3]],
  [[[]]],
  [[[0]]],
  [[[], []]],
  [[[0], [1]]],
  [[[0, 1], [2, 3]]],
  [[[0, 1], [2, 3]], [[4, 5], [6, 7]]],
  [[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]]
]

SUBSCRIPT_VAR_TEST_CASES = [
  [[[0, 1], [2, 3]], [[4, 5, 6], [7]], [[8, 9]]],
  [[[0, 1], [2, 3]], [[4, 5, nil], [nil], [7]], [[], [nil, 8]], [[9, 10]]],
  [[[0, 1, 2], [3, 4, 5, 6], [7, 8, 9, 10]], [[11, 12, 13, 14], [15, 16, 17], [18, 19]]],
  [[[0, 1], [2, 3], [4, 5]], [[6, 7], [8, 9]], [[10, 11]]]
]

def single_fixed max_ndim=4, min_shape=1, max_shape=10
  # TODO
end

def gen_fixed max_ndim=4, min_shape=1, max_shape=10
  # TODO
end

def single_var max_ndim=4, min_shape=1, max_shape=10
  # TODO
end

def gen_var max_ndim=4, min_shape=1, max_shape=10
  # TODO
end

def genindices
  # TODO
end

def rslice ndim
  # TODO
end

def rslice_neg ndim
  # TODO
end

def multislice ndim
  # TODO
end

def randslices ndim
  # TODO
end

def gen_indices_or_slices
  # TODO
end

def genslices n
  # TODO
end

def genslices_ndim ndim, shape
  # TODO
end

def mixed_index max_ndim
  # TODO
end

def mixed_index_neg max_ndim
  # TODO
end

def mixed_indices max_ndim
  # TODO
end

def itos indices
  # TODO
end

# ======================================================================
#                Split a shape into N almost equal slices
# ======================================================================

def start i, r, q
  # TODO
end

def stop i, r, q
  # TODO
end

def step i, r, q
  # TODO
end

def sl i, r, q
  # TODO
end

def prepend x, xs
  # TODO
end

def last_column i, r, q, n
  # TODO
end

def schedule n, shape
  # TODO
end

def column i, r, q, m, ms
  # TODO
end

# ======================================================================
#                   Split an xnd object into N subtrees
# ======================================================================

def zero_in_shape shape
  # TODO
end

def split_xnd x, n, max_outer=nil
  # TODO
end

# ======================================================================
#                           Generate test cases
# ======================================================================

functions = {
  "unary" => {
    "default" => ["copy", "abs"],
    "arith" => ["negative"],
    "complex_math_with_half" => ["exp", "log", "log10", "sqrt", "sin", "cos"],
    "complex_math" => ["tan", "asin", "acos", "atan", "sinh", "cosh", "tanh",
                        "asinh", "acosh", "atanh"],
    "real_math_with_half" => ["fabs", "exp2", "log2"],
    "real_math" => ["expm1", "log1p", "logb", "cbrt", "erf", "erfc", "lgamma",
                    "tgamma", "ceil", "floor", "trunc", "round", "nearbyint"],
    "bitwise" => ["invert"]
  },
  "binary" => {
    "default" => ["add", "subtract", "multiply", "floor_divide", "remainder", "power"],
    "float_result" => ["divide"],
    "bool_result" => ["less_equal", "less", "greater_equal", "greater", "equal", "not_equal"],
    "bitwise" => ["bitwise_and", "bitwise_or", "bitwise_xor"]
  },
  "binary_mv" => {
    "default" => ["divmod"]
  }
}

def complex_noimpl n
  functions["unary"]["real_math"].include?(n) ||
  functions["unary"]["real_math_with_half"].include?(n)
end

def half_noimpl n
  functions["unary"]["real_math"].include?(n) ||
  functions["unary"]["complex_math"].include?(n) ||
  ["floor_divide", "remainder"].include?(n)
end

TUNSIGNED = ["bool", "uint8", "uint16", "uint32", "uint64"]
TSIGNED = ["int8", "int16", "int32", "int64"]
TFLOAT = ["bfloat16", "float16", "float32", "float64"]
TCOMPLEX = ["complex32", "complex64", "complex128"]

TINFO = {
  "bool" => [0,1,0],
  "uint8" => [0, 2**8-1, 0],
  "uint16" => [0, 2**16-1, 0],
  "uint32" => [0, 2**32-1, 0],
  "uint64" => [0, 2**64-1, 0],
  "int8" => [-2**7, 2**7-1, 0],
  "int16" => [-2**15, 2**15-1, 0],
  "int32" => [-2**31, 2**31-1, 0],
  "int64" => [-2**63, 2**63-1, 0],
  "float16"  => [-2**11, 2**11, 15],
  "bfloat16" => [-2**8, 2**8, 127],
  "float32"  => [-2**24, 2**24, 127],
  "float64"  => [-2**53, 2**53, 1023],
  "complex32"=> [-2**11, 2**11, 15],
  "complex64"=> [-2**24, 2**24, 127],
  "complex128" => [-2**53, 2**53, 1023]
}

class Tint
  attr_accessor :type, :min, :max, :exp, :all

  def initialize type
    raise(ValueError, "not an integer type #{type}") unless 
      (TUNSIGNED + TSIGNED).include?(type)
    @type = type
    @min, @max, @exp = TINFO[type]
    @all = [@type, @min, @max, @exp]
  end

  def to_s
    @type
  end

  def == other
    other.is_a?(Tint) && @all == other.all
  end

  def testcases
    yield 0
    yield @min
    yield @max
    (1..10).each do |i|
      yield rand(@min, @max+1)
    end
  end

  def cpu_noimpl f=nil
    false
  end

  def cpu_nokern f=nil
    false
  end

  def cuda_noimpl f=nil
    false
  end

  def cuda_nokern f=nil
    false
  end
end # class TInt

class Tfloat
  attr_accessor :type, :min, :max, :exp, :all

  def initialize type
    raise(ValueError, "not a float type.") unless TFLOAT.include?(type)
    @type = type
    @min, @max, @exp = TINFO[type]
    @all = [@type, @min, @max, @exp]
  end

  def to_s
    @type
  end

  def == other
    other.is_a?(Tint) && @all == other.all
  end

  def testcases
    yield 0
    yield 0.5
    yield -0.5
    yield @min
    yield @max
    prec = rand(1..10)
  end

  def cpu_noimpl f=nil
    @type == "float16"
  end

  def cpu_nokern f=nil
    false
  end

  def cuda_noimpl f=nil
    if @type == "float16"
      return half_noimpl(f)
    end
  end

  def cuda_nokern f=nil
    false
  end
end # class Tfloat

class Tcomplex
  attr_accessor :type, :min, :max, :exp, :all

  def initialize type
    raise(ValueError, "not a complex type.") unless TCOMPLEX.include?(type)
    @type = type
    @min, @max, @exp = TINFO[type]
    @all = [@type, @min, @max, @exp]
  end

  def to_s
    @type
  end

  def == other
    other.is_a?(Tint) && @all == other.all
  end

  def testcases
    if block_given?
      yield 0
      yield 0.5
      yield -0.5
      yield @min
      yield @max
      prec = rand(1..10)
      # all_binary(prex, @exp, 1).each do |v, w|
      #   yield Complex(Float(v), Float(w))
      # end
      # bin_ranfloat.each do |v, w|
      #   yield Complex(Float(v), Float(w))
      # end
    end
  end

  def cpu_noimpl f=nil
    if @type == "complex32"
      return true
    end
    return complex_noimpl(f)
  end

  def cpu_nokern f=nil
    ["floor_divide", "remainder"].include?(f)
  end

  def cuda_noimpl f=nil
    return true if @type == "complex32"
    return complex_noimpl(f)
  end

  def cuda_nokern f=nil
    return ["floor_divide", "remainder"].include?(f)
  end
end # class Tcomplex

def tinfo_default
  [
    Tint.new("uint8"),
    Tint.new("uint16"),
    Tint.new("uint32"),
    Tint.new("uint64"),
    Tint.new("int8"),
    Tint.new("int16"),
    Tint.new("int32"),
    Tint.new("int64"),
    Tfloat.new("float16"),
    Tfloat.new("bfloat16"),
    Tfloat.new("float32"),
    Tfloat.new("float64"),
    Tcomplex.new("complex32"),
    Tcomplex.new("complex64"),
    Tcomplex.new("complex128")
  ]
end

def tinfo_bitwise
  [
    Tint.new("bool"),
    Tint.new("uint8"),
    Tint.new("uint16"),
    Tint.new("uint32"),
    Tint.new("uint64"),
    Tint.new("int8"),
    Tint.new("int16"),
    Tint.new("int32"),
    Tint.new("int64")
  ]  
end

def implemented_sigs
  {
    "unary" => {
      "default" => {}, "float_result" => {}
    },
    "binary" => {
      "default" => {}, "float_result" => {}, "bool_result" => {}, "bitwise" => {}
    },
    "binary_mv" => {
      "default" => {
        [Tint.new("uint8"),Tint.new("uint8")] => [Tint.new("uint8"), Tint.new("uint8")],
        [Tint.new("uint16"),Tint.new("uint16")] => [Tint.new("uint16"),Tint.new("uint16")],
        [Tint.new("uint32"),Tint.new("uint32")] => [Tint.new("uint32"),Tint.new("uint32")],
        [Tint.new("uint64"),Tint.new("uint64")] => [Tint.new("uint64"),Tint.new("uint64")],

        [Tint.new("int8"),Tint.new("int8")] => [Tint.new("int8"),Tint.new("int8")],
        [Tint.new("int16"),Tint.new("int16")] => [Tint.new("int16"),Tint.new("int16")],
        [Tint.new("int32"),Tint.new("int32")] => [Tint.new("int32"),Tint.new("int32")],
        [Tint.new("int64"),Tint.new("int64")] => [Tint.new("int64"),Tint.new("int64")],

        [Tfloat.new("float32"),Tfloat.new("float32")] => [Tfloat.new("float32"),
          Tfloat.new("float32")],
        [Tfloat.new("float64"),Tfloat.new("float64")] => [Tfloat.new("float64"),
          Tfloat.new("float64")]
      }
    }
  }
end

def exact_sigs
  {
    "unary" => {
      "default" => {}, "float_result" => {}
    },
    "binary" => {
      "default" => {}, "float_result" => {}, "bool_result" => {}, "bitwise" => {} 
    }
  }
end

def inexact_sigs
  {
    "unary" => {
      "default" => {}, "float_result" => {}
    },
    "binary" => {
      "default" => {}, "float_result" => {}, "bool_result" => {}, "bitwise" => {}
    }
  }
end

def init_unary_cast pattern, tinfo, rank
  # TODO
end

def init_unary_cast_tbl pattern
  # TODO
end

def is_binary_common_cast cast, t, u
  # TODO
end

def init_binary_cast pattern, tinfo, rank1, rank2
  # TODO
end

def init_binary_cast_tbl pattern
  # TODO
end

_struct_format = {
  "float16" => "e",
  "float32" => "f",
  "float64" => "d",
  "complex32" => "e",
  "complex64" => "f",
  "complex128" => "d"
}

def roundtrip_ne v, fmt
  # TODO
end

def struct_overflow v, t
  # TODO
end

init_unary_cast_tbl("default")
init_unary_cast_tbl("float_result")

init_binary_cast_tbl("default")
init_binary_cast_tbl("float_result")
init_binary_cast_tbl("bool_result")
init_binary_cast_tbl("bitwise")

_np_names = {
  "asin" => "arcsin",
  "acos" => "arccos",
  "atan" => "arctan",
  "asinh" => "arcsinh",
  "acosh" => "arccosh",
  "atanh" => "arctanh",
  "nearbyint" => "round"
}

def np_function name
  # TODO
end

def np_noimpl name
  # TODO
end

def gen_axes ndim
  # TODO
end

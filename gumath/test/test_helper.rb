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
    # TODO
  },
  "binary" => {
    # TODO
  },
  "binary_mv" => {
    "default" => ["divmod"]
  }
}

def complex_noimpl n
  # TODO
end

def half_noimpl n
  # TODO
end

tunsigned = ["bool", "uint8", "uint16", "uint32", "uint64"]
tsigned = ["int8", "int16", "int32", "int64"]
tfloat = ["bfloat16", "float16", "float32", "float64"]
tcomplex = ["complex32", "complex64", "complex128"]

tinfo = {
  # TODO
}

class Tint
  # TODO
end # class TInt

class Tfloat
  # TODO
end # class Tfloat

class Tcomplex
  # TODO
end # class Tcomplex

tinfo_default = [
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

tinfo_bitwise = [
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

implemented_sigs = {
  "unary" => {
    
  },
  "binary" => {

  },
  "binary_mv" => {

  }
}

exact_sigs = {

}

inexact_sigs = {

}

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
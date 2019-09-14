require 'test_helper'

class TestFunctionHash < Minitest::Test
  def test_hash_contents
    hash = Fn.instance_variable_get(:@gumath_functions)

    assert_instance_of Gumath::GufuncObject, hash[:sin]
  end
end

class TestCall < Minitest::Test
  class X < XND; end
  
  def test_subclass
    x = X.new [1,2,3]
    y = X.new [1,2,3]

    puts x.inspect

    z = Fn.multiply(x, y)
    assert_equal [1,4,9], z
    assert_equal XND, z.class

    # z = Fn.multiply(x, y, cls: X)
    # assert_equal [1,4,9], z
    # assert_equal X, z.class
  end
  
  def test_sin_scalar
    x1 = XND.new(1.2, type: "float64")
    y1 = Fn.sin(x1)
    
    x2 = XND.new(1.23e1, type: "float32")
    y2 = Fn.sin(x2)

    assert_in_delta y1.value, Math.sin(1.2), 0.00001
    assert_in_delta y2.value, Math.sin(1.23e1), 0.00001
  end

  def test_sin
    TEST_CASES.each do |data, t, dtype|
      x = XND.new data, type: t
      y = Fn.sin x
      
      sinned = compute :sin, data

      assert_array_in_delta y.value, sinned, 0.00001
    end
  end

  def test_sin_slice
    TEST_CASES.each do |data, t, dtype|
      x = XND.new data, type: t
      next if x.type.ndim < 2

      y = x[0..10, 0..10]
      z = Fn.sin y
    end
  end

  def test_copy
    TEST_CASES.each do |data, t, dtype|
      x = XND.new data, type: t
      y = Fn.copy x

      assert_array y.value, x.value
    end
  end

  def test_copy_slice
    TEST_CASES.each do |lst, t, dtype|
      x = XND.new(lst, type: t)
      next if x.type.ndim < 2

      y = x[0..10, 0..10]
      z = Fn.copy(y)

      assert_array z.value, y.value
    end
  end

  def test_quaternion
    data = [
      [[1+2i, 4+3i],
       [-4+3i, 1-2i]],
      [[4+2i, 1+10i],
       [-1+10i, 4-2i]],
      [[-4+2i, 3+10i],
       [-3+10i, -4-2i]]
    ]

    x = XND.new data, type: "3 * quaternion64"
    y = Ex.multiply x, x

    # FIXME: how to do einsum in Ruby?

    x = XND.new data, type: "3 * quaternion128"
    y = Ex.multiply x, x

    x = XND.new "xyz"
    assert_raises(TypeError) { Ex.multiply x, x }
  end

  def test_quaternion_error
    data = [
      [[1+2i, 4+3i],
       [-4+3i, 1-2i]],
      [[4+2i, 1+10i],
       [-1+10i, 4-2i]],
      [[-4+2i, 3+10i],
       [-3+10i, -4-2i]]
    ]
    x = XND.new data, type: "3 * Foo(2 * 2 * complex64)"
    assert_raises(TypeError) { Ex.multiply(x, x) }
  end

  def test_void
    x = Ex.randint

    assert_equal x.type, NDT.new("int32")
  end

  def test_multiple_return
    x, y = Ex.randtuple

    assert_equal x.type, NDT.new("int32")
    assert_equal y.type, NDT.new("int32")

    x, y = Ex.divmod10 XND.new(233)

    assert_equal x.value, 23
    assert_equal y.value, 3
  end
end

class TestMissingValues < Minitest::Test
  def test_missing_values
    x = [{'index'=> 0, 'name'=> 'brazil', 'value'=> 10},
         {'index'=> 1, 'name'=> 'france', 'value'=> nil},
         {'index'=> 1, 'name'=> 'russia', 'value'=> 2}]

    y = [{'index'=> 0, 'name'=> 'iceland', 'value'=> 5},
         {'index'=> 1, 'name'=> 'norway', 'value'=> nil},
         {'index'=> 1, 'name'=> 'italy', 'value'=> nil}]

    z = XND.new [x, y], type: "2 * 3 * {index: int64, name: string, value: ?int64}"
    ans = Ex.count_valid_missing z

    assert_equal ans.value, [{'valid' => 2, 'missing' => 1}, {'valid' => 1, 'missing' => 2}]
  end

  def test_unary
    a = [0, nil, 2]
    ans = XND.new([a.map { |x| x.nil? ? nil : Math.sin(x) }])

    x = XND.new(a, dtype: "?float64")
    y = Fn.sin(x)

    assert_equal y.value, ans
  end

  def test_binary
    a = [3, nil, 3]
    b = [100, 1, nil]
    ans = XND.new([
      a.zip(b).map {|t0, t1| t0.nil? && t1.nil? ? nil : t0 * t1 }
    ])
    
    x = XND.new a, dtype: "?float64"
    y = Fn.sin(x)

    assert_equal y.value, ans
  end

  def test_reduce
    a = [1, nil, 2]
    x = XND.new a

    y = Gm.reduce Fn, :add, x
    assert_equal y, nil

    y = Gm.reduce Fn, :multiply, x
    assert_equal y, nil

    y = Gm.reduce Fn, :subtract, x
    assert_equal y, nil

    x = XND.new [], dtype: "?int32"
    y = Gm.reduce Fn, :add, x
    assert_equal y, 0
  end

  def test_reduce_cuda
    skip
    a = [1,nil,2]
    x = XND.new a, device: "cuda:managed"

    y = Gm.reduce Cd, :add, x
    assert_equal y, nil

    y = Gm.reduce Cd, :multiply, x
    assert_equal y, nil

    x = XND.new [], dtype: "?int32", device: "cuda:managed"
    y = Gm.reduce Fn, :add, x
    assert_equal y, 0
  end

  def test_comparisons
    a = [1, nil, 3, 5]
    b = [2, nil, 3, 4]

    x = XND.new a
    y = XND.new b

    ans = Fn.equal x, y
    assert_equal ans.value, [false, nil, true, false]

    ans = Fn.not_equal x, y
    assert_equal ans.value, [true, nil, false, true]

    ans = Fn.less x, y
    assert_equal ans.value, [true, nil, false, false]

    ans = Fn.less_equal x, y
    assert_equal ans.value, [true, nil, true, false]

    ans = Fn.greater_equal x, y
    assert_equal ans.value, [false, nil, true, true]

    ans = Fn.greater x, y
    assert_equal ans.value, [false, nil, false, true]
  end

  def test_comparisons_cuda
    skip
    a = [1, nil, 3, 5]
    b = [2, nil, 3, 4]

    x = XND.new a, device: "cuda:managed"
    y = XND.new b, device: "cuda:managed"

    ans = Fn.equal x, y
    assert_equal ans.value, [false, nil, true, false]

    ans = Fn.not_equal x, y
    assert_equal ans.value, [true, nil, false, true]

    ans = Fn.less x, y
    assert_equal ans.value, [true, nil, false, false]

    ans = Fn.less_equal x, y
    assert_equal ans.value, [true, nil, true, false]

    ans = Fn.greater_equal x, y
    assert_equal ans.value, [false, nil, true, true]

    ans = Fn.greater x, y
    assert_equal ans.value, [false, nil, false, true]
  end

  def test_equaln
    a = [1, nil, 3, 5]
    b = [2, nil, 3, 4]
  end
end

class TestEqualN < Minitest::Test
  def test_nan_float
    ["bfloat16", "float32", "float64"].each do |dtype|
      x = XND.new [0, Float::NAN, 2], dtype: dtype

      y = XND.new [0, Float::NAN, 2], dtype: dtype
      z = Fn.equaln x, y

      y = XND.new [0,1,2], dtype: dtype
      z = Fn.equaln x, y

      assert_equal z, [true, false, true]
    end
  end

  def test_nan_complex
    ["complex64", "complex128"].each do |dtype|
      [
        [Complex(Float::NAN, 1.2), Complex(Float::NAN, 1.2), true],
        [Complex(Float::NAN, 1.2), Complex(Float::NAN, 1), false],
        [Complex(Float::NAN, Float::NAN), Complex(Float::NAN, 1.2), false],

        [Complex(1.2, Float::NAN), Complex(1.2, Float::NAN), true],
        [Complex(1.2, Float::NAN), Complex(1, Float::NAN), false],
        [Complex(Float::NAN, Float::NAN), Complex(1.2, Float::NAN), false],

        [Complex(Float::NAN, Float::NAN), Complex(Float::NAN, Float::NAN), true]
      ].each do |a, b, ans|
        x = XND.new [0,a,2], dtype: dtype
        y = XND.new [0,b,2], dtype: dtype
        z = Fn.equaln x, y

        assert_equal z, [true, ans, true]
      end
    end
  end

  def test_nan_float_cuda
    skip
    ["bfloat16", "float16", "float32", "float64"].each do |dtype|
      x = XND.new [0,Float::NAN, 2], dtype: dtype, device: "cuda:managed"
      
      y = XND.new [0,Float::NAN, 2], dtype: dtype, device: "cuda:managed"
      z = Cd.equaln x, y
      assert_equal z, [true, true, true]

      y = XND.new [0,1,2], dtype: dtype, device: "cuda:managed"
      z = Cd.equaln x, y
      assert_equal z, [true, false, true]
    end
  end

  def test_nan_complex_cuda
    skip
    ["complex64", "complex128"].each do |dtype|
      [
        [Complex(Float::NAN, 1.2), Complex(Float::NAN, 1.2), true],
        [Complex(Float::NAN, 1.2), Complex(Float::NAN, 1), false],
        [Complex(Float::NAN, Float::NAN), Complex(Float::NAN, 1.2), false],

        [Complex(1.2, Float::NAN), Complex(1.2, Float::NAN), true],
        [Complex(1.2, Float::NAN), Complex(1, Float::NAN), false],
        [Complex(Float::NAN, Float::NAN), Complex(1.2, Float::NAN), false],

        [Complex(Float::NAN, Float::NAN), Complex(Float::NAN, Float::NAN), true] 
      ].each do |a, b, ans|
        x = XND.new [0,a,2], dtype: dtype, device: "cuda:managed"
        y = XND.new [0,b,2], dtype: dtype, device: "cuda:managed"
        z = Cd.equaln x, y
        assert_equal z, [true, ans, true]
      end
    end
  end
end # class TestEqualN

class TestFlexibleArrays < Minitest::Test
  def test_sin_var_compatible
    lst = [
      [
        [1.0],
        [2.0, 3.0],
        [4.0, 5.0, 6.0]
      ],
      [
        [7.0],
        [8.0, 9.0],
        [10.0, 11.0, 12.0]
      ]]

    ans = [
      [
        [Math.sin(1.0)],
        [Math.sin(2.0), Math.sin(3.0)],
        [Math.sin(4.0), Math.sin(5.0), Math.sin(6.0)]
      ],
      [
        [Math.sin(7.0)],
        [Math.sin(8.0), Math.sin(9.0)],
        [Math.sin(10.0), Math.sin(11.0), Math.sin(12.0)]
      ]
    ]
    
    x = XND.new lst, type: "array * array * array * float64"
    y = Fn.sin x
    assert_equal y.value, ans
  end

  def test_add
    a = [
      [[1.0],
      [2.0, 3.0],
      [4.0, 5.0, 6.0]],
      [[7.0],
      [8.0, 9.0],
      [10.0, 11.0, 12.0]]
    ]

    b = [[[2.0],
      [3.0, 4.0],
      [5.0, 6.0, 7.0]],
      [[-8.0],
      [-9.0, -10.0],
      [111.1, 121.2, 25.3]]
    ]

    ans = [[[1.0+2.0],
      [2.0+3.0, 3.0+4.0],
      [4.0+5.0, 5.0+6.0, 6.0+7.0]],
      [[7.0-8.0],
      [8.0-9.0, 9.0-10.0],
      [10.0+111.1, 11.0+121.2, 12.0+25.3]]
    ]

    x = XND.new a, type: "array * array * array * float64"
    y = XND.new b, type: "array * array * array * float64"
    z = Fn.add x, y
    assert_equal z.value, ans
  end         
end # class TestFlexibleArrays

class TestRaggedArrays < Minitest::Test
  def test_sin
    data = [
      [[1.0],
       [2.0, 3.0],
       [4.0, 5.0, 6.0]],
      [[7.0],
       [8.0, 9.0],
       [10.0, 11.0, 12.0]]
    ]

    ans = [[[Math.sin(1.0)],
            [Math.sin(2.0), Math.sin(3.0)],
            [Math.sin(4.0), Math.sin(5.0), Math.sin(6.0)]],
           [[Math.sin(7.0)],
            [Math.sin(8.0), Math.sin(9.0)],
            [Math.sin(10.0), Math.sin(11.0), Math.sin(12.0)]]]
    
    x = XND.new data
    y = Fn.sin x

    assert_array y.value, ans
  end
end

class TestGraphs < Minitest::Test
  def test_shortest_path
    graphs = [
      [[[1, 1.2],  [2, 4.4]],
       [[2, 2.2]],
       [[1, 2.3]]],

      [[[1, 1.2], [2, 4.4]],
       [[2, 2.2]],
       [[1, 2.3]],
       [[2, 1.1]]]
    ]

    ans = [[[[0], [0, 1], [0, 1, 2]],      # graph1, start 0
            [[], [1], [1, 2]],             # graph1, start 1
            [[], [2, 1], [2]]],            # graph1, start 2

           [[[0], [0, 1], [0, 1, 2], []],  # graph2, start 0
            [[], [1], [1, 2], []],         # graph2, start 1
            [[], [2, 1], [2], []],         # graph2, start 2
            [[], [3, 2, 1], [3, 2], [3]]]] # graph2, start 3
    
    graphs.each do |i, arr|
      n = arr.size
      graph = Graph.new arr

      n.times do |start|
        node = XND.new start, type: "node"
        x = graph.shortest_paths node

        assert_equal x.value, ans[i][start]
      end
    end
  end

  def test_constraint
    data = [
      [[0, 1.2]],
      [[2, 2.2], [1, 0.1]]
    ]

    assert_raises(ValueError) { Graph.new(data) }
  end
end

class TestPdist < Minitest::Test
  def test_exceptions
    x = XND.new [], dtype: "float64"
    assert_raises(TypeError) { Ex.euclidian_pdist x }

    x = XND.new [[]], dtype: "float64"
    assert_raises(TypeError) { Ex.euclidian_pdist x }

    x = XND.new [[], []], dtype: "float64"
    assert_raises(TypeError) { Ex.euclidian_pdist x }

    x = XND.new [[1], [1]], dtype: "int64"
    assert_raises(TypeError) { Ex.euclidian_pdist x }
  end

  def test_pdist
    x = XND.new [[1]], dtype: "float64"
    y = Ex.euclidian_pdist x
    assert_equal y.value, []

    x = XND.new [[1,2,3]], dtype: "float64"
    y = Ex.euclidian_pdist x
    assert_equal y.value, []

    x = XND.new([[-1.2200, -100.5000,   20.1250,  30.1230],
                 [ 2.2200,    2.2720, -122.8400, 122.3330],
                 [ 2.1000,  -25.0000,  100.2000, -99.5000]], dtype: "float64")
    y = Ex.euclidian_pdist x
    assert_equal y.value, [198.78529349275314, 170.0746899276903, 315.75385646576035]
  end
end

class TestOut < Minitest::Test
  def test_api_cpu
    # negative
    x = XND.new [1,2,3]
    y = XND.empty "3 * int64"
    z = Fn.negative x, out: y

    assert_equal z.object_id, y.object_id
    assert_equal y, XND.new([-1, -2, -3])

    # divmod
    x = XND.new [10, 20, 30]
    y = XND.new [7,8,9]
    a = XND.empty "3 * int64"
    b = XND.empty "3 * int64"
    q, r = Fn.divmod x, y, out: [a, b]

    assert_equal q.object_id, a.object_id
    assert_equal r.object_id, b.object_id

    assert_equal q, XND.new([1,2,3])
    assert_equal r, XND.new([3,4,3])
  end

  def test_api_cuda
    skip
    x = XND.new [1,2,3], device: "cuda:managed"
    y = XND.empty "3 * int64", device: "cuda:managed"
    z = Cd.negative x, out: y

    assert_equal z.object_id, y.object_id
    assert_equal y, XND.new([-1,-2,-3])

    # divmod
    x = XND.new [10, 20, 30], device: "cuda:managed"
    y = XND.new [7,8,9], device: "cuda:managed"
    a = XND.empty "3 * int64", device: "cuda:managed"
    b = XND.empty "3 * int64", device: "cuda:managed"
    q, r = Fn.divmod x, y, out: [a, b]

    assert_equal q.object_id, a.object_id
    assert_equal r.object_id, b.object_id

    assert_equal q, XND.new([1,2,3])
    assert_equal r, XND.new([3,4,3])    
  end

  def test_broadcast_cpu
    # multiply
    x = XND.new [1,2,3]
    y = XND.new [2]
    z = XND.empty "3 * int64"
    ans = Fn.multiply x, y, out: z

    assert_equal ans.object_id, z.object_id
    assert_equal ans, XND.new([2,4,6])

    x = XND.new [1,2,3]
    y = XND.new 2
    z = XND.empty "3 * int64"
    ans = Fn.multiply x, y, out: z

    assert_equal ans.object_id, z.object_id
    assert_equal ans, XND.new([2,4,6])

    # divmod
    x = XND.new [10,20,30]
    y = XND.new [3]
    a = XND.empty "3 * int64"
    b = XND.empty "3 * int64"
    q, r = Fn.divmod x, y, out: [a,b]

    assert_equal q.object_id, a.object_id
    assert_equal r.object_id, b.object_id
    assert_equal q, XND.new([3,6,10])
    assert_equal r, XND.new([1,2,0])
  end

  def test_broadcast_cuda
    skip
    x = XND.new [1,2,3], device: "cuda:managed"
    y = XND.new [2], device: "cuda:managed"
    z = XND.empty "3 * int64", device: "cuda:managed"
    ans = Fn.mutiply x, y, out: z

    assert_equal ans.object_id, z.object_id
    assert_equal ans, XND.new([2,4,6])
  end
end # class TestOut

class TestUnaryCPU < Minitest::Test  
  def test_acos
    a = [0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7] 
    b = a.map{ |x| Math.acos(x) }

    x = XND.new a, dtype: "float64"
    y = Fn.acos x

    assert_equal y, b
  end

  def test_acos_opt
    a = [0, 0.1, 0.2, nil, 0.4, 0.5, 0.6, nil]
    b = a.map { |x| x.nil? ? nil : Math.acos(x) }

    x = XND.new a, dtype: "?float64"
    y = Fn.acos x

    assert_equal y, b
  end

  def test_inexact_cast
    a = [0,1,2,3,4,5,6,7]
    x = XND.new a, dtype: "int64"

    assert_raises(ValueError) { Fn.sin(x) }
  end
end # class TestUnaryCPU

class TestUnaryCUDA < Minitest::Test
  def test_cos
    skip
    a = [0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7] 
    b = a.map{ |x| Math.cos(x) }

    x = XND.new a, dtype: "float64", device: "cuda:managed"
    y = Fn.cos x

    assert_equal y, b
  end

  def test_cos_opt
    skip
    a = [0, 0.1, 0.2, nil, 0.4, 0.5, 0.6, nil]
    b = a.map { |x| x.nil? ? nil : Math.cos(x) }

    x = XND.new a, dtype: "?float64", device: "cuda:managed"
    y = Fn.cos x

    assert_equal y, b
  end

  def test_inexact_cast
    skip
    a = [0,1,2,3,4,5,6,7]
    x = XND.new a, dtype: "int64", device: "cuda:managed"

    assert_raises(ValueError) { Fn.sin(x) }
  end
end # class TestUnaryCUDA

class TestBinaryCPU < Minitest::Test
  def test_binary
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,2,3,4,5,6,7], dtype: t.type
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type
      z = Fn.add x, y

      assert_equal z, [1,3,5,7,9,11,13,15]
    end
  end

  def test_add_opt
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,nil,3,4,5,6,7], dtype: "?"+t.type
      y = XND.new [1,2,3,4,5,6,nil,8], dtype: "?"+u.type
      z = Fn.add x, y

      assert_equal z, [1,3,nil,7,9,11,nil,15]
    end
  end

  def test_subtract
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [2,3,4,5,6,7,8,9], dtype: t.type
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type
      z = Fn.subtract x, y

      assert_equal z, [1,1,1,1,1,1,1,1]
    end
  end

  def test_multiply
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [2,3,4,5,6,7,8,9], dtype: t.type
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type

      z = Fn.multiply x, y

      assert_equal z, [2,6,12,20,30,42,56,72]
    end
  end
end # class TestBinaryCPU

class TestBinaryCUDA < Minitest::Test
  def test_binary
    skip
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,2,3,4,5,6,7], dtype: t.type, device: "cuda:managed"
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type, device: "cuda:managed"
      z = Fn.add x, y

      assert_equal z, [1,3,5,7,9,11,13,15]
    end
  end

  def test_add_opt
    skip
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,nil,3,4,5,6,7], dtype: "?"+t.type, device: "cuda:managed"
      y = XND.new [1,2,3,4,5,6,nil,8], dtype: "?"+u.type, device: "cuda:managed"
      z = Fn.add x, y

      assert_equal z, [1,3,nil,7,9,11,nil,15]
    end
  end

  def test_subtract
    skip
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [2,3,4,5,6,7,8,9], dtype: t.type, device: "cuda:managed"
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type, device: "cuda:managed"
      z = Fn.subtract x, y

      assert_equal z, [1,1,1,1,1,1,1,1]
    end
  end

  def test_multiply
    skip
    implemented_sigs["binary"]["default"].each do |t, u|
      w = implemented_sigs["binary"]["default"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [2,3,4,5,6,7,8,9], dtype: t.type, device: "cuda:managed"
      y = XND.new [1,2,3,4,5,6,7,8], dtype: u.type, device: "cuda:managed"

      z = Fn.multiply x, y

      assert_equal z, [2,6,12,20,30,42,56,72]
    end
  end
end # class TestBinaryCUDA

class TestBitwiseCPU < Minitest::Test
  def test_add
    implemented_sigs["binary"]["bitwise"].each do |t, u|
      w = implemented_sigs["binary"]["bitwise"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,0,1,1,1,1,0], dtype: t.type
      y = XND.new [1,0,0,0,1,1,1,1], dtype: u.type
      z = Fn.bitwise_and x, y

      assert_equal z, [0,0,0,0,1,1,1,0]
    end
  end

  def test_and_opt
    implemented_sigs["binary"]["bitwise"].each do |t, u|
      w = implemented_sigs["binary"]["bitwise"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      a = [0,1,nil,1,1,1,1,0]
      b = [1,1,1,1,1,1,nil,0]
      c = [0,1,nil,1,1,1,nil,0]

      x = XND.new a, dtype: "?"+t.type
      y = XND.new b, dtype: "?"+u.type
      z = Fn.bitwise_and x, y

      assert_equal z, c
    end   
  end
end # class TestBitwiseCPU

class TestBitwiseCUDA < Minitest::Test
  def test_add
    skip
    implemented_sigs["binary"]["bitwise"].each do |t, u|
      w = implemented_sigs["binary"]["bitwise"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      x = XND.new [0,1,0,1,1,1,1,0], dtype: t.type, device: "cuda:managed"
      y = XND.new [1,0,0,0,1,1,1,1], dtype: u.type, device: "cuda:managed"
      z = Fn.bitwise_and x, y

      assert_equal z, [0,0,0,0,1,1,1,0]
    end
  end

  def test_add_opt
    skip
    implemented_sigs["binary"]["bitwise"].each do |t, u|
      w = implemented_sigs["binary"]["bitwise"][[t,u]]
      next if t.cpu_noimpl || u.cpu_noimpl

      a = [0,1,nil,1,1,1,1,0]
      b = [1,1,1,1,1,1,nil,0]
      c = [0,1,nil,1,1,1,nil,0]

      x = XND.new a, dtype: "?"+t.type, device: "cuda:managed"
      y = XND.new b, dtype: "?"+u.type, device: "cuda:managed"
      z = Fn.bitwise_and x, y

      assert_equal z, c
    end
  end
end # class TestBitwiseCUDA

class TestFunctions < Minitest::Test
  # FIXME: use some numpy substititute for these tests.
  def assert_rel_error_less calc, expected, maxerr, msg
    skip
  end

  def equal calc, expected, msg
    skip
  end

  def assert_equal func, z1, z2, w, msg, a=nil, b=nil
    skip
  end

  def create_xnd a, t, dev=nil
    skip
  end

  def check_unary_not_implemented func, a, t, mod=Fn, dev=nil
    skip
  end

  def check_unary_type_error func, a, t, mod=Fn, dev=nil
    skip
  end

  def check_unary func, a, t, u, mod=Fn, dev=nil
    skip
  end

  def check_binary_not_implemented func, a, t, b, u, mod=Fn, dev=nil
    skip
  end

  def check_binary_type_error func, a, t, b, u, mod=Fn, dev=nil
    skip
  end

  def check_binary func, a, t, b, u, w, mod=Fn, dev=nil
    skip
  end

  def check_binary_mv func, a, t, b, u, v, w, mod=Fn, devl=nil
    skip
  end

  def test_unary_cpu
    skip
  end

  def test_binary_cpu
    skip
  end

  def test_binary_mv_cpu
    skip
  end

  def test_unary_cpu
    skip
  end

  def test_binary_cuda
    skip
  end

  def test_binary_mv_cuda
    skip
  end

  def test_divide_inexact_cpu
    skip
  end

  def test_divide_inexact_cuda
    skip
  end

  def test_divmod_type_error_cpu
    skip
  end

  def test_divmod_type_error_cuda
    skip
  end
end # class TestFunctions

class TestCudaManaged < Minitest::Test
  def test_mixed_functions
    skip
    x = XND.new [1,2,3]
    y = XND.new [1,2,3]

    a = XND.new [1,2,3], device: "cuda:managed"
    b = XND.new [1,2,3], device: "cuda:managed"

    x = Fn.multiply x, y
    c = Cd.multiply a, b
    assert_equal z, c

    z = Fn.multiply a, b
    assert_equal z, c

    z = Fn.multiply x,b
    assert_equal z, c

    z = Fn.multiply a, y
    assert_equal z, c

    assert_raises(ValueError) { Cd.multiply(x, y) }
    assert_raises(ValueError) { Cd.multiply(x, b) }
    assert_raises(ValueError) { Cd.multiply(a, y) }
  end
end # class TestCudaManaged

class TestSpec < Minitest::Test
  # FIXME: figure out this one and implement.
end # class TestSpec

class LongIndexSliceTest < Minitest::Test
  def test_subarray
    skip
  end

  def test_subarray_cuda
    skip
  end

  def test_slices
    skip
  end

  def test_slices_cuda
    skip
  end

  def test_chained_indices_slices
    skip
  end

  def test_mixed_indices_slices
    skip
  end

  def test_var_mixed_indices_slices
    skip
  end

  def test_slices_brute_force
    skip
  end
end # class LongIndexSliceTest

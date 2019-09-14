require 'ndtypes'
require 'xnd'

require 'etc'

begin
  require 'ruby_gumath.so'
rescue LoadError
  require 'ruby_gumath/ruby_gumath.so'
end
require 'gumath/version'

class Gumath
  MAXCAST = {
    NDT.new("int8")  => NDT.new("int64"),
    NDT.new("int16") => NDT.new("int64"),
    NDT.new("int32") => NDT.new("int64"),
    NDT.new("int64") => NDT.new("int64"),
    NDT.new("uint8") => NDT.new("uint64"),
    NDT.new("uint16") => NDT.new("uint64"),
    NDT.new("uint32") => NDT.new("uint64"),
    NDT.new("uint64") => NDT.new("uint64"),
    NDT.new("bfloat16")=> NDT.new("float64"),
    NDT.new("float16") => NDT.new("float64"),
    NDT.new("float32") => NDT.new("float64"),
    NDT.new("float64") => NDT.new("float64"),
    NDT.new("complex32") => NDT.new("complex128"),
    NDT.new("complex64") => NDT.new("complex128"),
    NDT.new("complex128")=> NDT.new("complex128"),

    NDT.new("?int8") => NDT.new("?int64"),
    NDT.new("?int16")=> NDT.new("?int64"),
    NDT.new("?int32")=> NDT.new("?int64"),
    NDT.new("?int64")=> NDT.new("?int64"),
    NDT.new("?uint8")=> NDT.new("?uint64"),
    NDT.new("?uint16")=> NDT.new("?uint64"),
    NDT.new("?uint32")=> NDT.new("?uint64"),
    NDT.new("?uint64")=> NDT.new("?uint64"),
    NDT.new("?bfloat16") => NDT.new("?float64"),
    NDT.new("?float16")  => NDT.new("?float64"),
    NDT.new("?float32")  => NDT.new("?float64"),
    NDT.new("?float64")  => NDT.new("?float64"),
    NDT.new("?complex32")=> NDT.new("?complex128"),
    NDT.new("?complex64")=> NDT.new("?complex128"),
    NDT.new("?complex128")=> NDT.new("?complex128"),
  }
  
  class << self
    def reduce mod, meth, x, axes=0, dtype=nil
      if dtype.nil?
        dtype = MAXCAST[x.dtype]
      end

      reduce_cpu(mod, meth, x, axes, dtype)
    end

    def reduce_cpu mod, meth, x, axes, dtype
      
    end
  end
end

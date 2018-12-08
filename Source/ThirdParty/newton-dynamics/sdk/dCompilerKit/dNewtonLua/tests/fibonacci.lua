
	-- Naive recursive
	function Fibonacci.naive(n)
	  local function inner(m)
		if m < 2 then
		  return m
		end
		return inner(m-1) + inner(m-2)
	  end
	  return inner(n)
	end

	-- Tail-optimized recursive
	function Fibonacci.tail_call(n)
	  local function inner(m, a, b)
		if m == 0 then
		  return a
		end
		return inner(m-1, b, a+b)
	  end
	  return inner(n, 0, 1)
	end

	-- Iterative
	function Fibonacci.iterative(n)
	  a, b = 0, 1
	  for i = 1, n do
		a, b = b, a + b
	  end
	  return a
	end


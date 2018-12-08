	-- test liveness
    function RegisterAllocation (a, b, c, d)
		local e = d + a
		local f = b + c
		f = f + b
		if f == 0 then
			d = e + f
		else
			d = e - f
		end
		return d;
    end
    

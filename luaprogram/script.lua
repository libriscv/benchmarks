array = {}

function empty_function()
	-- do nothing
end

function test(arg)
	table.insert(array, arg)
end

function test_args(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	if arg1 == "This is a string"
		and arg2[0] == 222 and arg2[1] == 666
		and arg3 == 333 and arg4 == 444
		and arg5 == 555 and arg6 == 666
		and arg7 == 777 and arg8 == 888 then
		return 666
	end
	return -1
end

function test_maffs1(arg1, arg2)
	a = arg1 + arg2
	b = arg1 - arg2
	c = arg1 * arg2
	d = arg1 / arg2
	return a + b + c + d
end
function test_maffs2(arg1, arg2, arg3)
	return (arg1 * arg1 * arg3) / (arg2 * arg2 * arg3) + math.fmod(arg1, arg3)
end
function test_maffs3(arg1, arg2, arg3)
	return (arg1 ^ arg2) ^ (1.0 / arg3) ^ (1.0 / arg3)
end
function test_fib(n, acc, prev)
	if (n < 1) then
		return acc
	else
		return test_fib(n - 1, prev + acc, acc)
	end
end

function test_sieve(n)
	local prime = {}
	local primes = 0

	for i = 2, n do prime[i] = true end

	for i = 2, n do
		if prime[i] then
			primes = primes + 1
			for j = i*2, n, i do prime[j] = false end
		end
	end
	return primes
end

function test_syscall()
	script.nada()
end
function test_print()
	script.print("This is a string")
end

function test_longcall()
	for i=1,10 do
		script.longcall("This is a string", 2, 3, 4, 5, 6, 7)
	end
end

function test_threads()
	co = coroutine.create(function ()
		coroutine.yield()
	end)
	coroutine.resume(co)
	coroutine.resume(co)
end

local testvalue = 0
function test_threads_args1()
	co = coroutine.create(function (arg1, arg2, arg3, arg4)
		coroutine.yield()
		testvalue = arg1 + arg2 + arg3 + arg4
	end)
	coroutine.resume(co, 1, 2, 3, 4)
	return coroutine.resume(co)
end

function test_threads_args2()
	co = coroutine.create(function (arg1, arg2, arg3, arg4)
		coroutine.yield()
		return arg1 + arg2 + arg3 + arg4
	end)
	coroutine.resume(co, 1, 2, 3, 4)
	return coroutine.resume(co)
end


function table.shallow_copy(t)
  local t2 = {}
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

src = {}    -- new array
dst = {}
for i=1, 300 do
  src[i] = 0
  dst[i] = 0
end

function test_memcpy()
	for i=1, 300 do
		dst[i] = src[i]
	end
end

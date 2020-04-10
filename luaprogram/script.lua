array = {}

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

function test_maffs(arg1, arg2)
	a = arg1 + arg2
	b = arg1 - arg2
	c = arg1 * arg2
	d = arg1 / arg2
	return a + b + c + d
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
		--script.print("This is a string")
		coroutine.yield()
		--script.print("This is a string")
	end)
	coroutine.resume(co)
	coroutine.resume(co)
end

function test_threads_args()
	co = coroutine.create(function (arg1, arg2, arg3, arg4)
		coroutine.yield()
		return arg1 + arg2 + arg3 + arg4
	end)
	coroutine.resume(co, 1, 2, 3, 4)
	return coroutine.resume(co)
end

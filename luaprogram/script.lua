array = {}

function test(arg)
	table.insert(array, arg)
end

function test_args(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	table.insert(array, arg1)
	table.insert(array, arg2)
	table.insert(array, arg3)
	table.insert(array, arg4)
	table.insert(array, arg5)
	table.insert(array, arg6)
	table.insert(array, arg7)
	table.insert(array, arg8)
end

function test_maffs(arg1, arg2)
	a = arg1 + arg2
	b = arg1 - arg2
	c = arg1 * arg2
	d = arg1 / arg2
	return a + b + c + d
end

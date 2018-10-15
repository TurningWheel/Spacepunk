-- Common functions used for 'key-value' generic entity properties.

function string:split(sep)
	local sep, fields = sep or ":", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields+1] = c end)
	return fields
end

--local example = "a,b,cde"
--for index,value in pairs(example:split(",")) do
--	print(value)
--end
--print(example:split(",")[2]) --Also valid, will print out "b"

--local example2 = "a,b,cde"
--s = example2:split(",")
--print(s[1]) --Will print out "a"

function process()
end

function field(dim, uid, str)
	local num = tonumber(str)
	if num ~= nil then
		editor:entityComponentRotate(uid, dim, num)
	end
end
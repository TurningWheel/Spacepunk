function process()
end

function field(dim, uid, str)
	local num = tonumber(str)
	if num ~= nil then
		editor:entityComponentScale(uid, dim, num)
	end
end
function process()
end

function field(dim, uid, str)
	editor:entityComponentRotate(uid, dim, tonumber(str))
end
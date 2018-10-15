function process()
end

function field(dim, uid, str)
	editor:entityComponentScale(uid, dim, tonumber(str))
end
function process()
end

function field(dim, uid, str)
	editor:entityComponentTranslate(uid, dim, tonumber(str))
end
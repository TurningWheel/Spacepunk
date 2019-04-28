function process()
end

function field(uid, color, str)
	editor:entityModelCustomColorChannel(uid, 2, color, tonumber(str))
end
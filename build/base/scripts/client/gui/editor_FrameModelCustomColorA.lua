function process()
end

function field(uid, color, str)
	editor:entityModelCustomColorChannel(uid, 3, color, tonumber(str))
end
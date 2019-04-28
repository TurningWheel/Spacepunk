function process()
end

function field(uid, color, str)
	editor:entityModelCustomColorChannel(uid, 1, color, tonumber(str))
end
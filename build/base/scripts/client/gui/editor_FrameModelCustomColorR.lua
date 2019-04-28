function process()
end

function field(uid, color, str)
	editor:entityModelCustomColorChannel(uid, 0, color, tonumber(str))
end
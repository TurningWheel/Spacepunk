function process()
end

function field(uid, channel, color, str)
	editor:entityModelCustomColorChannel(uid, channel, color, tonumber(str))
end
function process()
end

function buttonClose()
	frame:removeSelf()
end

function buttonAddComponent(uid, type)
	editor:entityAddComponent(uid, type)
end
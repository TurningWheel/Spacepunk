function process()
end

function buttonClose()
	frame:removeSelf()
end

function buttonYes()
	editor:buttonSave()
	engine:shutdown()
end

function buttonNo()
	engine:shutdown()
end
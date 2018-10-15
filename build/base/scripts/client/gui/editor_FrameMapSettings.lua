function process()
end

function buttonClose()
	frame:removeSelf()
end

function buttonOkay()
	editor:buttonMapSettingsApply()
	frame:removeSelf()
end

function buttonRotate(rotate)
	editor:buttonWorldRotate(rotate)
end
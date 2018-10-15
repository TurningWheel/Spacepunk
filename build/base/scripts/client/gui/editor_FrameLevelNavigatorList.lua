function process()
end

function entityClick(uid)
	editor:setEditingMode(2)
	editor:selectAllEntities(false)
	editor:selectEntity(uid,true)
end

function entityCtrlClick(uid)
	editor:setEditingMode(2)
	editor:toggleSelectEntity(uid)
end

function entityHighlight(uid)
	--editor:playSound("editor/rollover.wav")
end

function entityHighlighting(uid)
	editor:setHighlightedObj(uid)
end
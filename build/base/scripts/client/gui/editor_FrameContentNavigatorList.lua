function process()
end

function spawnClick(str)
	editor:setEditingMode(2)
	editor:selectAllEntities(false)
	editor:selectEntityForSpawn(str)
	editor:playSound("editor/pickup.wav")
	engine:msg(1, string.format("Spawning entity \"%s\"", str))
end

function spawnCtrlClick(str)
	editor:setEditingMode(2)
	editor:selectEntityForSpawn(str)
	editor:playSound("editor/pickup.wav")
	engine:msg(1, string.format("Spawning entity \"%s\"", str))
end

function spawnHighlight(str)
	--editor:playSound("editor/rollover.wav")
end

function spawnHighlighting(str)
end
function process()
end

function buttonNew()
	editor:buttonNew()
end

function buttonSave()
	editor:buttonSave()
end

function buttonLoad()
	editor:buttonLoad()
end

function buttonTiles()
	engine:msg(1,"Editing tiles")
	editor:setEditingMode(0)
	editor:selectAllEntities(false)
end

function buttonTextures()
	engine:msg(1,"Editing tile textures")
	editor:setEditingMode(1)
	editor:selectAllEntities(false)
end

function buttonEntities()
	engine:msg(1,"Editing entities")
	editor:setEditingMode(2)
	local world = client:getWorld(0)
	world:deselectGeometry()
end

function buttonSectors()
	engine:msg(1,"Editing sectors")
	editor:setEditingMode(3)
	editor:selectAllEntities(false)
	local world = client:getWorld(0)
	world:deselectGeometry()
end

function buttonPreview()
	engine:msg(1,"Toggle preview")
	local world = client:getWorld(0)
	world:setShowTools(world:isShowTools()==false)
end

function buttonOptimize()
	engine:msg(1,"Optimizing chunk geometry...")
	editor:optimizeChunks()
end

function buttonEditorSettings()
	editor:buttonEditorSettings()
end

function buttonMapSettings()
	editor:buttonMapSettings()
end

function buttonPlay()
	engine:msg(1,"Playtesting level...")
	engine:editorPlaytest()
end

function buttonHelp()
	editor:buttonHelp()
end
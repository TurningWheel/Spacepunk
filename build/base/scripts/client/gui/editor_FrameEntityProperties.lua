function process()
end

function buttonFlag(flag)
	editor:entitiesFlag(flag)
end

function buttonSave()
	editor:entitiesSave()
end

-- General component buttons:

function buttonCollapse(uid)
	editor:entityComponentCollapse(uid)
end

function buttonExpand(uid)
	editor:entityComponentExpand(uid)
end

function buttonAdd(uid)
	editor:buttonEntityAddComponent(uid)
end

function buttonDelete(uid)
	editor:entityRemoveComponent(uid)
end

function buttonCopy(uid)
	editor:entityCopyComponent(uid)
end
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

-- BBox component:

function buttonBBoxEnabled(uid)
	editor:entityBBoxEnabled(uid)
end

-- Model component:

function buttonModelCustomColor(uid)
	editor:entityModelCustomColor(uid)
end

-- Light component:

function buttonLightShadowEnabled(uid)
	editor:entityLightShadow(uid)
end

-- Camera component:

function buttonCameraOrtho(uid)
	editor:entityCameraOrtho(uid)
end

-- Speaker component:

function buttonSpeakerDefaultLoop(uid)
	editor:entitySpeakerDefaultLoop(uid)
end
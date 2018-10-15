function process()
end

function propertyZ(str)
	if editor:getWidgetMode() == 1 then
		editor:widgetTranslateZ(tonumber(str))
	elseif editor:getWidgetMode() == 2 then
		editor:widgetRotateYaw(tonumber(str))
	elseif editor:getWidgetMode() == 3 then
		editor:widgetScaleZ(tonumber(str))
	end
end
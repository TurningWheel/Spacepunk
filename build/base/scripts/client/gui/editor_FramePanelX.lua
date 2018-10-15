function process()
end

function propertyX(str)
	if editor:getWidgetMode() == 1 then
		editor:widgetTranslateX(tonumber(str))
	elseif editor:getWidgetMode() == 2 then
		editor:widgetRotateRoll(tonumber(str))
	elseif editor:getWidgetMode() == 3 then
		editor:widgetScaleX(tonumber(str))
	end
end
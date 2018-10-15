function process()
end

function propertyY(str)
	if editor:getWidgetMode() == 1 then
		editor:widgetTranslateY(tonumber(str))
	elseif editor:getWidgetMode() == 2 then
		editor:widgetRotatePitch(tonumber(str))
	elseif editor:getWidgetMode() == 3 then
		editor:widgetScaleY(tonumber(str))
	end
end
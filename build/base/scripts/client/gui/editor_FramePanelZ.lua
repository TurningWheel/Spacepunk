function process()
end

function propertyZ(str)
	local num = tonumber(str)
	if num ~= nil then
		if editor:getWidgetMode() == 1 then
			editor:widgetTranslateZ(num)
		elseif editor:getWidgetMode() == 2 then
			editor:widgetRotateYaw(num)
		elseif editor:getWidgetMode() == 3 then
			editor:widgetScaleZ(num)
		end
	end
end
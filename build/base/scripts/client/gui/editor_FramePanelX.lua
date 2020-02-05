function process()
end

function propertyX(str)
	local num = tonumber(str)
	if num ~= nil then
		if editor:getWidgetMode() == 1 then
			editor:widgetTranslateX(num)
		elseif editor:getWidgetMode() == 2 then
			editor:widgetRotateRoll(num)
		elseif editor:getWidgetMode() == 3 then
			editor:widgetScaleX(num)
		end
	end
end
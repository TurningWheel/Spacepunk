function process()
end

function propertyY(str)
	local num = tonumber(str)
	if num ~= nil then
		if editor:getWidgetMode() == 1 then
			editor:widgetTranslateY(num)
		elseif editor:getWidgetMode() == 2 then
			editor:widgetRotatePitch(num)
		elseif editor:getWidgetMode() == 3 then
			editor:widgetScaleY(num)
		end
	end
end
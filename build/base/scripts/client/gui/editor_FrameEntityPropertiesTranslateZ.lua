function process()
end

function field(str)
	local num = tonumber(str)
	if num ~= nil then
		editor:widgetTranslateZ(num)
	end
end
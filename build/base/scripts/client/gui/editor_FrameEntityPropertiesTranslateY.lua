function process()
end

function field(str)
	local num = tonumber(str)
	if num ~= nil then
		editor:widgetTranslateY(num)
	end
end
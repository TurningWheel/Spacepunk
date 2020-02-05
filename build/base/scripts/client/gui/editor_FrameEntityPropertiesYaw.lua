function process()
end

function field(str)
	local num = tonumber(str)
	if num ~= nil then
		editor:widgetRotateYaw(num)
	end
end
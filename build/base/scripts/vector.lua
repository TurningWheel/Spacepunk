-- Vector operations

function vecAdd(vec1, vec2)
	return Vector(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z)
end

function vecSub(vec1, vec2)
	return Vector(vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z)
end

function vecMul(vec1, vec2)
	return Vector(vec1.x * vec2.x, vec1.y * vec2.y, vec1.z * vec2.z)
end

function vecDiv(vec1, vec2)
	return Vector(vec1.x / vec2.x, vec1.y / vec2.y, vec1.z / vec2.z)
end

function vecEquals(vec1, vec2)
	return vec1.x == vec2.x and vec1.y == vec2.y and vec1.z == vec2.z
end
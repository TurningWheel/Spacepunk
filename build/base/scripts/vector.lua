-- Vector operations

function vecPow(num)
	return Vector(num, num, num)
end

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

function vecNew(vec)
	return Vector(vec.x, vec.y, vec.z)
end

-- WideVector operations

function wvecPow(num)
	return WideVector(num, num, num, num)
end

function wvecAdd(vec1, vec2)
	return WideVector(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z, vec1.w + vec2.w)
end

function wvecSub(vec1, vec2)
	return WideVector(vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z, vec1.w - vec2.w)
end

function wvecMul(vec1, vec2)
	return WideVector(vec1.x * vec2.x, vec1.y * vec2.y, vec1.z * vec2.z, vec1.w * vec2.w)
end

function wvecDiv(vec1, vec2)
	return WideVector(vec1.x / vec2.x, vec1.y / vec2.y, vec1.z / vec2.z, vec1.w / vec2.w)
end

function wvecEquals(vec1, vec2)
	return vec1.x == vec2.x and vec1.y == vec2.y and vec1.z == vec2.z and vec1.w == vec2.w
end

function wvecNew(vec)
	return WideVector(vec.x, vec.y, vec.z, vec.w)
end
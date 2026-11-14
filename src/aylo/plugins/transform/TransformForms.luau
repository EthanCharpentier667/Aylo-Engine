-- engine.math.quat: rotations of aylo.Transform3D, which are unit quaternions { x, y, z, w }. Nothing here is needed:
-- a rotation can be written by hand. Angles are in radians; axes are right-handed, Y up, -Z forward.
--
--   transform.rotation = engine.math.quat.fromEuler(0, math.rad(90), 0)        -- a quarter turn to the left
--   local forward = engine.math.quat.rotate(transform.rotation, vector.create(0, 0, -1))
--   transform.rotation = engine.math.quat.lookRotation(target - transform.position)
--
-- A vector argument is a Luau vector or a table { x, y, z }; a vector result is a Luau vector.
local form = "engine.math.quat"

engine.math = engine.math or {}

local quat = {}

local function fail(message: string)
	error(`{form}: {message}`, 3)
end

local function toVector(value, name: string)
	if type(value) == "vector" then
		return value
	end
	if type(value) == "table" and type(value.x) == "number" and type(value.y) == "number" and type(value.z) == "number" then
		return vector.create(value.x, value.y, value.z)
	end
	fail(`{name} must be a vector or a table {"{"} x, y, z {"}"}`)
	return vector.zero
end

local function check(q, name: string)
	if
		type(q) ~= "table"
		or type(q.x) ~= "number"
		or type(q.y) ~= "number"
		or type(q.z) ~= "number"
		or type(q.w) ~= "number"
	then
		fail(`{name} must be a quaternion {"{"} x, y, z, w {"}"}`)
	end
	return q
end

local function make(x: number, y: number, z: number, w: number)
	return { x = x, y = y, z = z, w = w }
end

--- The rotation that does nothing.
function quat.identity()
	return make(0, 0, 0, 1)
end

--- @return q scaled to length 1 (the identity for a zero quaternion).
function quat.normalize(q)
	check(q, "the quaternion")
	local length = math.sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w)
	if length == 0 then
		return quat.identity()
	end
	return make(q.x / length, q.y / length, q.z / length, q.w / length)
end

--- @return The rotation of `angle` radians around `axis` (counterclockwise looking down the axis).
function quat.fromAxisAngle(axis, angle: number)
	local direction = toVector(axis, "the axis")
	if vector.magnitude(direction) == 0 then
		fail("the axis must not be zero")
	end
	direction = vector.normalize(direction)
	local half = angle / 2
	local s = math.sin(half)
	return make(direction.x * s, direction.y * s, direction.z * s, math.cos(half))
end

--- @return The rotation that applies `b`, then `a` (the product a * b).
function quat.multiply(a, b)
	check(a, "the first quaternion")
	check(b, "the second quaternion")
	return make(
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
	)
end

--- @return The rotation that undoes `q` (of a unit quaternion).
function quat.inverse(q)
	check(q, "the quaternion")
	local lengthSquared = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w
	if lengthSquared == 0 then
		return quat.identity()
	end
	return make(-q.x / lengthSquared, -q.y / lengthSquared, -q.z / lengthSquared, q.w / lengthSquared)
end

--- @return `v` turned by `q`.
function quat.rotate(q, v)
	check(q, "the quaternion")
	local point = toVector(v, "the vector")
	local axis = vector.create(q.x, q.y, q.z)
	local t = vector.cross(axis, point) * 2
	return point + t * q.w + vector.cross(axis, t)
end

--- @return The rotation of `x` radians around X (pitch), `y` around Y (yaw) and `z` around Z (roll), applied as Unity
--- does: Z, then X, then Y.
function quat.fromEuler(x: number, y: number, z: number)
	if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" then
		fail("fromEuler takes three numbers, in radians")
	end
	local pitch = quat.fromAxisAngle(vector.create(1, 0, 0), x)
	local yaw = quat.fromAxisAngle(vector.create(0, 1, 0), y)
	local roll = quat.fromAxisAngle(vector.create(0, 0, 1), z)
	return quat.multiply(yaw, quat.multiply(pitch, roll))
end

--- @return The angles fromEuler takes back: x, y, z, in radians. At a pitch of +-90 degrees, z is 0.
function quat.toEuler(q)
	q = quat.normalize(q)
	local x, y, z, w = q.x, q.y, q.z, q.w
	local m02 = 2 * (x * z + w * y)
	local m10 = 2 * (x * y + w * z)
	local m11 = 1 - 2 * (x * x + z * z)
	local m12 = 2 * (y * z - w * x)
	local m00 = 1 - 2 * (y * y + z * z)
	local m20 = 2 * (x * z - w * y)
	local m22 = 1 - 2 * (x * x + y * y)
	local pitch = math.asin(math.clamp(-m12, -1, 1))
	if math.abs(m12) < 0.9999 then
		return pitch, math.atan2(m02, m22), math.atan2(m10, m11)
	end
	return pitch, math.atan2(-m20, m00), 0
end

--- @return The rotation that makes -Z (the forward of a camera or a model) point along `direction`, its Y as close to
--- `up` (default: world up) as it can.
function quat.lookRotation(direction, up)
	local forward = toVector(direction, "the direction")
	if vector.magnitude(forward) == 0 then
		fail("the direction must not be zero")
	end
	local upward = if up == nil then vector.create(0, 1, 0) else toVector(up, "up")
	local back = -vector.normalize(forward)
	local right = vector.cross(upward, back)
	if vector.magnitude(right) < 1e-6 then
		-- Looking straight along `up`: any other axis gives a right.
		right = vector.cross(if math.abs(back.x) < 0.9 then vector.create(1, 0, 0) else vector.create(0, 0, 1), back)
	end
	right = vector.normalize(right)
	local newUp = vector.cross(back, right)
	-- The columns of the rotation matrix are right, newUp and back.
	local m00, m01, m02 = right.x, newUp.x, back.x
	local m10, m11, m12 = right.y, newUp.y, back.y
	local m20, m21, m22 = right.z, newUp.z, back.z
	local trace = m00 + m11 + m22
	if trace > 0 then
		local s = math.sqrt(trace + 1) * 2
		return make((m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, s / 4)
	elseif m00 > m11 and m00 > m22 then
		local s = math.sqrt(1 + m00 - m11 - m22) * 2
		return make(s / 4, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s)
	elseif m11 > m22 then
		local s = math.sqrt(1 + m11 - m00 - m22) * 2
		return make((m01 + m10) / s, s / 4, (m12 + m21) / s, (m02 - m20) / s)
	end
	local s = math.sqrt(1 + m22 - m00 - m11) * 2
	return make((m02 + m20) / s, (m12 + m21) / s, s / 4, (m10 - m01) / s)
end

--- @return The rotation `t` of the way from `a` to `b` (0: `a`, 1: `b`), along the short arc.
function quat.slerp(a, b, t: number)
	check(a, "the first quaternion")
	check(b, "the second quaternion")
	local dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
	local bx, by, bz, bw = b.x, b.y, b.z, b.w
	if dot < 0 then
		dot, bx, by, bz, bw = -dot, -bx, -by, -bz, -bw
	end
	if dot > 0.9995 then
		return quat.normalize(make(a.x + (bx - a.x) * t, a.y + (by - a.y) * t, a.z + (bz - a.z) * t, a.w + (bw - a.w) * t))
	end
	local theta = math.acos(dot)
	local sinTheta = math.sin(theta)
	local wa = math.sin((1 - t) * theta) / sinTheta
	local wb = math.sin(t * theta) / sinTheta
	return make(a.x * wa + bx * wb, a.y * wa + by * wb, a.z * wa + bz * wb, a.w * wa + bw * wb)
end

engine.math.quat = quat

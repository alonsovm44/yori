// program1.zig - Geometría 3D
const std = @import("std");

pub const Point3D = struct {
    x: f32,
    y: f32,
    z: f32,
};

pub fn get_distance(p1: Point3D, p2: Point3D) f32 {
    const dx = p1.x - p2.x;
    const dy = p1.y - p2.y;
    const dz = p1.z - p2.z;
    return std.math.sqrt(dx*dx + dy*dy + dz*dz);
}
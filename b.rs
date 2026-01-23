// program2.rs - Lógica de Inventario
pub struct Item {
    pub id: u32,
    pub weight: f32,
}

pub fn validate_capacity(items: &[Item], max_weight: f32) -> bool {
    let total: f32 = items.iter().map(|i| i.weight).sum();
    total <= max_weight
}
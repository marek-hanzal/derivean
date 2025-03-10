pub fn seed_of(seed: &String) -> u32 {
    let mut hash: u32 = 5381;

    for byte in seed.bytes() {
        hash = ((hash << 5) - hash) + byte as u32;
        hash |= 0;
    }

    hash
}

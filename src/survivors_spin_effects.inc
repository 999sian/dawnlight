// Native normal/Great Spin particles, shared by moving waves and local pulses.
constexpr std::array<u16, 2> kSurvivorSpinEffects{ID_ZI_J_KAITENGIRI_A, ID_ZI_J_KAITENGIRI_B};
constexpr std::array<u16, 6> kSurvivorGreatSpinEffects{
    ID_ZI_J_KAITENGIRID_A, ID_ZI_J_KAITENGIRID_B, ID_ZI_J_KAITENGIRID_C,
    ID_ZI_J_KAITENGIRID_D, ID_ZI_J_KAITENGIRID_E, ID_ZI_J_KAITENGIRID_F};
std::span<const u16> survivor_spin_effects(bool great) {
    return great ? std::span<const u16>(kSurvivorGreatSpinEffects) : std::span<const u16>(kSurvivorSpinEffects);
}

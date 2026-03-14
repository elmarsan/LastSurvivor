#pragma once

enum WeaponType
{
    WeaponType_Hand,
    WeaponType_Pistol,
    WeaponType_Shotgun
};

// TODO: sfx
struct Weapon
{
    WeaponType type;
    s32        damage;
    f32        knockbackforce;
};

// TODO: Tweak damage
global_variable Weapon gWeaponPistol{ WeaponType_Pistol, 30, 5.0f };
global_variable Weapon gWeaponHand{ WeaponType_Hand, 20, 17.0f };
// Fill out your copyright notice in the Description page of Project Settings.

#include "Sniper.h"

ASniper::ASniper()
{
	// Assegnazione delle statistiche come da specifiche PDF
	MovementRange = 4;
	AttackType = EAttackType::Ranged; // Attacco a distanza
	AttackRange = 10;
	MinDamage = 4;
	MaxDamage = 8;
	HealthPoints = 20;
}
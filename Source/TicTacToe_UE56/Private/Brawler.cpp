// Fill out your copyright notice in the Description page of Project Settings.

#include "Brawler.h"

ABrawler::ABrawler()
{
	//Statistiche:
	MovementRange = 6;
	AttackType = EAttackType::Melee; // Attacco a corto raggio
	AttackRange = 1;
	MinDamage = 1;
	MaxDamage = 6;
	HealthPoints = 40;
	MaxHP = 40; // X respawn
}
#ifndef __HEALTHSYSTEM_H__
#define __HEALTHSYSTEM_H__

struct HealthSystem
{
private:

	int health;
	int maxHealth;
	int defaultHealth;

	bool isDeath;

protected:
	bool damaged_now;

public:

	HealthSystem();

	//Initilization
	void Init();

	//Function to substract health from enemy attack
	bool RecieveDamage(int value);

	//Getters
	int GetHealth();

	//Setters
	void SetMaxHealth(int value);
	int GetMaxHealth();
	void IncreaseHealth(int value);

	void SetDefaultHealth();

	void IncreaseHealthMonk();

	void SetHealth(int value);

	void SetMaxUnitHealth();

	void DivideHealth();
	bool IsDeath();

};

#endif // !__HEALTHSYSTEM_H__

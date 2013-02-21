#include "qSlicerProtonDoseBeamParameters.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

// accessors definitions

void ProtonBeamParameters::accessBeamName(std::string newBeamName) 
{
	m_beamName = newBeamName;
}

void ProtonBeamParameters::accessIsocenter_x(double newIsocenter_x)
{
	m_isocenter_x = newIsocenter_x;
}
void ProtonBeamParameters::accessIsocenter_y(double newIsocenter_y)
{
	m_isocenter_y = newIsocenter_y;
	
}
void ProtonBeamParameters::accessIsocenter_z(double newIsocenter_z)
{
	m_isocenter_z = newIsocenter_z;
}
void ProtonBeamParameters::accessGantryAngle(double newGantryAngle)
{
	m_gantryAngle = newGantryAngle;
}
void ProtonBeamParameters::accessCollimatorAngle(double newCollimatorAngle)
{
	m_collimatorAngle = newCollimatorAngle;
}
void ProtonBeamParameters::accessRange(double newRange)
{
	m_range = newRange;
}
void ProtonBeamParameters::accessMod(double newMod)
{
	m_mod = newMod;
}
void ProtonBeamParameters::accessJaw_x(double newJaw_x)
{
	m_jaw_x = newJaw_x;
}
void ProtonBeamParameters::accessJaw_y(double newJaw_y)
{
	m_jaw_y = newJaw_y;
}
void ProtonBeamParameters::accessCouchRotation(double newCouchRotation)
{
	m_couchRotation = newCouchRotation;
}
void ProtonBeamParameters::accessCouchPitch(double newCouchPitch)
{
	m_couchPitch = newCouchPitch;
}
void ProtonBeamParameters::accessCouchRoll(double newCouchRoll)
{
	m_couchRoll = newCouchRoll;
}

// Reader definitions

std::string ProtonBeamParameters::readBeamName()
{
	return m_beamName;
}
double ProtonBeamParameters::readIsocenter_x()
{
	return m_isocenter_x;
}
double ProtonBeamParameters::readIsocenter_y()
{
	return m_isocenter_y;
}
double ProtonBeamParameters::readIsocenter_z()
{
	return m_isocenter_z;
}
double ProtonBeamParameters::readGantryAngle()
{
	return m_gantryAngle;
}
double ProtonBeamParameters::readCollimatorAngle()
{
	return m_collimatorAngle;
}
double ProtonBeamParameters::readRange()
{
	return m_range;
}
double ProtonBeamParameters::readMod()
{
	return m_mod;
}
double ProtonBeamParameters::readJaw_x()
{
	return m_jaw_x;
}
double ProtonBeamParameters::readJaw_y()
{
	return m_jaw_y;
}
double ProtonBeamParameters::readCouchRotation()
{
	return m_couchRotation;
}
double ProtonBeamParameters::readCouchPitch()
{
	return m_couchPitch;
}
double ProtonBeamParameters::readCouchRoll()
{
	return m_couchRoll;
}
double ProtonBeamParameters::readSourceX()
{
	return m_sourceX;
}

double ProtonBeamParameters::readSourceY()
{
	return m_sourceY;
}

double ProtonBeamParameters::readSourceZ()
{
	return m_sourceZ;
}

// Source position calculation in the patient reference (no roll pitch or collimator)

void ProtonBeamParameters::calculateSourcePosition()
{
	double PI = 3.14159265359;
	m_sourceX = m_isocenter_x + m_DSA*sin(m_gantryAngle*PI/180);
	m_sourceY = m_isocenter_y - m_DSA*cos(m_gantryAngle*PI/180);
	m_sourceZ = m_isocenter_z;
}

// constructor of a default beam
ProtonBeamParameters::ProtonBeamParameters()
{
	m_beamName = "defaultname";
	m_isocenter_x = 0;
	m_isocenter_y = 0;
	m_isocenter_z = 0;
	m_gantryAngle = 0;
	m_collimatorAngle = 0;
	m_range = 0;
	m_mod = 0;
	m_jaw_x = 10;
	m_jaw_y = 10;
	m_couchRotation = 0;
	m_couchPitch = 0;
	m_couchRoll = 0;
	m_sourceX = 0;   // position initial of the source in CC
	m_sourceY = 0;
	m_sourceZ = 10000; 
	m_DSA = 10000; // DSA far away to create a parallele beam
}
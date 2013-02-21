#ifndef qSlicerProtonDoseBeamParameters
#define qSlicerProtonDoseBeamParameters

#include <iostream>
#include <string>

class ProtonBeamParameters
{
	// Methods
public:
	ProtonBeamParameters();  // accessor of the beam parameters
	void accessBeamName(std::string);
	void accessIsocenter_x(double);
	void accessIsocenter_y(double);
	void accessIsocenter_z(double);
	void accessGantryAngle(double);
	void accessCollimatorAngle(double);
	void accessRange(double);
	void accessMod(double);
	void accessJaw_x(double);
	void accessJaw_y(double);
	void accessCouchRotation(double);
	void accessCouchPitch(double);
	void accessCouchRoll(double);


	std::string readBeamName(); // reader of the parameters
	double readIsocenter_x();
	double readIsocenter_y();
	double readIsocenter_z();
	double readGantryAngle();
	double readCollimatorAngle();
	double readRange();
	double readMod();
	double readJaw_x();
	double readJaw_y();
	double readCouchRotation();
	double readCouchPitch();
	double readCouchRoll();
	double readSourceX();
	double readSourceY();
	double readSourceZ();

	void calculateSourcePosition(); // function to calculate the source position in the patient reference

	// Members
private:
	std::string m_beamName;
	double m_isocenter_x, m_isocenter_y, m_isocenter_z; // isocenter
	double m_gantryAngle, m_collimatorAngle; // rotations of the gantry
	double m_range, m_mod; // energy parameters of the beam
	double m_jaw_x, m_jaw_y; // collimator parameters
	double m_couchRotation, m_couchPitch, m_couchRoll; // rotations of the couch
	double m_sourceX, m_sourceY, m_sourceZ; // x,y,z of the couch (DICOM)
	double m_DSA; // Distance Source Axis
};

#endif
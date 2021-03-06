/**

 * Zenom - Hard Real-Time Simulation Enviroment
 * @author zenom
 *
 * TargetTest
 * A simple example of a control program.
 * Does not require any hardware, just generates a sine signal.
 */
#include <iostream>
#include <controlbase.h>
#include <math.h>
#include <Integrator.hpp>
#include <Differentiator.hpp>

class TargetTest : public ControlBase
{
public:

	// ----- User Functions -----
	// This functions need to be implemented by the user.
	int initialize();
	int start();
	int doloop();
	int stop();
	int terminate();

private:
	// ----- Log Variables -----
	double tau[2];
	double e[2];
	double ed[2];


	// ----- Control Parameters -----
	double q[2];
	double a[2];
	double freq[2];
	double Ks[2];
	double Ke[2];
	double Kr[2];
	double C[2];
	double alpha[2];
	double deltaE[2];
	double deltaR[2];


	// ----- Variables -----
	Integrator< double > mrInt[2];  // p_dot integrator
	Integrator< double > mrInt2[2];  // p_dot integrator
	Integrator< double > mrInt3[2];  // p_dot integrator
	Differentiator< double > mqDiff[2]; // q differentiator

	double d[2];
	double dd[2];
	double qd[2];
	double r[2];
	double rint[2];
	double rint2[2];
	double rint3[2];
	double ke[2];
	double kr[2];
	double t,t2,t3;
	double dE[2];
	double dR[2];

	const double pi = 3.14159265359;
	const double Deg2Rad = pi / 180;
	const double Rad2Deg = 180 / pi;

	double sgn(double val){
		if(val >= 0) return 1.0;
		else return -1.0;
	}
};

/**
 * This function is called when the control program is loaded to zenom.
 * Use this function to register control parameters, to register log variables
 * and to initialize control parameters.
 *
 * @return Return non-zero to indicate an error.
 */
int TargetTest::initialize()
{
	// ----- Initializes log and control variables -----
	// ----- Register the log variables -----
	registerLogVariable(tau, "tau", 1, 2);
	registerLogVariable(e, "e", 1, 2);
	registerLogVariable(ed, "ed", 1, 2);
	registerLogVariable(r, "r", 1, 2);
	registerLogVariable(kr, "kr", 1, 2);
	registerLogVariable(ke, "ke", 1, 2);

	// ----- Register the control paramateres -----
	registerControlVariable(q, "q", 1, 2);
	registerControlVariable(a, "a", 1, 2);
	registerControlVariable(freq, "freq", 1, 2);
	registerControlVariable(Kr, "Kr", 1, 2);
	registerControlVariable(Ke, "Ke", 1, 2);
	registerControlVariable(Ks, "Ks", 1, 2);
	registerControlVariable(C, "C", 1, 2);
	registerControlVariable(alpha, "alpha", 1, 2);
	registerControlVariable(deltaE, "deltaE", 1, 2);
	registerControlVariable(deltaR, "deltaR", 1, 2);

	a[0] = 0.7;
	a[1] = 1.2;
	freq[0] = 3;
	freq[1] = 3;

	alpha[0] = 10;
	alpha[1] = 10;

	Kr[0] = 35;
	Kr[1] = 30;

	Ke[0] = 35;
	Ke[1] = 30;

	Ks[0] = 150;
	Ks[1] = 30;

	C[0] = 100;
	C[1] = 1000;

	deltaE[0] = 10 * Deg2Rad;
	deltaE[1] = 10 * Deg2Rad;

	deltaR[0] = 10 * Deg2Rad;
	deltaR[1] = 10 * Deg2Rad;

	// ----- Prints message in screen -----
	std::cout
		<< "Position and Velocity Constrained Full State Feedback Nonlinear PI for 2-link planar robot "
		<< std::endl << std::endl;

	return 0;
}

/**
 * This function is called when the START button is pushed from zenom.
 *
 * @return If you return 0, the control starts and the doloop() function is
 * called periodically. If you return nonzero, the control will not start.
 */
int TargetTest::start()
{
	mrInt[0].setSamplingPeriod( period() );
	mrInt[0].reset( 0 );
	mrInt[1].setSamplingPeriod( period() );
	mrInt[1].reset( 0 );

	mrInt2[0].setSamplingPeriod( period() );
	mrInt2[0].reset( 0 );
	mrInt2[1].setSamplingPeriod( period() );
	mrInt2[1].reset( 0 );

	mrInt3[0].setSamplingPeriod( period() );
	mrInt3[0].reset( 0 );
	mrInt3[1].setSamplingPeriod( period() );
	mrInt3[1].reset( 0 );

	mqDiff[0].reset();
	mqDiff[0].setSamplingPeriod( period() );
	mqDiff[0].enableFilter();
	mqDiff[0].setDampingRatio(1);
	mqDiff[0].setCutOffFrequencyHz(500);

	mqDiff[1].reset();
	mqDiff[1].setSamplingPeriod( period() );
	mqDiff[1].enableFilter();
	mqDiff[1].setDampingRatio(1);
	mqDiff[1].setCutOffFrequencyHz(500);

	q[0] = 0;
	q[1] = 0;

	qd[0] = 0;
	qd[1] = 0;

	rint[0] = 0;
	rint[1] = 0;

	rint2[0] = 0;
	rint2[1] = 0;

	rint3[0] = 0;
	rint3[1] = 0;

	return 0;
}


/**
 * This function is called periodically (as specified by the control frequency).
 * The useful functions that you can call used in doloop() are listed below.
 *
 * frequency()          returns frequency of simulation.
 * period()             returns period of simulation.
 * duration()           returns duration of simulation.
 * simTicks()           returns elapsed simulation ticks.
 * simTimeInNano()      returns elapsed simulation time in nano seconds.
 * simTimeInMiliSec()   returns elapsed simulation time in miliseconds.
 * simTimeInSec()       returns elapsed simulation time in seconds.
 * overruns()           returns the count of overruns.
 *
 * @return If you return 0, the control will continue to execute. If you return
 * nonzero, the control will abort.
 */
int TargetTest::doloop()
{
	//derivate q to get qd
	qd[0] = mqDiff[0].differentiate(q[0]);
	qd[1] = mqDiff[1].differentiate(q[1]);

	//desired trajectory
	t = elapsedTime();
	t2 = pow(t,2);
	t3 = pow(t,3);
	d[0] = a[0] * sin(t*freq[0]) * (1-exp(-0.3 * t3));
	d[1] = a[1] * sin(t*freq[1]) * (1-exp(-0.3 * t3));
	dd[0]= a[0]*(freq[0]*cos(t*freq[0])*(1-exp(-0.3*t3))+0.9*t2*exp(-0.3*t3)*sin(t*freq[0]));
	dd[1]= a[1]*(freq[1]*cos(t*freq[1])*(1-exp(-0.3*t3))+0.9*t2*exp(-0.3*t3)*sin(t*freq[1]));

	// error
	e[0] = d[0] - q[0] ;
	e[1] = d[1] - q[1] ;
	ed[0] = dd[0] - qd[0];
	ed[1] = dd[1] - qd[1];

	dE[0] = pow((deltaE[0]*Deg2Rad),2);
	dE[1] = pow((deltaE[1]*Deg2Rad),2);

	dR[0] = pow((deltaR[0]*Deg2Rad),2);
	dR[1] = pow((deltaR[1]*Deg2Rad),2);

	ke[0] = Ke[0] * dE[0] / (dE[0] - e[0]*e[0]);
	ke[1] = Ke[1] * dE[1] / (dE[1] - e[1]*e[1]);

	if(ke[0] < 0) ke[0] = Ke[0]*5;
	if(ke[1] < 0) ke[1] = Ke[1]*5;

	r[0] = ed[0]+ke[0]*e[0];
	r[1] = ed[1]+ke[1]*e[1];

	kr[0] = Kr[0] * dR[0] / (dR[0] - r[0]*r[0]);
	kr[1] = Kr[1] * dR[1] / (dR[1] - r[1]*r[1]);

	if(kr[0] < 0) kr[0] = Kr[0]*5;
	if(kr[1] < 0) kr[1] = Kr[1]*5;

	// integrate r
	rint[0] = mrInt[0].integrate(r[0]);
	rint[1] = mrInt[1].integrate(r[1]);

	rint2[0] = mrInt2[0].integrate(sgn(r[0]));
	rint2[1] = mrInt2[1].integrate(sgn(r[1]));

	rint3[0] = mrInt3[0].integrate(kr[0]*r[0]);
	rint3[1] = mrInt3[1].integrate(kr[1]*r[1]);

	// calculate tau
	tau[0] = Ks[0]*(r[0] + alpha[0]*rint[0]) + C[0]*rint2[0] + rint3[0];
	tau[1] = Ks[1]*(r[1] + alpha[1]*rint[1]) + C[1]*rint2[1] + rint3[1];

	//to see errors in degree form
	e[0] = e[0] * Rad2Deg;
	e[1] = e[1] * Rad2Deg;

	ed[0] = ed[0] * Rad2Deg;
	ed[1] = ed[1] * Rad2Deg;

	r[0] = r[0] * Rad2Deg;
	r[1] = r[1] * Rad2Deg;

	// for robot
	tau[0] = tau[0] * 10.0 / 287.0;
	tau[1] = tau[1] * -10.0 / 52.0;

	// bound tau
	if(tau[0] > 9.6) tau[0] = 9.6;
	if(tau[0] < -9.6) tau[0] = -9.6;

	if(tau[1] > 9.6) tau[1] = 9.6;
	if(tau[1] < -9.6) tau[1] = -9.6;

	return 0;
}


/**
 * Called when a timed run ends or the STOP button is pushed from zenom.
 *
 * @return Return non-zero to indicate an error.
 */
int TargetTest::stop()
{


	return 0;
}


/**
 * This function is called when the control is unloaded. It happens when
 * the user loads a new control program or exits.
 *
 * @return Return non-zero to indicate an error.
 */
int TargetTest::terminate()
{


	return 0;
}


/**
 * The main function starts the control program
 */
int main( int argc, char *argv[] )
{
	TargetTest c;
	c.run( argc, argv );

	return 0;
}
